/*
 *  sidpi.c: MOS 6581 SID driver for Raspberry Pi
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>	/* for put_user */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include "sidpithread.h"

#define PROC_FS_NAME "sidpi"
#define SIDPI_VERSION 1

/**
 * This structure hold information about the /proc file
 *
 */
struct proc_dir_entry *Our_Proc_File;
struct cdev * sid_dev;
/*
 *  Prototypes - this would normally go in a .h file
 */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static int sid_ioctl(struct file *, unsigned int ,unsigned long );


#define SUCCESS 0
#define DEVICE_NAME "sid"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */
#define MAJOR_NUM 60

/*
 * Global variables are declared as static, so are global within the file.
 */

static int Major; /* Major number assigned to our device driver */
int dev_no;
dev_t dev_handle;

static char msg[BUF_LEN]; /* The msg the device will give when asked */
static char *msg_Ptr;
static int sidPiInterfaceType = SIDPI_PARALLEL_INTERFACE;
static int piType = 0; //default to Pi1

static struct file_operations fops = {
		.owner   = THIS_MODULE,
		.read = device_read,
		.write = device_write,
		.unlocked_ioctl = sid_ioctl,
		.open = device_open,
		.release = device_release,
};


static int sid_proc_show(struct file *m,char *buf,size_t count,loff_t *offp ) {
  seq_printf(m, "SIDPi module version 0.1 by Jamie Nuttall\n");
  seq_printf(m, "Interface : %s\n",(sidPiInterfaceType==SIDPI_PARALLEL_INTERFACE?"Parallel":"Serial"));
  seq_printf(m, "Pi Type is : %s\n",(piType==0?"Pi1":"Pi2"));

  //seq_printf(m, "Buffer size : %d\n",getBufferMax());
  //seq_printf(m, "Buffer count : %d\n",getBufferCount());
  //seq_printf(m, "Buffer first pointer : %d\n",getBufferFirst());
  //seq_printf(m, "Buffer last pointer : %d\n",getBufferLast());
  //seq_printf(m, "Buffer full : %d\n",getBufferFull());

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
	pr_debug("Init sidpi module\n");
	printk(KERN_INFO SIDPILOG "Module version %d by Jamie Nuttall\n",SIDPI_VERSION);
	dev_no = MKDEV(0,0);
	alloc_chrdev_region(&dev_no,0,1,DEVICE_NAME);
	sid_dev = cdev_alloc();
	cdev_init(sid_dev, &fops);
	dev_handle = cdev_add(sid_dev, dev_no, 1);

	if (dev_handle < 0) {
		printk(KERN_ALERT SIDPILOG "Registering char device failed with %d\n", Major);
		return dev_handle;
	}
	proc_create(PROC_FS_NAME, 0, NULL, &sid_proc_fops);
	printk(KERN_INFO SIDPILOG "sid device created Major %d Minor %d\n",MAJOR(dev_no),MINOR(dev_no));
    
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
	pr_debug("Clean up sidpi module\n");

	cdev_del(sid_dev);
	unregister_chrdev_region(dev_no,1);
	remove_proc_entry(PROC_FS_NAME, NULL);

}
static int device_open(struct inode *inode, struct file *file) {
	/*
	 * We don't want to talk to two processes at the same time
	 */
	Sid *sid;

	pr_debug("Opening device\n");

	sid = setupSid(sidPiInterfaceType,piType);
	if(!sid) return -EIO;

	pr_debug("SID setup\n");

	file->private_data = sid;

	return SUCCESS;
}
static int device_release(struct inode *inode, struct file *file) {

	Sid *sid = file->private_data;

	pr_debug("Closing sid\n");

	closeSid(sid);

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
	size_t      bytes;
	__u8        buf[20];
	const char *p = buffer;
	size_t      c = length;
	Sid 		*sid = file->private_data;

	if(!sid) {
		return -EIO;
	}

	bytes = copy_from_user(&buf, p, c);
	//sidWrite(sid,buf[1],buf[0],(buf[3] << 8 | buf[2]) & 0xffff);
    udelay((buf[3] << 8) | buf[2]);
	//file->f_dentry->d_inode->i_mtime = CURRENT_TIME;
	//mark_inode_dirty(file->f_dentry->d_inode);
	return (ssize_t) bytes;

}

static int sid_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	Sid *sid = file->private_data;
	if(!sid) return -EIO;

	switch(cmd)
    {
        case SID_IOCTL_RESET:
        {
        	printk(KERN_INFO SIDPILOG "Reset or flush request %u\n",cmd);
        	//sid = reqSidReset(sid);
        	if(sid) {
        		file->private_data = sid;
        		return OK;
        	}
        	else
        		return -EIO;
        	break;
        }
        case SID_IOCTL_FIFOSIZE:
        {
        	printk(KERN_INFO SIDPILOG "FIFO size request\n");
            return put_user(SID_BUFFER_SIZE, (int*)arg);
        }
        case SID_IOCTL_FIFOFREE:
        {
        	printk(KERN_INFO SIDPILOG "FIFO free request\n");
        	return 0; //put_user(getBufferCount(), (int*)arg);
        }
        case SID_IOCTL_SIDTYPE:
            return 0; // return 6581

        case SID_IOCTL_CARDTYPE:
            return 0;
//========================================================================================================================
        case SID_IOCTL_MUTE:
        {
        	printk(KERN_INFO SIDPILOG "Mute request\n");
        	//writeSid(sid,24,0); //mute
        	break;
        }
        case SID_IOCTL_NOFILTER:
        {
        	printk(KERN_INFO SIDPILOG "No filter not implemented\n");
        	break;
        }
        case SID_IOCTL_FLUSH:
        {
        	return OK;
        }
        case SID_IOCTL_DELAY:
        {
        	//sidDelay(sid,(int) arg);
        	if(arg) {
        		printk(KERN_INFO SIDPILOG "Delay %d",arg);
        	} else {
        		printk(KERN_INFO SIDPILOG "Delay no arg");
        	}
            break;
        }
        case SID_IOCTL_READ:
        {
        	printk(KERN_INFO "sidpi: Read request not implemented yet\n");
            return 0;
        }
        default:
            printk(KERN_ERR "sidpi: unknown ioctl %x\n", cmd);
            break;
    }
    return 0;
}
module_init( _sid_init_module);
module_exit( _sid_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jamie Nuttall");
MODULE_DESCRIPTION("A MOS 6581 SID driver module using the hardsid protocol.");
module_param(sidPiInterfaceType, int, (S_IRUSR | S_IRGRP | S_IROTH));
MODULE_PARM_DESC(sidPiInterfaceType, "Sid Interface type, can be serial (1) or parallel (0)");
module_param(piType, int, (S_IRUSR | S_IRGRP | S_IROTH));
MODULE_PARM_DESC(piType, "Pi version 0 for orginal A,B, B+ or 1 for Pi2");
