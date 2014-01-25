#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "SidRunnerThread.h"
#include "rpi.h"

pthread_t sidThreadHandle;
pthread_t cmdThreadHandle;

typedef struct buffer {
        unsigned char q[BUFFER_SIZE];		/* body of queue */
        unsigned int first;                      /* position of first element */
        unsigned int last;                       /* position of last element */
        unsigned int  count;                      /* number of queue elements */
} Buffer;

Buffer buffer;

unsigned int bufReadPos, bufWritePos;
unsigned long dataPins[256];
unsigned long addrPins[32];
int isPlaybackReady = 0;
long lastClock = 0,currentClock = 0, realClock,realClockStart,targetCycles;
int threshold = 10,multiplier = 1000;

void init_queue(Buffer *q);
int enqueue(Buffer *q, unsigned char x);
unsigned char dequeue(Buffer *q);
int empty(Buffer *q);
void print_queue(Buffer *q);

void setupSid() {

	mmapRPIDevices();

	generatePinTables();

	setPinsToOutput();

	startSidClk(DEFAULT_SID_SPEED_HZ);

	realClock = getRealSidClock();
}

void startSidThread() {
	printf("Sid Thread Running...\n");

	if (pthread_create(&sidThreadHandle, NULL, sidThread, NULL) == -1)
		perror("cannot create Sid thread");
}

void *sidThread() {
	unsigned char reg,val;
	int cycles;
	long startClock;
	init_queue(&buffer);
	startClock = getRealSidClock();
	while (1) {


		if (buffer.count >= 3 && playbackReady()) {
			targetCycles = getRealSidClock();
			reg = dequeue(&buffer);
			val = dequeue(&buffer);

			cycles = (int) dequeue(&buffer) << 8;
			cycles |= (int) dequeue(&buffer);

			currentClock +=cycles;
			targetCycles += cycles;
			if ((unsigned char) reg != 0xff) {

				delay(cycles);
				writeSid(reg,val);

			} else {
				delay(cycles);
			}

		} else {
			usleep(100);
		}
	}
}

int playbackReady() {
	return isPlaybackReady;
}

void startPlayback() {
	isPlaybackReady = 1;
}

void stopPlayback() {
	isPlaybackReady = 0;
}
void sidDelay(int cycles) {
	//printf("siddelay : cycles %d\n ",cycles);

	enqueue(&buffer,(unsigned char) 0xff);
	enqueue(&buffer,(unsigned char) 0);
	enqueue(&buffer,(unsigned char) (cycles & 0xff00) >> 8);
	enqueue(&buffer,(unsigned char) cycles & 0xff);

}
void sidWrite(int reg, int value, int cycleHigh,int cycleLow) {
	//printf("reg = %02x\t: val = %02x\t: cycles = %04x\n",reg,value,cycles);
	enqueue(&buffer,(unsigned char) reg & 0xff);
	enqueue(&buffer,(unsigned char) value & 0xff);
	enqueue(&buffer,(unsigned char) cycleHigh & 0xff);
	enqueue(&buffer,(unsigned char) cycleLow & 0xff);

	//printf("cycles1 = %02x\tcycles2 = %02x\n",(cycles & 0xff00) >> 8, cycles & 0xff);

}
void delay(long howLong) {
/*	struct timespec tim;
	long current;
	if(cycles >= threshold) {
		tim.tv_sec = 0;
		tim.tv_nsec = (long) (cycles * multiplier);
		nanosleep(&tim,NULL);
	}
	*/
	//while(getRealSidClock() < targetCycles);
	 struct timespec sleeper ;
	 struct timeval tNow, tLong, tEnd ;
	  /**/
	   if (howLong ==   0)
	    return ;
	  else if (howLong  < 1024) {
		  gettimeofday (&tNow, NULL) ;
		  	tLong.tv_sec  = howLong / 1000000 ;
		  	tLong.tv_usec = howLong % 1000000 ;
		  	timeradd (&tNow, &tLong, &tEnd) ;

		  	while (timercmp (&tNow, &tEnd, <))
		  		gettimeofday (&tNow, NULL) ;

	  	  } else {
	  		  sleeper.tv_sec  = 0 ;
	  		  sleeper.tv_nsec = (long)(howLong * 1000) ;
	  		  nanosleep (&sleeper, NULL) ;
	  	  }

}

void setThreshold(int value) {
	threshold = value;
}

void setMultiplier(int value) {
	multiplier = value;
}

long getSidClock() {
	return currentClock;
}
long getRealSidClock() {
	long long int * clock = (long long int *) ((char *) gpio_timer.addr + TIMER_OFFSET);
	return *clock;
}
void writeSid(int reg, int val) {
	long current = getRealSidClock();
	int i;
	*(gpio.addr + 7) = (unsigned long) addrPins[reg % 32];
	*(gpio.addr + 10) = (unsigned long) ~addrPins[reg % 32] & addrPins[31];
	*(gpio.addr + 10) = (unsigned long) 1 << CS;
	*(gpio.addr + 7) = (unsigned long) dataPins[val % 256];
	*(gpio.addr + 10) = (unsigned long) ~dataPins[val % 256] & dataPins[255];
	delay(1);
	*(gpio.addr + 7) = (unsigned long)  1 << CS;
}
void startSidClk(int freq) {
	int divi, divr, divf;

	divi = 19200000 / freq;
	divr = 19200000 % freq;
	divf = (int) ((double) divr * 4096.0 / 19200000.0);

	if (divi > 4095)
		divi = 4095;

	*(gpio_clock.addr + 28) = BCM_PASSWORD | GPIO_CLOCK_SOURCE;
	while ((*(gpio_clock.addr + 28) & 0x80) != 0)
		;

	*(gpio_clock.addr + 29) = BCM_PASSWORD | (divi << 12) | divf;
	*(gpio_clock.addr + 28) = BCM_PASSWORD | 0x10 | GPIO_CLOCK_SOURCE;

	*(gpio_timer.addr + TIMER_CONTROL) = 0x0000280 ;
	*(gpio_timer.addr + TIMER_PRE_DIV) = 0x00000F9 ;
	//timerIrqRaw = gpio_timer.addr + TIMER_IRQ_RAW ;
}

void setPinsToOutput() {

	int i, fSel, shift;

	for (i = 0; i < 8; i++) {
		fSel = gpioToGPFSEL[DATA[i]];
		shift = gpioToShift[DATA[i]];
		*(gpio.addr + fSel) = (*(gpio.addr + fSel) & ~(7 << shift))
				| (1 << shift);
		usleep(10);
	}
	for (i = 0; i < 5; i++) {
		fSel = gpioToGPFSEL[ADDR[i]];
		shift = gpioToShift[ADDR[i]];
		*(gpio.addr + fSel) = (*(gpio.addr + fSel) & ~(7 << shift))
				| (1 << shift);
		usleep(10);
	}
	fSel = gpioToGPFSEL[CS];
	shift = gpioToShift[CS];
	*(gpio.addr + fSel) = (*(gpio.addr + fSel) & ~(7 << shift)) | (1 << shift);
	usleep(10);
	fSel = gpioToGPFSEL[RW];
	shift = gpioToShift[RW];
	*(gpio.addr + fSel) = (*(gpio.addr + fSel) & ~(7 << shift)) | (1 << shift);
	usleep(10);
	fSel = gpioToGPFSEL[RES];
	shift = gpioToShift[RES];
	*(gpio.addr + fSel) = (*(gpio.addr + fSel) & ~(7 << shift)) | (1 << shift);
	usleep(10);
	fSel = gpioToGPFSEL[CLK];
	shift = gpioToShift[CLK];
	*(gpio.addr + fSel) = (*(gpio.addr + fSel) & ~(7 << shift)) | (4 << shift);
	usleep(10);
}

void generatePinTables() {
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
void mmapRPIDevices() {
	if (map_peripheral(&gpio) == -1) {
		printf(
				"Failed to map the physical GPIO registers into the virtual memory space.\n");
		return;
	}
	if (map_peripheral(&gpio_clock) == -1) {
		printf(
				"Failed to map the physical Clock into the virtual memory space.\n");
		return;
	}
	if (map_peripheral(&gpio_timer) == -1) {
		printf(
				"Failed to map the physical Timer into the virtual memory space.\n");
		return;
	}
}
void init_queue(Buffer *q)
{
		q->first = 0;
        q->last = BUFFER_SIZE-1;
        q->count = 0;
}

int enqueue(Buffer *q, unsigned char x)
{
		//printf("AA count %d : last %d : queue %d : x %d\n",q->count,q->last,q->q[q->last],x);
        if (q->count >= BUFFER_SIZE) {
        	printf("Warning: queue overflow enqueue x=%d\n",x);
        	return -1;
        }
        else {
                q->last = (q->last+1) % BUFFER_SIZE;
                q->q[ q->last ] = x;
                q->count = q->count + 1;
              // printf("BB count %d : last %d : queue %d : x %d\n",q->count,q->last,q->q[q->last],x);
        }
        return 0;
}

unsigned char dequeue(Buffer *q)
{
        unsigned char x;

        if (q->count <= 0) printf("Warning: empty queue dequeue.\n");
        else {
                x = q->q[ q->first ];
                q->first = (q->first+1) % BUFFER_SIZE;
                q->count = q->count - 1;
        }

        return(x);
}

int empty(Buffer *q)
{
        if (q->count <= 0) return (1);
        else return (0);
}

void print_queue(Buffer *q)
{
        int i,j;

        i=q->first;

        while (i != q->last) {
                printf("%c ",q->q[i]);
                i = (i+1) % BUFFER_SIZE;
        }

        printf("%2d ",q->q[i]);
        printf("\n");
}

int getBufferFirst() {
	return buffer.first;
}
int getBufferLast() {
	return buffer.last;
}
int getBufferCount() {
	return buffer.count;
}
int getBufferFull() {
	return (buffer.count >= BUFFER_SIZE-4?1:0);
}
int getBufferMax() {
	return BUFFER_SIZE;
}
void flush() {
	init_queue(&buffer);
}

