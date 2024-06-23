#include "asm-generic/gpio.h"
#include "asm/uaccess.h"
#include "linux/of.h"
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

struct gpio_desc{
	int gpio;
	int irq;
    char *name;
    int key;
	struct timer_list key_timer;
} ;



/* 主设备号                                                                 */
static int major = 0;
static struct class *led_class;
static int led_pin=0;

static int led_open(struct inode*node,struct file *filpp)
{
	int err;
	printk("led_pin=%d\n",led_pin);
	err=gpio_request(led_pin, "led0");//申请GPIO引脚
	if (err < 0) {
		printk("can not request gpio \n");//返回<0，表示申请失败，说明这个io脚被使用了
		//return -ENODEV;
	}
	gpio_direction_output(led_pin, 1);//设置io口方向为输出，输出1，高电平
	return 0;
}
#if 0
/* 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t led_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	char temp_buf[2];
	int err;
	if(size!=2)//输入出错判断
		return -ENODEV;
	int count = sizeof(gpios)/sizeof(gpios[0]);
	err=copy_from_user(temp_buf,buf,1);//应用程序先告诉驱动要读哪个led
	if(temp_buf[0]>count)//输入出错判断
		return -ENODEV;
	temp_buf[1]=gpio_get_value(gpios[temp_buf[0]].gpio);//读出这个led的电平
	err=copy_to_user(buf, temp_buf, 2);//返回数据到应用程序
	return 2;
}
#endif
static ssize_t led_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    unsigned char ker_buf[2];
    int err;

    if (size != 2)
        return -EINVAL;

    err = copy_from_user(ker_buf, buf, size);
  

    gpio_set_value(ker_buf[0], ker_buf[1]);
    return 2;    
}



/* 定义自己的file_operations结构体                                              */
static struct file_operations file_op_led = {
	.owner	 = THIS_MODULE,
	.open    = led_open,
	//.read    = led_read,
	.write   = led_write,
};


static int led_probe(struct platform_device*pdev)
{
	struct resource *res;
	//res=platform_get_resource(pdev,IORESOURCE_MEM,0);
	//led_pin=res->start;
	of_property_read_u32(pdev->dev.of_node, "pin", &led_pin);
	printk("led_pin=%d\n",led_pin);
	printk("led probe,found led\n");

	major=register_chrdev(0, "myled", &file_op_led);
	led_class=class_create(THIS_MODULE,"myled");
	device_create(led_class,NULL,MKDEV(major,0),NULL,"led");
	return 0;
}

static int led_remove(struct platform_device*pdev)
{
	unregister_chrdev(major, "myled");
	device_destroy(led_class, MKDEV(major, 0));
	class_destroy(led_class);
	return 0;
}
static const struct of_device_id of_match_leds[] = {
	{ .compatible = "led_test", .data = NULL },
	{	}
};
struct platform_driver led_drv={
	.probe  =  led_probe,
	.remove =  led_remove,
	.driver={
		.name="myled",
		.of_match_table=of_match_leds,
	},

};


/* 在入口函数 */
static int __init myled_drv_init(void)
{
    platform_driver_register(&led_drv);
	return 0;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 */
static void __exit myled_drv_exit(void)
{
 platform_driver_unregister(&led_drv);
}
/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(myled_drv_init);
module_exit(myled_drv_exit);

MODULE_LICENSE("GPL");


