#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/resource.h>
#include <asm/delay.h>
#include <mach/platform.h>
#include <linux/syscalls.h>
#include "sidpithread.h"

#define timer_t		struct timer_list

struct
{
    timer_t  id;
    atomic_t expired;
} rt_timer;

static struct task_struct *thread;

typedef struct buffer {
	unsigned char q[SID_BUFFER_SIZE]; /* body of queue */
	unsigned int first; /* position of first element */
	unsigned int last; /* position of last element */
	unsigned int count; /* number of queue elements */
} Buffer;

Buffer buffer;

static unsigned char gpioToGPFSEL [] =
{
  0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,
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
static void __iomem * gpio, * gpio_clock, * gpio_timer;
int isPlaybackReady = 0;
unsigned long lastClock = 0, currentClock = 0;
int threshold = 10, multiplier = 1000;
int sidSetup = 0;
int timeValid = 0,cycles=0;
struct timeval lasttv;


static struct semaphore     bufferSem;
static struct semaphore     todoSem;

static struct semaphore     todo;

void init_queue(Buffer *q);
int enqueue(Buffer *q, unsigned char x);
unsigned char dequeue(Buffer *q);
int empty(Buffer *q);
void print_queue(Buffer *q);

void rt_timer_func (unsigned long arg)
{
    atomic_set (&rt_timer.expired, 1);
    up(&todo);
}

void setupSid(void) {

	int i, fSel, shift;

	if(sidSetup) return;

	printk(KERN_INFO "sidpi: setting up sid\n");

	timeValid = 0;

	if(mapGPIO() != 0) return;

	generatePinTables();

	setPinsToOutput();

	sema_init(&bufferSem, SID_BUFFER_SIZE /4);

	sema_init(&todoSem, 0);

	startSidClk(DEFAULT_SID_SPEED_HZ);
	
	init_timer (&rt_timer.id);
	rt_timer.id.data = 0;
	rt_timer.id.function = &rt_timer_func;

	init_MUTEX(&todo);

	startSidThread();

	sidSetup = 1;

}

void closeSid(void) {
	printk(KERN_INFO "sidpi: closing sid cleanup\n");
	up(&todoSem);
	stopSidThread();
	unmapGPIO();
}

void startSidThread(void) {

	int err;

	thread = kthread_run(sidThread, &err, THREAD_NAME);
	if (IS_ERR(thread)) {
		err = PTR_ERR(thread);
		thread = NULL;
	}	else {
		printk(KERN_INFO "sidpi: sid thread started\n");
	}
}

void stopSidThread(void) {
	int ret;
	ret = kthread_stop(thread);
	thread = NULL;
	printk(KERN_INFO "sidpi: sid thread stopped\n");
}

int sidThread(void) {
	unsigned char reg, val;
	int cycles,clocks;
	long startClock;
	struct timeval tv,lasttv;
	current->policy=SCHED_FIFO;
	current->rt_priority=1;
	//current->prio = sys_sched_get_priority_max(SCHED_FIFO);
	set_user_nice(current, -20);
	//current->need_resched = 1;
	init_queue(&buffer);
	while (!kthread_should_stop()) {

		if (signal_pending(current))
			break;

		if (buffer.count > 3) {
			down_interruptible(&todoSem);

			reg = dequeue(&buffer);
			val = dequeue(&buffer);

			cycles = dequeue(&buffer) | dequeue(&buffer) << 8;

		if ((unsigned char) reg != 0xff) {
			delay(cycles);
			writeSid(reg, val);
				//printk(KERN_INFO "Write val %x reg %x delay %04x\n",val,reg,cycles);

		} else {
			delay(cycles);
				//printk(KERN_INFO "Delay %2x\n", cycles);
		}
			up(&bufferSem);
		} else {
			msleep(10);
		}
		do_gettimeofday(&tv);

			/* Update cycle status */
		clocks = (tv.tv_sec - lasttv.tv_sec) * 1000000
			    + ( tv.tv_usec - lasttv.tv_usec);

		memcpy(&lasttv, &tv, sizeof(tv));
		cycles -= clocks;

		if ( cycles < 0 )
			cycles = 0;
		atomic_set(&(todo->count), 0);

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

	down(&bufferSem);
	if(enqueue(&buffer, (unsigned char) 0xff) != 0) return -1;
	if(enqueue(&buffer, (unsigned char) 0) != 0) return -1;
	if(enqueue(&buffer, (unsigned char) cycles & 0xff) != 0) return -1;
	if(enqueue(&buffer, cycles >> 8) != 0) return -1;
	up(&todoSem);

	return 0;

}
int sidWrite(int reg, int value, unsigned int cycles) {

	down(&bufferSem);
	if(enqueue(&buffer, (unsigned char) reg & 0xff) != 0) return -1;
	if(enqueue(&buffer, (unsigned char) value & 0xff) != 0) return -1;
	if(enqueue(&buffer, cycles & 0xff) != 0) return -1;
	if(enqueue(&buffer, cycles >> 8) != 0) return -1;
	up(&todoSem);
	up(&todo);
	return 0;
}
void delay(unsigned int howLong) {

	int clocks,now;
	struct timeval tv;

	cycles+= howLong;

	if(timeValid) {

		do_gettimeofday(&tv);

		clocks = (tv.tv_sec - lasttv.tv_sec) * 1000000
		                + ( tv.tv_usec - lasttv.tv_usec);

		memcpy(&lasttv, &tv, sizeof(tv));

		cycles -= clocks;

		//printk(KERN_INFO "1 Clocks %lu Delay %d Last Clock %lu Difference %lu\n",clocks,howLong,lastClock,getRealSidClock() - lastClock);
		while (cycles > 1000000 / HZ ) {

			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(cycles / 1000000);
			do_gettimeofday(&tv);

			clocks = (tv.tv_sec - lasttv.tv_sec) * 1000000
			           + ( tv.tv_usec - lasttv.tv_usec);

			memcpy(&lasttv, &tv, sizeof(tv));
			cycles -= clocks;
		}

		if(cycles > 10) {
			udelay(cycles);

		}

	} else {

		timeValid=1;
		udelay(4);
	}
}

static inline int_least64_t timer_get (void)
{
    struct timeval tv;
    do_gettimeofday(&tv);
    return (int_least64_t) tv.tv_sec * NSEC_PER_SEC + (tv.tv_usec * 1000);
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
	iowrite32((unsigned long) 1 << CS, (u32 *) gpio + 10);
	iowrite32((unsigned long) dataPins[val % 256], (u32 *) gpio + 7);
	iowrite32((unsigned long) ~dataPins[val % 256] & dataPins[255], (u32 *) gpio + 10);
	udelay(600);
	iowrite32((unsigned long) 1 << CS, (u32 *) gpio + 7);

}
void startSidClk(int freq) {
	int divi, divr, divf;

//	divi = 19;
//	divr = 19200000 % freq;
//	divf = 42;

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
	int i;
	do_gettimeofday(&lasttv);
	sema_init(&bufferSem, SID_BUFFER_SIZE/4);
	sema_init(&todoSem, 0);
	timeValid =0;
	//flush();
	for(i=0;i<32;i++) {
		writeSid(i,0);
	}
	writeSid(24,15);
	init_queue(&buffer);
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
void init_queue(Buffer *q) {
	q->first = 0;
	q->last = SID_BUFFER_SIZE - 1;
	q->count = 0;
}

int enqueue(Buffer *q, unsigned char x) {
	if (q->count >= SID_BUFFER_SIZE) {
	//	printk(KERN_INFO "Warning: queue overflow enqueue x=%d\n", x);
		return -1;
	} else {
		q->last = (q->last + 1) % SID_BUFFER_SIZE;
		q->q[q->last] = x;
		q->count = q->count + 1;
	}
	return 0;
}

unsigned char dequeue(Buffer *q) {
	unsigned char x;

	if (q->count <= 0)
		printk(KERN_INFO "Warning: empty queue dequeue.\n");
	else {
		x = q->q[q->first];
		q->first = (q->first + 1) % SID_BUFFER_SIZE;
		q->count = q->count - 1;
	}

	return (x);
}

int empty(Buffer *q) {
	if (q->count <= 0)
		return (1);
	else
		return (0);
}

void print_queue(Buffer *q) {
	int i, j;

	i = q->first;

	while (i != q->last) {
		printk(KERN_INFO "%c ", q->q[i]);
		i = (i + 1) % SID_BUFFER_SIZE;
	}

	printk(KERN_INFO "%2d ", q->q[i]);
	printk(KERN_INFO "\n");
}

int getBufferFirst(void) {
	return buffer.first;
}
int getBufferLast(void) {
	return buffer.last;
}
int getBufferCount(void) {
	return buffer.count;
}
int getBufferFull(void) {
	return (buffer.count >= SID_BUFFER_SIZE - 4 ? 1 : 0);
}
int getBufferMax(void) {
	return SID_BUFFER_SIZE;
}
void flush(void) {
	while ( atomic_read( (atomic_t *) &todoSem.count) > 0 ) {
	  current->state = TASK_INTERRUPTIBLE;
	  schedule_timeout(1);
	}
	init_queue(&buffer);
	currentClock = 0;
	stopPlayback();
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
