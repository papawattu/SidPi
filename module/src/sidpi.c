/*
 *  sidpi.c: MOS 6581 SID driver for Raspberry Pi
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/kfifo.h>
#include <linux/wait.h>
#include <linux/slab.h>
//#include <arch/arm64/kernel/cpuinfo.h>

#include <asm/uaccess.h>   //used for copy_from_user
#include <asm/atomic.h>    //used for atomic operations


#include "sidpi.h"

#define CLASS_NAME "sid"
#define PROC_FS_NAME "sidpi"
#define SIDPI_VERSION 1
#define SUCCESS 0
#define DEVICE_NAME "sid"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */
#define MAJOR_NUM 60
#define SID_FIFO_SIZE 8192

extern unsigned int system_rev;

static volatile __u32 * gpio, * gpio_clock, * gpio_timer;

static unsigned char gpioToGPFSEL [] =
{
  0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,
} ;

static unsigned char gpioToGPLEV [] =
{
  13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
  14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
} ;
// gpioToShift
//        Define the shift up for the 3 bits per pin in each GPFSEL port

static unsigned char gpioToShift [] =
{
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
  0,3,6,9,12,15,18,21,24,27,
} ;

const int DATA[] = {11,9,10,22,27,17,3,2};
const int ADDR[] = {23,24,25,8,7};

unsigned long dataPins[256];
unsigned long addrPins[32];

struct SidDevice
{
  struct cdev         cdev;
  struct task_struct* pThread;
 // struct kfifo*       pFifo;
  struct semaphore    lockMutex;
  struct semaphore    lockWriter;
  char*               pcFifoBuffer;
  struct tty_driver*  pTTYDriver;
};

///Queue for reader
DECLARE_WAIT_QUEUE_HEAD(queueReader);
///Queue for writer
DECLARE_WAIT_QUEUE_HEAD(queueWriter);


// Globals
struct SidDevice g_sidDevice;
static atomic_t g_atomUnprocessedData = ATOMIC_INIT(0);
int piVersion;

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

/* Device variables */
static struct class* sid_class = NULL;
static struct device* sid_device = NULL;
static int sid_major;
/* A mutex will ensure that only one process accesses our device */
static DEFINE_MUTEX(sid_device_mutex);
/* Use a Kernel FIFO for read operations */
static DECLARE_KFIFO(sid_msg_fifo, unsigned char, SID_FIFO_SIZE);
/* This table keeps track of each message length in the FIFO */

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


static void writeData(struct SidDevice* pDevice, char* pcBuffer, int iSize)
{
  char acChar[2] = { 0, 0};
  int i          = 0;
  int iRetval    = 0;
  
  //    printk(KERN_NOTICE "Write to Sid FIFO");
  while(i < iSize)
  { 
    //1. Check if there is space available to write
    iRetval = wait_event_interruptible(queueWriter, 
                                       (!kfifo_is_full(&sid_msg_fifo)));

    //2. get fifo lock
    if (iRetval == 0) // && down_interruptible(&pDevice->lockMutex) == 0)
    {
      //printk(KERN_NOTICE ".%d", pDevice->pFifo->size - pDevice->pFifo->in + pDevice->pFifo->out);

      //3. Send data and release fifo lock
      while(i < iSize && !kfifo_is_full(&sid_msg_fifo))
      {
        acChar[0] = pcBuffer[i];
       kfifo_in(&sid_msg_fifo, acChar, 1);
        i++;
      }
      //up(&pDevice->lockMutex);

      //4. wake the reader
      wake_up_interruptible(&queueReader);
    }

    //Error
    else
    {
      //Just to be sure the unprocessed data equals the 
      // number of bytes when we entered the function
      atomic_sub(iSize -i, &g_atomUnprocessedData);

      i = iSize;
      printk(KERN_NOTICE "Proceeding interrupted (2)\n");
    }
  }
}


static int sid_proc_show(struct file *m,char *buf,size_t count,loff_t *offp ) {
  seq_printf(m, "SIDPi module version 0.1 by Jamie Nuttall\n");
  seq_printf(m, "Interface : %s\n",(sidPiInterfaceType==SIDPI_PARALLEL_INTERFACE?"Parallel":"Serial"));
  seq_printf(m, "Pi Type is : %s\n", (piVersion==1?"Pi2":"Pi1"));

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
void writeSidPar(int reg, int val) {

	iowrite32((unsigned long) addrPins[reg % 32],(u32 *) gpio + 7);
	iowrite32((unsigned long) ~addrPins[reg % 32] & addrPins[31], (u32 *) gpio + 10);
    iowrite32((unsigned long) dataPins[val % 256], (u32 *) gpio + 7);
	iowrite32((unsigned long) ~dataPins[val % 256] & dataPins[255], (u32 *) gpio + 10);
	iowrite32((unsigned long) 1 << CS, (u32 *) gpio + 10);
	udelay(1);
	iowrite32((unsigned long) 1 << CS, (u32 *) gpio + 7);

}
void generatePinTables(void) {
	int i;

	for (i = 0; i < 256; i++) {
		dataPins[i] = (unsigned long) (i & 1) << DATA[0];
		dataPins[i] |= (unsigned long) ((i & 2) >> 1) << DATA[1];
		dataPins[i] |= (unsigned long) ((i & 4) >> 2) << DATA[2];
		dataPins[i] |= (unsigned long) ((i & 8) >> 3) << DATA[3];
		dataPins[i] |= (unsigned long) ((i & 16) >> 4) << DATA[4];
		dataPins[i] |= (unsigned long) ((i & 32) >> 5) << DATA[5];
		dataPins[i] |= (unsigned long) ((i & 64) >> 6) << DATA[6];
		dataPins[i] |= (unsigned long) ((i & 128) >> 7) << DATA[7];
	}

	for (i = 0; i < 32; i++) {
		addrPins[i] = (unsigned long) (i & 1) << ADDR[0];
		addrPins[i] |= (unsigned long) ((i & 2) >> 1) << ADDR[1];
		addrPins[i] |= (unsigned long) ((i & 4) >> 2) << ADDR[2];
		addrPins[i] |= (unsigned long) ((i & 8) >> 3) << ADDR[3];
		addrPins[i] |= (unsigned long) ((i & 16) >> 4) << ADDR[4];

	}
}
void setPinsToOutput2(void) {

	int i, fSel, shift;

	for (i = 0; i < 8; i++) {
		fSel = gpioToGPFSEL[DATA[i]];
		shift = gpioToShift[DATA[i]];
		iowrite32(ioread32((u32 *) gpio + fSel) & ~(7 << shift)
				| (1 << shift),(u32 *)gpio + fSel);
	}
	for (i = 0; i < 5; i++) {
		fSel = gpioToGPFSEL[ADDR[i]];
		shift = gpioToShift[ADDR[i]];
		iowrite32(ioread32((u32 *) gpio + fSel) & ~(7 << shift)
						| (1 << shift),(u32 *) gpio + fSel);
	}
	fSel = gpioToGPFSEL[CS];
	shift = gpioToShift[CS];
	iowrite32(ioread32((u32 *) gpio + fSel) & ~(7 << shift)
					| (1 << shift),(u32 *) gpio + fSel);
	fSel = gpioToGPFSEL[RW];
	shift = gpioToShift[RW];
	iowrite32(ioread32((u32 *) gpio + fSel) & ~(7 << shift)
					| (1 << shift),(u32 *) gpio + fSel);

	fSel = gpioToGPFSEL[RES];
	shift = gpioToShift[RES];
	iowrite32(ioread32((u32 *) gpio + fSel) & ~(7 << shift)
					| (1 << shift),(u32 *) gpio + fSel);
	fSel = gpioToGPFSEL[CLK];
	shift = gpioToShift[CLK];
	iowrite32(ioread32((u32 *) gpio + fSel) & ~(7 << shift)
					| (4 << shift),(u32 *) gpio + fSel);
}

void startSidClk(int freq) {
	int divi, divr, divf;

	divi = 19200000 / freq;
	divr = 19200000 % freq;
	divf = (int) (divr * 4096 / 19200000);

	if (divi > 4095)
		divi = 4095;
	iowrite32(BCM_PASSWORD | GPIO_CLOCK_SOURCE, (u32 *) gpio_clock + 28);

	while ((ioread32(gpio_clock + 28) & 0x80) != 0)
		;

	iowrite32(BCM_PASSWORD | (divi << 12) | divf,(u32 *) gpio_clock + 29);
	iowrite32(BCM_PASSWORD | 0x10 | GPIO_CLOCK_SOURCE,(u32 *) gpio_clock + 28);

	iowrite32(0x0000280,(u32 *) gpio_timer + TIMER_CONTROL);
	iowrite32(0x00000F9,(u32 *) gpio_timer + TIMER_PRE_DIV);
}

static int sidThread2(void* pData)
{
  struct SidDevice* pDev  = (struct SidDevice*)pData;
  int iRetval = 0;
  unsigned int cycles = 0;
  
  if (pDev != NULL)
  {
    printk(KERN_INFO "Thread started...\n");

    set_current_state(TASK_INTERRUPTIBLE);
    while(!kthread_should_stop())
    {
      //1. check if there is data to write
      set_current_state(TASK_INTERRUPTIBLE);
      iRetval = wait_event_interruptible(queueReader, (!kfifo_is_empty(&sid_msg_fifo) || kthread_should_stop()));

      //2. check error and try to get the fifo lock
      set_current_state(TASK_INTERRUPTIBLE);
      if (iRetval == 0) // && !kthread_should_stop() && down_interruptible(&pDev->lockMutex) == 0)
      {
        //3. Read data and release the lock
        unsigned char acBuffer[4];
        kfifo_out(&sid_msg_fifo, acBuffer, 4);
        //printk(KERN_NOTICE "R%02X V%02X CH%02X CL%02X\n",acBuffer[0],acBuffer[1],acBuffer[2],acBuffer[3]);
        //udelay(acBuffer[3] << 8 | acBuffer[2]);
        cycles = acBuffer[3] << 8 | acBuffer[2],acBuffer[3] << 8 | acBuffer[2];
        //if(cycles < 1000) {
        //    udelay(cycles);
        //} else {
            usleep_range(cycles,cycles);    
        //}
        
        if(acBuffer[1] < 32) writeSidPar(acBuffer[1],acBuffer[0]);
        //up(&pDev->lockMutex);

        //4. wake up all writers
        wake_up_interruptible(&queueWriter);

        //5. send character as morse code to keyboard
        atomic_dec(&g_atomUnprocessedData);
      } //(down_interruptible(&pDev->lockMutex) == 0)
    }
    __set_current_state(TASK_RUNNING);
    printk(KERN_INFO "Thread ended.\n");
  } //(pDev != NULL)

  return 0;
}

/*
 * This function is called when the module is loaded
 */
static int __init _sid_init_module(void)
{
    int retval;
    unsigned long mem,peri_base;
	pr_debug("Init sidpi module\n");
	printk(KERN_INFO SIDPILOG "Module version %d by Jamie Nuttall\n",SIDPI_VERSION);
	proc_create(PROC_FS_NAME, 0, NULL, &sid_proc_fops);
	
	/* First, see if we can dynamically allocate a major for our device */
	sid_major = register_chrdev(0, DEVICE_NAME, &fops);
	if (sid_major < 0) {
		pr_err("failed to register device: error %d\n", sid_major);
		retval = sid_major;
		goto failed_chrdevreg;
	}

	/* We can either tie our device to a bus (existing, or one that we create)
	 * or use a "virtual" device class. For this example, we choose the latter */
	sid_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(sid_class)) {
		pr_err("failed to register device class '%s'\n", CLASS_NAME);
		retval = PTR_ERR(sid_class);
		goto failed_classreg;
	}

	/* With a class, the easiest way to instantiate a device is to call device_create() */
	sid_device = device_create(sid_class, NULL, MKDEV(sid_major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(sid_device)) {
		pr_err("failed to create device '%s_%s'\n", CLASS_NAME, DEVICE_NAME);
		retval = PTR_ERR(sid_device);
		goto failed_devreg;
	}

    printk(KERN_INFO SIDPILOG "sid device created Major %d Minor %d\n",MAJOR(dev_no),MINOR(dev_no));
    
	mutex_init(&sid_device_mutex);
	/* This device uses a Kernel FIFO for its read operation */
	  atomic_set(&g_atomUnprocessedData, 0);

      INIT_KFIFO(sid_msg_fifo);
      
      // *g_sidDevice.pFifo = sid_msg_fifo;
      //kfifo_init(g_sidDevice.pFifo,&g_sidDevice.pcFifoBuffer, SID_FIFO_SIZE);
      init_waitqueue_head(&queueReader);
      init_waitqueue_head(&queueWriter);
      sema_init(&g_sidDevice.lockMutex,       1);
      sema_init(&g_sidDevice.lockWriter,      1);
      
      
      piVersion = ((system_rev & 0xFFFF) > 0x15?1:0);
      
      if(piVersion == 0) {
          peri_base = BCM2708_PERI_BASE_1;
      } else {
          peri_base = BCM2708_PERI_BASE_2;
      }
      
      printk(KERN_INFO "GPIO BASE %X", GPIO_BASE + peri_base);
      mem = request_mem_region(GPIO_BASE + peri_base, 4096, "gpio");
	  gpio = ioremap(GPIO_BASE + peri_base, 4096);
      printk(KERN_INFO "GPIO MAPPPED OK %X\n",gpio);
      
      mem = request_mem_region(GPIO_CLOCK + peri_base, 32, "gpioclk");
	  gpio_clock = ioremap(GPIO_CLOCK + peri_base, 32);
	  printk(KERN_INFO "CLOCK MAPPPED OK %X\n",gpio_clock);
      
      mem = request_mem_region(GPIO_TIMER + peri_base, 256, "gpiotimer");
	  gpio_timer = ioremap(GPIO_TIMER + peri_base, 256); 
      printk(KERN_INFO "TIMER MAPPPED OK %X\n",gpio_timer);
      
      setPinsToOutput2();
      generatePinTables();
      startSidClk(DEFAULT_SID_SPEED_HZ);

      g_sidDevice.pThread = kthread_run(sidThread2, &g_sidDevice, "sid_thread");
      if (IS_ERR(g_sidDevice.pThread))
      {
        printk(KERN_WARNING "Error creating the sidthread\n");
        return -EAGAIN;
      }
      
	return SUCCESS;

failed_devreg:
	class_destroy(sid_class);
failed_classreg:
	unregister_chrdev(sid_major, DEVICE_NAME);
failed_chrdevreg:
	return -EIO;

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
    kthread_stop(g_sidDevice.pThread);
  	iounmap(gpio);
	iounmap(gpio_clock);
	iounmap(gpio_timer);

    device_destroy(sid_class, MKDEV(sid_major, 0));
	class_destroy(sid_class);
	unregister_chrdev(sid_major, DEVICE_NAME);
    remove_proc_entry(PROC_FS_NAME, NULL);

}
static int device_open(struct inode *inode, struct file *file) {
	/*
	 * We don't want to talk to two processes at the same time
	 */
	struct SidDevice* pDevice;

    printk(KERN_NOTICE "open() - device opened\n");
    pDevice             = container_of(inode->i_cdev, struct SidDevice, cdev);
    file->private_data = pDevice;
    return 0;
}
static int device_release(struct inode *inode, struct file *file) {

	Sid *sid = file->private_data;

	pr_debug("Closing sid\n");

	//closeSid(sid);
    mutex_unlock(&sid_device_mutex);
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
ssize_t device_write(struct file* pFile, const char __user* pcUserData, size_t iSize, loff_t* piPos)
{
  int iRetval = -EFAULT;
  struct SidDevice* pDevice = (struct SidDevice*)pFile->private_data;

  if (pDevice == NULL)
  {
    iRetval = -EPIPE;
    printk(KERN_NOTICE "write() - No device found\n");
  }
  else if (iSize > 0)
  { 
    unsigned char* pcBuffer = kmalloc((iSize+1)*sizeof(unsigned char), GFP_KERNEL);
    if (pcBuffer != NULL)
    {
      memset(pcBuffer, 0, iSize*sizeof(unsigned char));
      if (copy_from_user(pcBuffer, pcUserData, iSize) == 0)
      {
        pcBuffer[iSize] = 0x00;
        iRetval = -ERESTARTSYS;

        //////////////////////////////////////////////////////// 
        atomic_add(iSize, &g_atomUnprocessedData);
        //printk(KERN_NOTICE "waiting... %d\n",&pDevice->lockWriter);

        if (down_interruptible(&g_sidDevice.lockWriter) == 0)
        {
      //    printk(KERN_NOTICE "mo 0-2: write() - Proceed with data: %s (Size=%d)\n", pcBuffer, iSize);
          writeData(&g_sidDevice, pcBuffer, iSize);
          up(&g_sidDevice.lockWriter);
          iRetval = iSize;
        } //down_interruptible(&pDevice->lockWriter) == 0
        else
        {
          //Could not aquire the write lock (interrupted)
          // -> decrement the unprocessed data size
          atomic_sub(iSize, &g_atomUnprocessedData);
          printk(KERN_NOTICE "write() - Proceeding interrupted (1)\n");
        } 
        //////////////////////////////////////////////////////// 

      } //(copy_from_user(pcBuffer, pcUserData, iSize) == 0)
      else
      {
        printk(KERN_NOTICE "write() - Could not copy from user\n");
        iRetval = -EFAULT;
      }
      kfree(pcBuffer);
    } //(pcBuffer != NULL)
    else
    {
      iRetval = -ENOMEM;
    }
  }
  else 
  {
    printk(KERN_NOTICE "write() - Empty string passed\n");
  }
  return iRetval;

}

static int sid_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	struct SidDevice *sid = file->private_data;
	if(!sid) return -EIO;

	switch(cmd)
    {
        case SID_IOCTL_RESET:
        {
        	printk(KERN_INFO SIDPILOG "Reset or flush request %u\n",cmd);
        	//sid = reqSidReset(sid);
            kfifo_reset(&sid_msg_fifo);
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
        		//printk(KERN_INFO SIDPILOG "Delay %x %x %x %x",arg[0],arg[1],arg[2],arg[3]);
			unsigned char buf[4];
			buf[0] = 0;
			buf[1] = 255;
			buf[2] = arg & 0xff;
			buf[3] = ((arg & 0xff00) >> 8);
			writeData(&g_sidDevice, &buf, 4);
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
