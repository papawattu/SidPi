#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/resource.h>
#include <asm/delay.h>
#include <linux/wait.h>
#include <mach/platform.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include "sidpithread.h"

static struct task_struct *thread;

typedef struct Sid {
	struct semaphore bufferSem;
	struct semaphore todoSem;
	struct semaphore wait;
	struct semaphore todoReset;
	unsigned char volatile buffer[SID_BUFFER_SIZE]; /* body of queue */
	unsigned int lastCommand; /* position of first element */
	unsigned int currentCommand; /* position of last element */
	atomic_t todoCount; /* number of queue elements */
	__u64 targetTime;
	atomic_t reset;

} Sid;

Sid * sid;

struct
{
    struct timer_list id;
    atomic_t expired;
} sidTimer;

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

unsigned int bufReadPos, bufWritePos;
unsigned long dataPins[256];
unsigned long addrPins[32];
static volatile __u32 * gpio, * gpio_clock, * gpio_timer;
int isPlaybackReady = 0;
unsigned long lastClock = 0, currentClock = 0;
int threshold = 10, multiplier = 1000;
int timeValid = 0,cycles=0;
struct timeval lasttv;
__s32 busy = 	0;
int sidSetup = 0;

static inline __u64 timerGet (void)
{
	struct timeval tv;
	do_gettimeofday(&tv);
	return (__u64) tv.tv_sec * NSEC_PER_SEC + (tv.tv_usec * 1000);
}

void sidTimerFunc (unsigned long arg)
{

    atomic_set (&sidTimer.expired, 1);
    up(&sid->wait);

  //  printk(KERN_INFO "NEW DELAY %llu\n",timerGet ());
}


void sidInit(Sid *sid) {
	sema_init(&sid->bufferSem, (SID_BUFFER_SIZE >> 2)-4);
	sema_init(&sid->todoReset,0);
	sema_init(&sid->todoSem,0);
	sema_init(&sid->wait,0);

	sid->lastCommand = 0;
	sid->currentCommand = 0;
	sid->targetTime = 0;

	atomic_set(&sid->todoCount,0);
	
	atomic_set(&sid->reset,0);
}

void sidReset(Sid *sid) {
	sema_init(&sid->bufferSem, (SID_BUFFER_SIZE >> 2)-4);
	sema_init(&sid->todoSem,0);
	sema_init(&sid->wait,0);

	sid->lastCommand = 0;
	sid->currentCommand = 0;
	sid->targetTime = 0;

	atomic_set(&sid->todoCount,0);
	
	atomic_set(&sid->reset,0);
	
	up(sid->todoReset);
}

void setupSid(Sid *sid) {

	int i, fSel, shift;

	if(sidSetup) return;

	sid = kmalloc(sizeof(Sid), GFP_KERNEL);

	sidInit(sid);

	timeValid = 0;

	if(mapGPIO() != 0) return;

	generatePinTables();

	setPinsToOutput();

	startSidClk(DEFAULT_SID_SPEED_HZ);
	
	init_timer (&sidTimer.id);
	sidTimer.id.data = 0;
	sidTimer.id.function = &sidTimerFunc;

	startSidThread();

	sidSetup = 1;

}

void closeSid(void) {
	up(&sid->todoSem);
	stopSidThread();
	kfree(sid);
	unmapGPIO();
}

void startSidThread(void) {

	int err;

	thread = kthread_run(sidThread, &err, THREAD_NAME);
	if (IS_ERR(thread)) {
		err = PTR_ERR(thread);
		thread = NULL;
	}	else {
		printk(KERN_INFO "Sid Thread Running...\n");
	}
}

void stopSidThread(void) {
	int ret;
	ret = kthread_stop(thread);
	thread = NULL;
	printk(KERN_INFO "Sid Thread Stopped...\n");
}

int sidThread(void) {
	unsigned char reg, val;
	int cycles,clocks;
	long startClock;
	struct timeval tv,lasttv;

	current->policy=SCHED_FIFO;
	current->rt_priority=1;
	//current->prio = sys_sched_get_priority_max(SCHED_FIFO);
	set_user_nice(current, -5);
	//schedule();
	//init_queue(&buffer);

	busy = -1;

	while (!kthread_should_stop()) {

		if (signal_pending(current))
			break;

		if(atomic_read(&sid->reset) == 1) {
			sidReset(sid);
		}

		down(&sid->todoSem);
		reg = sid->buffer[sid->currentCommand];
		val = sid->buffer[sid->currentCommand+1];
		cycles = sid->buffer[sid->currentCommand+2] | sid->buffer[sid->currentCommand+3] << 8;

		sid->currentCommand = (sid->currentCommand + 4) & (SID_BUFFER_SIZE -1);

		up(&sid->bufferSem);

		busy = ((__u32) cycles * SID_PAL);

		sid->targetTime += busy;

		if(busy > 60000) {
			busy -= 55000;
		}

		if ((unsigned char) reg != 0xff) {
			delay(cycles);
			writeSid(reg, val);
		} else {
			delay(cycles);
		}
	}
	return 0;
}

int playbackReady(void) {
	return isPlaybackReady;
}

void startPlayback(void) {
	isPlaybackReady = 1;
}

void stopPlayback(void) {
	isPlaybackReady = 0;

}
int sidDelay(unsigned int cycles) {

	sid->buffer[sid->lastCommand] = 0xff;
	sid->buffer[sid->lastCommand + 1] = 0x00;
	sid->buffer[sid->lastCommand + 2] = cycles & 0xff;
	sid->buffer[sid->lastCommand + 3] = cycles >> 8;
	sid->lastCommand = (sid->lastCommand + 4) & (SID_BUFFER_SIZE -1);
	down(&sid->bufferSem);
	up(&sid->todoSem);
	return 0;

}
int sidWrite(int reg, int value, unsigned int cycles) {

	sid->buffer[sid->lastCommand] = reg;
	sid->buffer[sid->lastCommand + 1] = value;
	sid->buffer[sid->lastCommand + 2] = cycles & 0xff;
	sid->buffer[sid->lastCommand + 3] = cycles >> 8;
	sid->lastCommand = (sid->lastCommand + 4) & (SID_BUFFER_SIZE -1);
	down(&sid->bufferSem);
	up(&sid->todoSem);
	return 0;
}


void delay(unsigned int howLong) {

	int clocks;
	struct timeval tv;
	unsigned long jnow = jiffies, jdelta;
	__u64 now    = timerGet ();
	__s64 deltal = sid->targetTime - now;
	__s32 delta  = (__u32) deltal;

	if(deltal < 0) {
		if(deltal < (-NSEC_PER_SEC / 5))
			sid->targetTime = now;
		return;
	}

	jdelta = (__u32) delta / (NSEC_PER_SEC / HZ);

	if(!jdelta) {
		busy = sid->targetTime;
	} else {
		//if(atomic_read(&sidTimer.expired) == 0)
		atomic_set (&sidTimer.expired, 0);
		mod_timer (&sidTimer.id, jnow + jdelta);
		if (down_interruptible(&sid->wait))
			return;

		//while(atomic_read(&sidTimer.expired) == 0);
		if(atomic_read(&sidTimer.expired) == 1) {
			for (;;)
			{
				__s32 delta = ((__s32) timerGet() & 0x7fffffffL) - busy;

				if (delta >= 0)
					break;
			}
		}
	}
}

void setThreshold(int value) {
	threshold = value;
}

void setMultiplier(int value) {
	multiplier = value;
}

unsigned long getSidClock(void) {
	return currentClock;
}
unsigned long getRealSidClock(void) {
	unsigned long clock = ioread32(gpio_timer + TIMER_OFFSET);
	return clock;
}

void writeSid(int reg, int val) {


	iowrite32((unsigned long) addrPins[reg % 32],(u32 *) gpio + 7);
	iowrite32((unsigned long) ~addrPins[reg % 32] & addrPins[31], (u32 *) gpio + 10);
	while((int) (*(gpio + gpioToGPLEV [4]) & (1 << (4 & 31))) == 1);
	while((int) (*(gpio + gpioToGPLEV [4]) & (1 << (4 & 31))) == 0);
	iowrite32((unsigned long) dataPins[val % 256], (u32 *) gpio + 7);
	iowrite32((unsigned long) ~dataPins[val % 256] & dataPins[255], (u32 *) gpio + 10);
	iowrite32((unsigned long) 1 << CS, (u32 *) gpio + 10);
	while((*(gpio + gpioToGPLEV [4]) & (1 << (4 & 31))) == 1);
	while((int) (*(gpio + gpioToGPLEV [4]) & (1 << (4 & 31))) == 0);

	iowrite32((unsigned long) 1 << CS, (u32 *) gpio + 7);

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

void setPinsToOutput(void) {

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

void sidReset() {

	if(sid) {
		atomic_set(sid->reset,1);
		down(sid->todoReset);
	}
	
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

int mapGPIO(void) {

	   unsigned long mem;

	   mem = request_mem_region(GPIO_BASE, 4096, "gpio");
	   if(mem == NULL) {
		   printk(KERN_ERR "Cannot get GPIO");
		   return -1;
	   }
	   gpio = ioremap(GPIO_BASE, 4096);

	   mem = request_mem_region(GPIO_CLOCK, 32, "gpioclk");
	   if(mem == NULL) {
		   printk(KERN_ERR "Cannot get GPIO Clock");
		   return -1;
	   }
	   gpio_clock = ioremap(GPIO_CLOCK, 32);

	   mem = request_mem_region(GPIO_TIMER, 256, "gpiotimer");
	   if(mem == NULL) {
		   printk(KERN_ERR "Cannot get GPIO timer");
		   return -1;
	   }
   	   gpio_timer = ioremap(GPIO_TIMER, 256);

	   return 0;
}

void unmapGPIO(void) {
	iounmap(gpio);
	iounmap(gpio_clock);
	iounmap(gpio_timer);
	release_mem_region(GPIO_BASE, 4096);
	release_mem_region(GPIO_CLOCK, 32);
	release_mem_region(GPIO_TIMER, 256);

}
