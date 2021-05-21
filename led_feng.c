#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#define RED_BASE 0xC001A000
#define GONGNENG_BASE 0xC001C000

unsigned int *red_base = NULL;
unsigned int *feng_base = NULL;

#define CNAME "ledfeng"
int major = 0;
int majord;
struct class *cls;
struct device *dev;
char kbuf[4] = {0};
int mycdev_open(struct inode *inode, struct file *file)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}
ssize_t mycdev_read(struct file *file, char __user *ubuf,
	size_t size, loff_t *offs)
{
	int ret;
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	if(size > sizeof(kbuf)) size = sizeof(kbuf);

	ret = copy_to_user(ubuf,kbuf,size);
	if(ret){
		printk("copy data to user error\n");
		return -EINVAL;
	}
	
	return size;

}
ssize_t mycdev_write(struct file *file, const char __user *ubuf,
	size_t size, loff_t *offs)
{
	int ret;
	
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);

	if(size > sizeof(kbuf)) size = sizeof(kbuf);

	ret = copy_from_user(kbuf,ubuf,size);
	if(ret){
		printk("copy data from user error\n");
		return -EINVAL;
	}

	if(kbuf[0] == 'a')
	{
		//亮灯
		*red_base |= (1<<28);
		
	}
	else if(kbuf[0] == 'b')
	{
		//灭灯
		*red_base &= ~(1<<28);
	}	
	else if(kbuf[0] == 'c')
	{
		//响
		*feng_base |= (1<<14);
	}
	else if(kbuf[0] == 'd')
	{
		//不响
		*feng_base &= ~(1<<14);
	}
	return size;
}
int mycdev_close(struct inode *inode, struct file *file)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

const struct file_operations fops = {
	.open    = mycdev_open,
	.read    = mycdev_read,
	.write   = mycdev_write,
	.release = mycdev_close,
};

static int __init mycdev_init(void)
{
	//注册字符设备驱动
	majord = register_chrdev(major,CNAME,&fops);
	if(majord < 0)
	{
		printk("register char device error\n");
		return -majord;
	}
	/*
	//创建class
	cls = class_create(THIS_MODULE,CNAME);
	if(IS_ERR(cls))
	{
		printk("class create is fail\n");
		return PTR_ERR(cls);
	}
	//创建device
	dev = device_create(cls,NULL,MKDEV(major,0),NULL,CNAME);
	if(IS_ERR(dev))
	{
		//printk("device create is fail\n");
		return PTR_ERR(dev);
	}
	*/
	
	//建立映射
	red_base = ioremap(RED_BASE,36);
	if(red_base == NULL)
	{
		printk("red ioremap error\n");
		return -ENOMEM;
	}
	feng_base = ioremap(GONGNENG_BASE,20);
	if(feng_base == NULL)
	{
		//printk("GONGNENG_base ioremap error\n");
		return -ENOMEM;
	}
	*red_base &= ~(1<<28);
	*(red_base+1) |= (1<<28);
	*(red_base+9) &= ~(3<<24);

	*feng_base &= ~(1<<14);
    *(feng_base+1) |= (1<<14);
    *(feng_base+8) &= ~(3 << 28);
    *(feng_base+8) |= (1 << 28);
	
	return 0;
}

static void __exit mycdev_exit(void)
{
	iounmap(red_base);
	iounmap(feng_base);
	//注销字符设备驱动
	unregister_chrdev(major,CNAME);
}
module_init(mycdev_init);
module_exit(mycdev_exit);
MODULE_LICENSE("GPL");








