
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/io.h>

#include "pfk_event.h"
#include "cycle_count_omap.h"

static const char *evt_enum_names[] = {
#define PFK_EVENT(enum,text) #enum ,
   PFK_EVENT_LIST
#undef  PFK_EVENT
};

static const char *evt_names[] = {
#define PFK_EVENT(enum,text) text ,
   PFK_EVENT_LIST
#undef  PFK_EVENT
};

struct event {
   enum pfk_event_enum evt;
   int v1;
   int v2;
   const char * file;
   const char * func;
   int line;
   u32 timestamp;
   u32 jiffies;
   u32 event_number;
};

static int _pfk_event_enabled = 0;

static char proc_dir_name[128];
static struct proc_dir_entry * pfk_event_dir;
static int max_events;
static struct event * events = NULL;
static spinlock_t events_lock;
static int current_event;
static int event_number;

static int
my_isdigit(char c)
{
   if (c >= '0' && c <= '9')
      return 1;
   return 0;
}

static int
atoi(const char *s)
{
   int i=0;
   const char **tmp_s = &s;

   while (my_isdigit(**tmp_s))
      i = i*10 + *((*tmp_s)++) - '0';
   return i;
}

int
pfk_event_enabled(void)
{
    return _pfk_event_enabled;
}
EXPORT_SYMBOL(pfk_event_enabled);

static ssize_t
pfk_enable_write(struct file *file, const char __user *buf,
                 size_t sz, loff_t *off)
{
   char value_string[80];
   int value;

   if (sz > 80)
      sz = 80;
   if (copy_from_user(value_string,buf,sz))
      return -EFAULT;

   value_string[sizeof(value_string)-1] = 0;
   value = atoi(value_string);

   pfk_event_enable(value);

   return sz;
}

int
pfk_enable_show(struct seq_file *seq, void *offset)
{
   switch (_pfk_event_enabled)
   {
   case 0:
       seq_printf(seq, "pfk_event logging disabled\n");
       break;
   case 1:
       seq_printf(seq, "pfk_event logging enabled for one loop\n");
       break;
   case 2:
       seq_printf(seq, "pfk_event logging enabled for continuous operation\n");
       break;
   default:
       seq_printf(seq, "pfk_event enable value invalid\n");
   }
   return 0;
}

static int
pfk_enable_open(struct inode *inode, struct file *file)
{
   return single_open(file, pfk_enable_show, NULL);
}

struct file_operations proc_enable_fops = {
   .owner   = THIS_MODULE,
   .open    = pfk_enable_open,
   .read    = seq_read,
   .write   = pfk_enable_write,
   .llseek  = seq_lseek,
   .release = single_release    
};

static ssize_t
pfk_max_events_write(struct file *file, const char __user *buf,
                     size_t sz, loff_t *off)
{
   char value_string[80];
   int value;

   if (sz > 80)
      sz = 80;
   if (copy_from_user(value_string,buf,sz))
      return -EFAULT;

   value_string[sizeof(value_string)-1] = 0;
   value = atoi(value_string);

   pfk_event_max_events(value);

   return sz;
}

int
pfk_max_events_show(struct seq_file *seq, void *offset)
{
   seq_printf(seq, "%d\n", max_events);
   return 0;
}

static int
pfk_max_events_open(struct inode *inode, struct file *file)
{
   return single_open(file, pfk_max_events_show, NULL);
}

struct file_operations proc_max_events_fops = {
   .owner   = THIS_MODULE,
   .open    = pfk_max_events_open,
   .read    = seq_read,
   .write   = pfk_max_events_write,
   .llseek  = seq_lseek,
   .release = single_release    
};

struct events_stuff {
   int ind;
   int count;
   u32 ts_start;
   u32 ts_prev;
   u32 jif_start;
   int first;
   int temp_pos;
   int temp_len;
   char temp_buf[1024];
};

static int
pfk_events_open(struct inode *inode, struct file *file)
{
   struct events_stuff * es;

   if (_pfk_event_enabled != 0)
   {
      printk(KERN_ERR "pfk_events: must be disabled to show events\n");
      return -EPERM;
   }

   es = (struct events_stuff *)
       kmalloc(sizeof(struct events_stuff), GFP_KERNEL);

   es->ind = current_event;
   es->count = 0;
   es->ts_start = 0;
   es->ts_prev = 0;
   es->jif_start = 0;
   es->first = 1;
   es->temp_pos = -1;
   es->temp_len = 0;

   file->private_data = es;

   return 0;
}

static int
_get_next_event(struct events_stuff * es)
{
    int len = 0;

    if (es->count >= max_events)
        return -1;

    if (es->first)
    {
        es->first = 0;
        len = sprintf(
            es->temp_buf,
            "timestamp,tsdiff,jiffies,evt,name,desc,v1,v2,func,line,file\n");
    }
    else
    {
        struct event * pEvt;
        enum pfk_event_enum evt_id;
        pEvt = &events[es->ind];
        evt_id = pEvt->evt & PFK_EVENT_MASK;

        if (evt_id != PFK_EVENT_NULL)
        {
            if (es->ts_start == 0)
            {
                es->ts_start = pEvt->timestamp;
                es->jif_start = pEvt->jiffies;
            }

            len = snprintf(
                es->temp_buf, sizeof(es->temp_buf),
                "%u,%u,%u,%d,%s,%s,0x%08x,0x%08x,%s,%d,%s\n",
                pEvt->timestamp - es->ts_start,
                (es->ts_prev != 0) ? (pEvt->timestamp - es->ts_prev) : 0,
                pEvt->jiffies - es->jif_start, evt_id,
                evt_enum_names[evt_id], evt_names[evt_id],
                pEvt->v1, pEvt->v2, pEvt->func, pEvt->line,
                pEvt->file);

            es->ts_prev = pEvt->timestamp;
        }

        if (++(es->ind) >= max_events)
            es->ind = 0;

        es->count++;
    }

    return len;
}

static ssize_t
pfk_events_read(struct file *file, char __user *buf, size_t sz, loff_t *off)
{
   struct events_stuff * es = file->private_data;
   int tocopy, len, ret = 0;

   while (sz > 0)
   {
       if (es->temp_pos < 0)
       {
           do {
               len = _get_next_event(es);
           } while (len == 0);

           if (len == -1)
               break;
           es->temp_len = len;
           es->temp_pos = 0;
       }

       tocopy = es->temp_len - es->temp_pos;
       if (tocopy > sz)
           tocopy = sz;

       if (copy_to_user(buf, es->temp_buf, tocopy))
           return -EFAULT;

       buf += tocopy;
       sz -= tocopy;
       ret += tocopy;
       es->temp_pos += tocopy;
       if (es->temp_pos == es->temp_len)
           es->temp_pos = -1;
   }

   return ret;
}

static int
pfk_events_release(struct inode *inode, struct file *file)
{
   kfree(file->private_data);
   return 0;
}

struct file_operations proc_events_fops = {
   .owner   = THIS_MODULE,
   .open    = pfk_events_open,
   .read    = pfk_events_read,
   .release = pfk_events_release
};

int
pfk_current_event_show(struct seq_file *seq, void *offset)
{
   seq_printf(seq, "%d\n", current_event);
   return 0;
}

static int
pfk_current_event_open(struct inode *inode, struct file *file)
{
   return single_open(file, pfk_current_event_show, NULL);
}

struct file_operations proc_current_event_fops = {
   .owner   = THIS_MODULE,
   .open    = pfk_current_event_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release    
};

int
pfk_test_event_show(struct seq_file *seq, void *offset)
{
   if (_pfk_event_enabled == 0)
      seq_printf(seq, "logging disabled, trying anyway\n");
   seq_printf(seq, "generating test event\n");
   PFK_EVENT_LOG(TEST, 1, 2);
   return 0;
}

static int
pfk_test_event_open(struct inode *inode, struct file *file)
{
   return single_open(file, pfk_test_event_show, NULL);
}

struct file_operations proc_test_event_fops = {
   .owner   = THIS_MODULE,
   .open    = pfk_test_event_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release    
};

struct procfiles {
   char * name;
   int mode;
   struct file_operations * fops;
} pfk_event_procfiles[] = {
   { "enable",        0666, &proc_enable_fops        },
   { "max_events",    0666, &proc_max_events_fops    },
   { "events.csv",    0444, &proc_events_fops        },
   { "current_event", 0444, &proc_current_event_fops },
   { "test_event",    0444, &proc_test_event_fops    },
   {  NULL,              0, NULL                     }
};

void
pfk_event_log_init(char *name)
{
   struct procfiles * pf;
   spin_lock_init(&events_lock);
   snprintf(proc_dir_name, sizeof(proc_dir_name), "pfkevt_%s", name);
   pfk_event_dir = proc_mkdir( proc_dir_name, NULL );
   for (pf = pfk_event_procfiles; pf->name; pf++)
      proc_create( pf->name, pf->mode, pfk_event_dir, pf->fops );
   _pfk_event_enabled = 0;
   event_number = 1;
   pfk_event_max_events(100);
}

void
pfk_event_log_exit(void)
{
   struct procfiles * pf;
   _pfk_event_enabled = 0;
   if (events)
      vfree(events);
   for (pf = pfk_event_procfiles; pf->name; pf++)
      remove_proc_entry( pf->name, pfk_event_dir);
   remove_proc_entry(proc_dir_name, NULL);
}

void
pfk_event_log(enum pfk_event_enum evt, int v1, int v2, 
              const char *file, const char *func, int line)
{
   struct event * pEvt;
   int number;

   if (_pfk_event_enabled == 0 || events == NULL)
      return;

   spin_lock(&events_lock);
   pEvt = &events[current_event];
   if (++current_event >= max_events)
   {
      current_event = 0;
      if (_pfk_event_enabled == 1)
      {
         _pfk_event_enabled = 0;
         printk(KERN_ERR "pfk logging for %s disabled due to wrap\n",
                proc_dir_name);
      }
   }
   number = event_number++;
   spin_unlock(&events_lock);

   pEvt->evt = evt;
   pEvt->v1 = v1;
   pEvt->v2 = v2;
   pEvt->timestamp = PFK_CYCLE_COUNT();
   pEvt->jiffies = jiffies;
   pEvt->file = file;
   pEvt->func = func;
   pEvt->line = line;
   pEvt->event_number = number;
}

void
pfk_event_enable(int value)
{
   if (value != 0 && events == NULL)
   {
      printk(KERN_ERR "pfk_event cannot enable: no memory for events\n");
      return;
   }

   PFK_ENABLE_CYCLE_COUNT();

   if (value == 2)
   {
      printk(KERN_ERR "pfk_event logging for %s enabled circularly\n",
             proc_dir_name);
      _pfk_event_enabled = 2;
   }
   else if (value == 1)
   {
      printk(KERN_ERR "pfk_event logging for %s enabled for one loop\n",
             proc_dir_name);
      _pfk_event_enabled = 1;
   }
   else if (value == 0)
   {
      printk(KERN_ERR "pfk_event logging for %s disabled\n", proc_dir_name);
      _pfk_event_enabled = 0;
   }
   else
   {
      printk(KERN_ERR "pfk_event enable values should be 0, 1, or 2\n");
   }
}

void
pfk_event_max_events(int value)
{
   int size;

   if (_pfk_event_enabled != 0)
   {
      printk(KERN_ERR "pfk max_events can only be changed while disabled\n");
      return;
   }

   if (value < 10 || value > 65535)
   {
      printk(KERN_ERR "pfk max_events limited to 10 < events < 65536\n");
      return;
   }

   max_events = value;
   current_event = 0;

   if (events)
      vfree(events);
   size = sizeof(struct event) * max_events;
   printk(KERN_ERR "pfk_events : %d events requires %d bytes of memory\n",
          max_events, size);
   events = (struct event *) vmalloc( size );
   if (!events)
      printk(KERN_ERR "pfk_events : vmalloc for events failed!\n");
   else
      memset(events, 0, size);
}
