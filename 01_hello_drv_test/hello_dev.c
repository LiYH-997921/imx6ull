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
//实现open函数
static int hello_open(struct inode *node, struct file *file)
{
    printk("%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); //注意这里是用的printk，而不是printf函数
    return 0;
}
//实现read函数
static ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    printk("%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); //注意这里是用的printk，而不是printf函数
    return size;
}
//实现hello_write函数
static ssize_t hello_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    printk("%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); //注意这里是用的printk，而不是printf函数
    return size;
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
static int maior=0;//记录主设备号，卸载驱动程序的时候会用到，不需要卸载驱动可以不用管
//入口函数,//注册字符设备驱动
static int hello_init(void)
{
    //0:表示让这个函数遍历注册驱动表中的数组，重第0项查找，找到一个未使用的主设备号，
    //然后将file_opration结构体放到这个位置，并返回这个位置的下标（即主设备号）(即内核里面第几个设备)
    
    //100ask_hello：设备名字，
    //hello_dev:file_oparetion结构体
    maior=register_chrdev(0, "100ask_hello",&hello_dev);
    return 0;
}
//退出函数（卸载函数）,//注销字符设备驱动
static void  hello_exit(void)
{
    //0:主设备号，
    //100ask_hello：设备名字，
    unregister_chrdev(maior,"100ask_hello");
 
}
module_init(hello_init);//装载驱动程序的时候，入口函数会被内核调用，入口函数相当于main函数
module_exit(hello_exit);//卸载驱动程序的时候，出口函数会被内核调用
MODULE_LICENSE("GPL");//指定GPL协议，开源协议
