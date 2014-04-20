/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h> 
#include <linux/delay.h>
//#include <asm/uaccess.h>	/* for put_user */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include "sidpithread.h"

#define PROC_FS_NAME "sidpi"

/**
 * This structure hold information about the /proc file
 *
 */
struct proc_dir_entry *Our_Proc_File;
struct cdev sid_dev;
/*  
 *  Prototypes - this would normally go in a .h file
 */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "sid"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */
#define MAJOR_NUM 60

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major; /* Major number assigned to our device driver */
int dev_handle;
static int Device_Open = 0; /* Is device open?
 * Used to prevent multiple access to device */
static char msg[BUF_LEN]; /* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = { .read = device_read, .write =
		device_write, .open = device_open, .release = device_release };


static int sid_proc_show(struct file *m,char *buf,size_t count,loff_t *offp ) {
  seq_printf(m, "SIDPi version 0.1\n");
  seq_printf(m, "Buffer size : %d\n",getBufferMax());
  seq_printf(m, "Buffer count : %d\n",getBufferCount());
  seq_printf(m, "Buffer first pointer : %d\n",getBufferFirst());
  seq_printf(m, "Buffer last pointer : %d\n",getBufferLast());
  seq_printf(m, "Buffer full : %d\n",getBufferFull());
  seq_printf(m, "Real clock : %lu\n",getRealSidClock());
  seq_printf(m, "Sid clock : %lu\n",getSidClock());


  return count;
}
static int sid_proc_open(struct inode *inode, struct  file *file) {
  return single_open(file, sid_proc_show, NULL);
}
static const struct file_operations sid_proc_fops = {
  .owner = THIS_MODULE,
  .open = sid_proc_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};
/*
 * This function is called when the module is loaded
 */
static int __init _sid_init_module(void)
{
	devno = MKDEV(0,0);
	alloc_chrdev_region(&devno,0,1,DEVICE_NAME);
	//cdev_init(&sid_dev, &fops);
	dev_handle = cdev_add(&sid_dev, devno, 1);

	if (dev_handle < 0) {
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return dev_handle;
	}
	proc_create(PROC_FS_NAME, 0, NULL, &sid_proc_fops);
	setupSid();

	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
static void __exit _sid_cleanup_module(void)
{
	/* 
	 * Unregister the device 
	 */
	closeSid();
	cdev_del(&sid_dev);
	remove_proc_entry(PROC_FS_NAME, NULL);

}
static int device_open(struct inode *inode, struct file *file) {
	/*
	 * We don't want to talk to two processes at the same time
	 */
	if (Device_Open)
		return -EBUSY;

	Device_Open++;

	try_module_get(THIS_MODULE);
	return SUCCESS;
}
static int device_release(struct inode *inode, struct file *file) {
#ifdef DEBUG
	printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif

	/*
	 * We're now ready for our next caller
	 */
	Device_Open--;

	module_put(THIS_MODULE);
	return SUCCESS;
}

/*
 * This function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h   */
		char __user * buffer, /* buffer to be
		 * filled with data */
		size_t length, /* length of the buffer     */
		loff_t * offset)
{
	return 0;
}
/*
 *
 * This function is called when somebody tries to
 * write into our device file.
 */
static ssize_t device_write(struct file *file,
		const char __user * buffer, size_t length, loff_t * offset)
{
	unsigned int cycles,reg,val;
	struct timeval ts,now;
	//printk(KERN_INFO "%x %x %x %x length %d\n", buffer[0],buffer[1],buffer[2],buffer[3],length);

	/*while(getBufferFull()) {
		mdelay(100);
	} */


	cycles = (buffer[3] << 8 | buffer[2]) & 0xffff;
	reg = buffer[1];
	val = buffer[0];

	//printk("Sid write - reg %x - val %x - delay %x\n",reg,val,cycles);
	sidWrite(reg, val,cycles);
	udelay(10);
	return length;
}
module_init( _sid_init_module);
module_exit( _sid_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lakshmanan");
MODULE_DESCRIPTION("A Simple Hello World module");
