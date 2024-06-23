#include "asm-generic/gpio.h"
#include "asm/uaccess.h"
#include <linux/module.h>
#include <linux/poll.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>

static struct resource led_resource[]={
[0]={//第0个资源
    .start=131,
    .end=131,
    .flags=IORESOURCE_MEM,
},

};

static void led_release(struct device*dev)
{

}
//分配，设置，注册一个platform_device结构体
static struct platform_device led_dev={
    .name="myled",
    .id=-1,
    .num_resources=ARRAY_SIZE(led_resource),
    .resource=led_resource,
    .dev={
        .release=led_release,
    },
};


static int __init led_dev_init(void)
{
    platform_device_register(&led_dev);
    return 0;
}

static void __exit led_dev_exit(void)
{
    platform_device_unregister(&led_dev);
}
module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");