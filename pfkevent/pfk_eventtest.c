
#include <linux/init.h>
#include <linux/module.h>

#include "pfk_printdev.h"
#include "pfk_event.h"

MODULE_LICENSE("Dual BSD/GPL");

static int testmod_init(void)
{
    printk(KERN_ERR "Hello, world\n");
    printdev_init();
    pfk_event_log_init("eventtest");
    pfk_event_max_events(100);
    return 0;
}

static void testmod_exit(void)
{
    printdev_exit();
    pfk_event_log_exit();
    printk(KERN_ERR "Goodbye, cruel world\n");
}

module_init(testmod_init);
module_exit(testmod_exit);
