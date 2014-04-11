#include <linux/kthread.h>
//#include <time.h>
//#include <linux/time.h>
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <mach/platform.h>
#include "sidpithread.h"

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

const int DATA[] = {2,3,17,27,22,10,9,11};
const int ADDR[] = {8,25,24,23,18};

unsigned int bufReadPos, bufWritePos;
unsigned long dataPins[256];
unsigned long addrPins[32];
unsigned long * gpio;
int isPlaybackReady = 0;
long lastClock = 0, currentClock = 0, realClock, realClockStart, targetCycles;
int threshold = 10, multiplier = 1000;
int sidSetup = 0;

void init_queue(Buffer *q);
int enqueue(Buffer *q, unsigned char x);
unsigned char dequeue(Buffer *q);
int empty(Buffer *q);
void print_queue(Buffer *q);

void setupSid(void) {

	if(sidSetup) return;

	if(mapGPIO() != 0) return;

	printf(KERN_INFO "GPIO mapped addr is %x and value is %x\n",gpio,ioread32(gpio));

	generatePinTables();

	//setPinsToOutput();

	//startSidClk(DEFAULT_SID_SPEED_HZ);

	startSidThread();

	sidSetup = 1;

}

void closeSid(void) {
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
		printk(KERN_INFO "Sid Thread Running...\n");
	}
}

void stopSidThread(void) {
	int ret;
	ret = kthread_stop(thread);
}

int sidThread(void) {
	unsigned char reg, val;
	int cycles;
	long startClock;
	init_queue(&buffer);
	startClock = getRealSidClock();
	while (!kthread_should_stop()) {

		if (buffer.count >= 3 && playbackReady()) {
			targetCycles = getRealSidClock();
			reg = dequeue(&buffer);
			val = dequeue(&buffer);

			cycles = (int) dequeue(&buffer) << 8;
			cycles |= (int) dequeue(&buffer);

			currentClock += cycles;
			targetCycles += cycles;
			if ((unsigned char) reg != 0xff) {

				delay(cycles);
				writeSid(reg, val);

			} else {
				delay(cycles);
			}

		} else {
//			usleep(100);
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
void sidDelay(int cycles) {

	enqueue(&buffer, (unsigned char) 0xff);
	enqueue(&buffer, (unsigned char) 0);
	enqueue(&buffer, (unsigned char) (cycles & 0xff00) >> 8);
	enqueue(&buffer, (unsigned char) cycles & 0xff);

}
void sidWrite(int reg, int value, int cycleHigh, int cycleLow) {
	enqueue(&buffer, (unsigned char) reg & 0xff);
	enqueue(&buffer, (unsigned char) value & 0xff);
	enqueue(&buffer, (unsigned char) cycleHigh & 0xff);
	enqueue(&buffer, (unsigned char) cycleLow & 0xff);

}
void delay(long howLong) {

/*	struct timespec sleeper;
	struct timeval tNow, tLong, tEnd;
	if (howLong == 0)
		return;

	if (howLong < 100) {
		gettimeofday(&tNow, NULL);
		tLong.tv_sec = howLong / 1000000;
		tLong.tv_usec = howLong % 1000000;
		timeradd(&tNow, &tLong, &tEnd);

	} else {
		sleeper.tv_sec = 0;
		sleeper.tv_nsec = (long) (howLong * 1000);
		nanosleep(&sleeper, NULL);
	}

	while (timercmp (&tNow, &tEnd, <)) {
		gettimeofday(&tNow, NULL);
	}
*/
}

void setThreshold(int value) {
	threshold = value;
}

void setMultiplier(int value) {
	multiplier = value;
}

long getSidClock(void) {
	return currentClock;
}
long getRealSidClock(void) {
	//long long int * clock = (long long int *) ((char *) gpio_timer.addr
	//		+ TIMER_OFFSET);
	return 0; //*clock;
}
void writeSid(int reg, int val) {
	int i;
	writel((unsigned long) addrPins[reg % 32], __io_address(GPIO_BASE + 7));
	writel((unsigned long) ~addrPins[reg % 32] & addrPins[31], __io_address(GPIO_BASE + 10));
	writel((unsigned long) 1 << CS, __io_address(GPIO_BASE + 10));
	writel((unsigned long) dataPins[val % 256], __io_address(GPIO_BASE + 7));
	writel((unsigned long) ~dataPins[val % 256] & dataPins[255], __io_address(GPIO_BASE + 10));
	writel((unsigned long) 1 << CS, __io_address(GPIO_BASE + 7));

}
void startSidClk(int freq) {
	int divi, divr, divf;

	divi = 19;
	divr = 19200000 % freq;
	divf = 42;

	if (divi > 4095)
		divi = 4095;
	writel(BCM_PASSWORD | GPIO_CLOCK_SOURCE, __io_address(GPIO_CLOCK + 28));

	while ((readl(__io_address(GPIO_CLOCK) + 28) & 0x80) != 0)
		;

	writel(BCM_PASSWORD | (divi << 12) | divf,__io_address(GPIO_CLOCK) + 29);
	writel(BCM_PASSWORD | 0x10 | GPIO_CLOCK_SOURCE,__io_address(GPIO_CLOCK) + 28);

	writel(0x0000280,__io_address(GPIO_TIMER) + TIMER_CONTROL);
	writel(0x00000F9,__io_address(GPIO_TIMER) + TIMER_PRE_DIV);
}

void setPinsToOutput(void) {

	int i, fSel, shift;

	for (i = 0; i < 8; i++) {
		fSel = gpioToGPFSEL[DATA[i]];
		shift = gpioToShift[DATA[i]];
		writel(readl(__io_address(gpio + fSel)) & ~(7 << shift)
				| (1 << shift),__io_address(gpio) + fSel);
	}
	for (i = 0; i < 5; i++) {
		fSel = gpioToGPFSEL[ADDR[i]];
		shift = gpioToShift[ADDR[i]];
		writel(readl(__io_address(gpio) + fSel) & ~(7 << shift)
						| (1 << shift),__io_address(gpio) + fSel);
	}
	fSel = gpioToGPFSEL[CS];
	shift = gpioToShift[CS];
	writel(readl(__io_address(gpio) + fSel) & ~(7 << shift)
					| (1 << shift),__io_address(gpio) + fSel);
	fSel = gpioToGPFSEL[RW];
	shift = gpioToShift[RW];
	writel(readl(__io_address(gpio) + fSel) & ~(7 << shift)
					| (1 << shift),__io_address(gpio) + fSel);

	fSel = gpioToGPFSEL[RES];
	shift = gpioToShift[RES];
	writel(readl(__io_address(gpio) + fSel) & ~(7 << shift)
					| (1 << shift),__io_address(gpio) + fSel);
	fSel = gpioToGPFSEL[CLK];
	shift = gpioToShift[CLK];
	writel(readl(__io_address(gpio) + fSel) & ~(7 << shift)
					| (1 << shift),__io_address(gpio) + fSel);
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
		printk(KERN_INFO "Warning: queue overflow enqueue x=%d\n", x);
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
	init_queue(&buffer);
	currentClock = 0;
	stopPlayback();
}

int mapGPIO(void) {

	   unsigned long mem;

	   mem = request_mem_region(GPIO_BASE, 4096, "mygpio");

	   if(mem == NULL) {
		   printk(KERN_ERR "Cannot get GPIO");
		   return -1;
	   }
	   gpio = ioremap(GPIO_BASE, 4096);

	   return 0;
}

void unmapGPIO(void) {
	release_mem_region(GPIO_BASE, 4096);
	iounmap(gpio);
}
