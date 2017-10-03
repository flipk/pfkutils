
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <stdarg.h>

#include "pfk_printdev.h"
#include "cycle_count_omap.h"

dev_t printdev_dev;
int printdev_major;
struct class * printdev_class;
struct cdev printdev_cdev;
struct device * printdev_device;
wait_queue_head_t process_read_wait_queue;

#define BUFFER_SIZE (4 * 1024 * 1024)

char buffer[BUFFER_SIZE];
int buffer_inpos;
int buffer_outpos;
int buffer_used;
spinlock_t buffer_lock;

static int
free_space ( void )
{
    return (BUFFER_SIZE-1) - buffer_used;
}

static int
contig_readable  (void)
{
    return
        (buffer_inpos >= buffer_outpos) ?
        (buffer_inpos - buffer_outpos) :
        (BUFFER_SIZE - buffer_outpos);
}

static int
contig_writeable (void)
{
    int r =
        (buffer_outpos >  buffer_inpos) ?
        (buffer_outpos - buffer_inpos) :
        (BUFFER_SIZE -  buffer_inpos);
    int f = free_space();
    return (r > f) ? f : r;
}

int device_open_flag;

#define lockdecl() unsigned long __spinlock_flags_save
#define lock()     spin_lock_irqsave     (&buffer_lock, __spinlock_flags_save)
#define unlock()   spin_unlock_irqrestore(&buffer_lock, __spinlock_flags_save)

static int
user_open    (struct inode *ino, struct file *f)
{
    if (device_open_flag)
    {
        PFKPR("user_open : error opening because already open %d\n",
              4);
        pfk_printhexdev((unsigned char *)&user_open, 200);
        return -EPERM;
    }
    device_open_flag = 1;
    buffer_inpos = buffer_outpos = 0;
    buffer_used = 0;
    try_module_get(THIS_MODULE);
    return 0;
}

static int
user_release (struct inode *ino, struct file *f)
{
    device_open_flag = 0;
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t
user_read    (struct file *f, char __user *buf, size_t bsz, loff_t *pos)
{
    lockdecl();
    int cpy, retval = 0;
    // f->private_data

    DECLARE_WAITQUEUE(wait, current);

    add_wait_queue(&process_read_wait_queue, &wait);
    while (1)
    {
        set_current_state(TASK_INTERRUPTIBLE);
        if (signal_pending(current))
        {
            retval = -ERESTARTSYS;
            break;
        }
        if (buffer_used != 0)
            break;
        schedule();
    }
    current->state = TASK_RUNNING;
    remove_wait_queue(&process_read_wait_queue, &wait);

    if (retval)
        return retval;

    if (bsz > buffer_used)
        bsz = buffer_used;
    cpy = contig_readable();
    if (cpy > bsz)
        cpy = bsz;
    if (cpy > 0)
        if (copy_to_user(buf, buffer + buffer_outpos, cpy))
            return -EFAULT;
    if (cpy != bsz)
        if (copy_to_user(buf + cpy, buffer, bsz - cpy))
            return -EFAULT;
    buffer_outpos = (buffer_outpos + bsz) % BUFFER_SIZE;

    lock();
    buffer_used -= bsz;
    unlock();

    return bsz;
}

static ssize_t
user_write   (struct file *f, const char __user *buf,
              size_t len, loff_t *pos)
{
    return -EIO;
}

static long
user_ioctl   (struct file *f,
              unsigned int cmd, unsigned long arg)
{
    return -EIO;
}

static unsigned int
user_poll (struct file *f, struct poll_table_struct *pt)
{
    return -EIO;
}

struct file_operations printdev_fops = {
    .open = user_open,
    .release = user_release,
    .read = user_read,
    .write = user_write,
    .unlocked_ioctl = NULL,
    .compat_ioctl = user_ioctl,
    .poll = user_poll
};

void
printdev_init(void)
{
    int ret;

    device_open_flag = 0;
    buffer_inpos = 0;
    buffer_outpos = 0;
    buffer_used = 0;
    spin_lock_init( &buffer_lock );
    init_waitqueue_head(&process_read_wait_queue);

    ret = alloc_chrdev_region(&printdev_dev, 0, 1, DEVNAME);
    if (ret != 0)
    {
        printk(KERN_ERR "failure to alloc chrdev region\n");
        return;
    }

    printdev_major = MAJOR(printdev_dev);
    printdev_class = class_create(THIS_MODULE, DEVNAME);

    cdev_init( &printdev_cdev, &printdev_fops );
    cdev_add( &printdev_cdev, MKDEV(printdev_major,0), 1);
    printdev_device =
        device_create( printdev_class, /*parent*/ NULL,
                       MKDEV(printdev_major,0), NULL,
                       "%s%d", DEVNAME, 0 );

    PFK_ENABLE_CYCLE_COUNT();
}

void
printdev_exit(void)
{
    cdev_del( &printdev_cdev );
    device_destroy( printdev_class, MKDEV(printdev_major,0) );
    class_destroy( printdev_class );
    unregister_chrdev_region( printdev_dev, 1 );
}

int
pfk_printdev_enabled(void)
{
    return device_open_flag;
}
EXPORT_SYMBOL(pfk_printdev_enabled);

static void
add_data(char * stringbuf, int bsz)
{
    int cpy;
    lockdecl();

    lock();
    if (bsz > free_space())
        bsz = free_space();
    cpy = contig_writeable();
    if (cpy > bsz)
        cpy = bsz;
    if (cpy > 0)
        memcpy( buffer + buffer_inpos, stringbuf, cpy);
    if (cpy != bsz)
        memcpy( buffer, stringbuf + cpy, bsz - cpy );
    buffer_inpos = (buffer_inpos + bsz) % BUFFER_SIZE;
    buffer_used += bsz;
    unlock();

    wake_up_all(&process_read_wait_queue);
}

void
pfk_printdev(char *format, ...)
{
    char stringbuf[500];
    int bsz;
    va_list ap;
    u32 cycles;

    cycles = PFK_CYCLE_COUNT();

    bsz = scnprintf(stringbuf, sizeof(stringbuf), "%u: ", cycles);

    va_start(ap,format);
    bsz += vscnprintf(stringbuf + bsz, sizeof(stringbuf) - bsz, format, ap);
    va_end(ap);

    add_data(stringbuf, bsz);
}
EXPORT_SYMBOL(pfk_printdev);

void
pfk_printhexdev(unsigned char *buf, int len)
{
    char stringbuf[500];
    int bsz, pos;

    while (len > 0)
    {
        int toprint = len;
        if (toprint > 160)
            toprint = 160;
        bsz = 0;
        for (pos = 0; pos < toprint; pos++)
            bsz += scnprintf(stringbuf + bsz, sizeof(stringbuf) - bsz,
                             "%02x ", buf[pos]);
        bsz += scnprintf(stringbuf + bsz, sizeof(stringbuf) - bsz, "\n");
        add_data(stringbuf, bsz);
        len -= toprint;
        buf += toprint;
    }
}
EXPORT_SYMBOL(pfk_printhexdev);
