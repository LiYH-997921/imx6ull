#include "asm-generic/errno-base.h"
#include "asm-generic/gpio.h"
#include "linux/jiffies.h"
#include "linux/timekeeping.h"
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
#include <linux/delay.h>
struct gpio_desc{
	int gpio;
	int irq;
    char *name;
    int key;
	struct timer_list key_timer;
} ;

static struct gpio_desc gpios[2] = {
    {131, 0, "gpio_100ask_1", },
};

/* 主设备号                                                                 */
static int major = 0;
static struct class *gpio_class;

/* 环形缓冲区 */
#define BUF_LEN 128
static unsigned char g_keys[BUF_LEN];
static int r, w;

struct fasync_struct *button_fasync;

#define NEXT_POS(x) ((x+1) % BUF_LEN)

static int is_key_buf_empty(void)
{
	return (r == w);
}

static int is_key_buf_full(void)
{
	return (r == NEXT_POS(w));
}

static void put_key(unsigned char key)
{
	if (!is_key_buf_full())
	{
		g_keys[w] = key;
		w = NEXT_POS(w);
	}
}

static unsigned char get_key(void)
{
	unsigned char key = 0;
	if (!is_key_buf_empty())
	{
		key = g_keys[r];
		r = NEXT_POS(r);
	}
	return key;
}

static u64 g_IR_irq_time[68];
static int g_IR_irq_cnt=0;
static DECLARE_WAIT_QUEUE_HEAD(gpio_wait);
extern void parse_dht11_datas(void);
// static void key_timer_expire(struct timer_list *t)
static void key_timer_expire(unsigned long data)
{
	g_IR_irq_cnt=0;
	//1.超时
	put_key(-1);
	put_key(-1);
	//2.唤醒应用程序
	wake_up_interruptible(&gpio_wait);
	kill_fasync(&button_fasync, SIGIO, POLL_IN);
}


void parse_IR_datas(void)
{
	u64 time;
	unsigned char data=0,datas[5]={0},crc=0;
	int bits=0,byte=0;
	int i=0,m=0,n=0;
	/*判断前导码（即开始信号）*/
	time=g_IR_irq_time[1]-g_IR_irq_time[0];
	if(time<8000000 || time>10000000)//如果小于8ms或者大于10ms
	{
		goto err;//出错
	}
	time=g_IR_irq_time[2]-g_IR_irq_time[1];
	if(time<3500000 || time>5500000)//如果小于3.5ms或者大于5.5ms
	{
		goto err;//出错
	}
	/*解析数据*/
	for(i=0;i<32;i++)
	{
		m=3+i*2;
		n=m+1;
		time=g_IR_irq_time[m]-g_IR_irq_time[n];
		data<<=1;
		if(time>1000000)//大于1ms
		{
			data|=1;
		}
		bits++;
		if(bits==8){
			datas[byte]=data;
			byte++;
			data=bits=0;
		}
	}
	/*判断数据的有效性*/
	if((datas[0]!= (unsigned char)~datas[1]) || (datas[2]!= (unsigned char)~datas[3]))//这里取反可能会转成32位的数据，所以要加强制转换
	{
		printk("data verify err\n");
		printk("datas[0]=%02x,datas[1]=%02x,datas[2]=%02x,datas[3]=%02x\n",datas[0],datas[1],datas[2],datas[3]);
		goto err;
	}
	put_key(datas[0]);
	put_key(datas[2]);
	wake_up_interruptible(&gpio_wait);
	kill_fasync(&button_fasync, SIGIO, POLL_IN);
	g_IR_irq_cnt=0;
	return;
err:
	g_IR_irq_cnt;
	put_key(-1);
	put_key(-1);
	//2.唤醒应用程序
	wake_up_interruptible(&gpio_wait);
	kill_fasync(&button_fasync, SIGIO, POLL_IN);

}

static irqreturn_t IR_isr(int irq, void *dev_id)
{
	struct gpio_desc *gpio_desc = dev_id;
	u64 time;
	//1.记录中断发生的时间
	time=ktime_get_ns();
	g_IR_irq_time[g_IR_irq_cnt]=time;
	//2.累计计数
	g_IR_irq_cnt++;
	//3.次数足够：删除定时器，解析数据，放入环形buffer，唤醒APP
	if(g_IR_irq_cnt==68){
		del_timer(&gpios[0].key_timer);//删除定时器
		parse_IR_datas();
		g_IR_irq_cnt=0;
	}
	//4.启动定时器
	mod_timer(&gpio_desc->key_timer,jiffies+msecs_to_jiffies(100));
	
	return IRQ_HANDLED;
}
/* 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t IR_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err;
	unsigned char kern_buf[2];
	if(size!=2)
		return -EINVAL;
	if (is_key_buf_empty() && (file->f_flags & O_NONBLOCK))
		return -EAGAIN;
	
	wait_event_interruptible(gpio_wait, !is_key_buf_empty());
	kern_buf[0] = get_key();/*device*/
	kern_buf[1] = get_key();/*data*/
	printk("kern_buf[0]=%d,kern_buf[1]=%d\r\n",kern_buf[0],kern_buf[1]);
	if(kern_buf[0]==(char)-1 && kern_buf[1]==(char)-1){
		return -EIO;
	}
	err = copy_to_user(buf, kern_buf, 2);

	return 2;
}

static int IR_release (struct inode *inode, struct file *file)
{
	
	return 0;
}



/* 定义自己的file_operations结构体 */
static struct file_operations gpio_key_drv = {
	.owner	 = THIS_MODULE,
	.read    = IR_read,
	.release = IR_release,

};





/* 在入口函数 */
static int __init IR_init(void)
{
    int err;
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	for (i = 0; i < count; i++)
	{		
		gpios[i].irq  = gpio_to_irq(gpios[i].gpio);

		setup_timer(&gpios[i].key_timer, key_timer_expire, (unsigned long)&gpios[i]);
	 	//timer_setup(&gpios[i].key_timer, key_timer_expire, 0);
		//gpios[i].key_timer.expires = ~0;
		//add_timer(&gpios[i].key_timer);
		err = request_irq(gpios[i].irq, IR_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, gpios[i].name, &gpios[i]);
	}

	/* 注册file_operations 	*/
	major = register_chrdev(0, "100ask_IR", &gpio_key_drv);  /* /dev/gpio_desc */

	gpio_class = class_create(THIS_MODULE, "100ask_IR_class");
	if (IS_ERR(gpio_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "100ask_IR");
		return PTR_ERR(gpio_class);
	}

	device_create(gpio_class, NULL, MKDEV(major, 0), NULL, "100ask_IR"); /* /dev/100ask_gpio */
	
	return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 */
static void __exit IR_exit(void)
{
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	device_destroy(gpio_class, MKDEV(major, 0));
	class_destroy(gpio_class);
	unregister_chrdev(major, "100ask_IR");

	for (i = 0; i < count; i++)
	{
		free_irq(gpios[i].irq, &gpios[i]);
		del_timer(&gpios[i].key_timer);
	}
}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(IR_init);
module_exit(IR_exit);

MODULE_LICENSE("GPL");


