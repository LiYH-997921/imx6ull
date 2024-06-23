#include "linux/printk.h"
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/backing-dev.h>
#include <linux/shmem_fs.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/uio.h>

#include <linux/uaccess.h>
#include <linux/module.h>
static char hello_buf[100]={0};
static struct class *hello_class;

//实现open函数
static int hello_open(struct inode *node, struct file *file)
{
    printk("%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); //注意这里是用的printk，而不是printf函数
    return 0;
}
//实现read函数
static ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    unsigned int len=size>100?100:size;
    printk("%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); //注意这里是用的printk，而不是printf函数
    copy_to_user(buf,hello_buf,len); //内核拷贝数据到应用程序
    return len;
}
//实现hello_write函数
static ssize_t hello_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    unsigned int len=size>100?100:size;
    printk("%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); //注意这里是用的printk，而不是printf函数
    copy_from_user(hello_buf,buf,len);//应用程序拷贝数据到内核
    return len;
}
//实现relesse函数，应用程序关闭的时候会调用这个release函数
static int hello_release (struct inode *node, struct file *file)
{
    printk("%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); //注意这里是用的printk，而不是printf函数
    return 0;
}
//1.创建一个file_operations结构体
static const struct file_operations hello_dev = {
	.owner	    = THIS_MODULE,
	.read		= hello_read,
	.write		= hello_write,
	.open		= hello_open,
    .release    = hello_release,
};
//static int major=0;//记录主设备号，卸载驱动程序的时候会用到，不需要卸载驱动可以不用管
static struct cdev hello_cdev;
static dev_t dev;
//入口函数,//注册字符设备驱动
static int hello_init(void)
{
    #if 0
    //0:表示让这个函数遍历注册驱动表中的数组，重第0项查找，找到一个未使用的主设备号，
    //然后将file_opration结构体放到这个位置，并返回这个位置的下标（即主设备号）(即内核里面第几个设备)
    
    //100ask_hello：设备名字，
    //hello_dev:file_oparetion结构体
    major=register_chrdev(0, "100ask_hello",&hello_dev);
    #else
    int ret=0;
    //dev结构体将会获取到一个未使用到的主设备号和一个0设备号（0，1）表示从0开始，只有一个
    //
    ret = alloc_chrdev_region(&dev, 0, 2, "hello");
	//major = MAJOR(dev);//从这里获取到主设备号
	if (ret < 0) {
		printk(KERN_ERR "alloc_chrdev_region() failed for bsr\n");
		return -EINVAL;//返回无效码
	}
    cdev_init(&hello_cdev,&hello_dev);//hello_dev:file_oparetion结构体
    ret=cdev_add(&hello_cdev,dev,2);//1:表示只有一个次设备号
    if(ret){
        printk(KERN_ERR "cdev_add() failed for bsr\n");
		return -EINVAL;//返回无效码
    }
    #endif
    //创建设备节点1.创建一个类2.创建一个设备
    //创建一个类，传入类名
    hello_class = class_create(THIS_MODULE, "hello_class");
	if (IS_ERR(hello_class)){
        pr_err("class_create faild\n");//
		return PTR_ERR(hello_class);
    }
    //创建一个设备
    //传入类
    //传入主设备号，和次设备号0（这个可以看实际情况，只要不重复就行），
    //传入设备名
    #if 0
    device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello");//系统就创建了一个/dev/hello这个设备节点了
    #else
    device_create(hello_class, NULL, dev, NULL, "hello");
    #endif
    return 0;
}
//退出函数（卸载函数）,//注销字符设备驱动
static void  hello_exit(void)
{
    #if 0
    //0:主设备号，
    //100ask_hello：设备名字，
    unregister_chrdev(major,"100ask_hello");
    //销毁设备和销毁类
    device_destroy(hello_class,MKDEV(major,0));
    #else
    device_destroy(hello_class,dev);
    #endif
    class_destroy(hello_class);
#if 1
    cdev_del(&hello_cdev);
    unregister_chrdev_region(dev, 2);//1:只有一个次设备号
#endif

    

 
}
module_init(hello_init);//装载驱动程序的时候，入口函数会被内核调用，入口函数相当于main函数
module_exit(hello_exit);//卸载驱动程序的时候，出口函数会被内核调用
MODULE_LICENSE("GPL");//指定GPL协议，开源协议
