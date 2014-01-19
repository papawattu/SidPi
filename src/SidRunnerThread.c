#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include "SidRunnerThread.h"
#include "rpi.h"

pthread_t sidThreadHandle;

typedef struct buffer {
        unsigned char q[BUFFER_SIZE+1];		/* body of queue */
        int first;                      /* position of first element */
        int last;                       /* position of last element */
        int count;                      /* number of queue elements */
} Buffer;

Buffer buffer;

unsigned int bufReadPos, bufWritePos;
unsigned long dataPins[256];
unsigned long addrPins[32];
int isPlaybackReady = 0;
long lastClock = 0,currentClock = 0;

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

	if (pthread_create(&sidThreadHandle, NULL, sidThread, NULL) == -1)
		perror("cannot create thread");
}

void *sidThread() {
	unsigned char reg,val;
	int cycles;
	init_queue(&buffer);
//
	printf("Sid Thread Running...\n");
	while (1) {
		//print_queue(&buffer);
		//printf("playback ready %d : empty : %d buffer : %8x\n",playbackReady(),empty(&buffer),&buffer);
		if (!empty(&buffer) && playbackReady()) {
			reg = dequeue(&buffer);
			val = dequeue(&buffer);

			cycles = (int) ((dequeue(&buffer) &0xff) << 8) | dequeue(&buffer);

			printf("cycles = %4x\n",cycles);

			if ((unsigned char) reg != 0xff) {

				writeSid(reg,val);
				delay(cycles);

			} else {
				printf("delay thread\n");
				delay(cycles);
			}
			currentClock +=cycles;
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
void sidWrite(int reg, int value, int cycles) {
	//printf("reg = %d\t: val = %d\t: cycles = %d\tbuffer %8x\n",reg,value,cycles,&buffer);
	enqueue(&buffer,(unsigned char) reg & 0xff);
	enqueue(&buffer,(unsigned char) value & 0xff);
	enqueue(&buffer,(unsigned char) (cycles & 0xff00) >> 8);
	enqueue(&buffer,(unsigned char) cycles & 0xff);

	//printf("cycles1 = %d\tcycles2 = %d",cycles & 0xff, (cycles & 0xff00) >> 8);

}
void delay(int cycles) {
	long long int * timer;
	long long int beforeCycle, afterCycle, difference;

	struct timespec tim ={0};
	timer = (long long int *) ((char *) gpio_timer.addr + TIMER_OFFSET);
	beforeCycle = *timer;
	if (cycles < 8) return;
	return;
	tim.tv_nsec = cycles * 900;

	nanosleep(&tim, NULL);

	do {
		afterCycle = *timer;
		difference = afterCycle - beforeCycle;
		printf("target : %d\tdifference %llu\n",cycles,difference);
	} while(cycles >= difference);



}

long getSidClock() {
	return currentClock;
/*	if(isPlaybackReady) {
		if(lastClock == 0) {
			lastClock = (long) (*(long long int *) ((char *) gpio_timer.addr + TIMER_OFFSET) & 0xffffffff);
		}
		currentClock =  (long) (*(long long int *) ((char *) gpio_timer.addr + TIMER_OFFSET) & 0xffffffff) - lastClock;
		lastClock = currentClock;
		return currentClock;
	} else {
		return lastClock;
	}

	return (long) (*(long long int *) ((char *) gpio_timer.addr + TIMER_OFFSET) & 0xffffffff);
*/}
void writeSid(int reg, int val) {
	//printf("reg : %d val : %d data pins : %ul addr pins : %ul \n",reg,val,dataPins[val % 256],addrPins[reg % 32]);
	*(gpio.addr + 7) = (unsigned long) addrPins[reg % 32];
	*(gpio.addr + 10) = (unsigned long) ~addrPins[reg % 32] & addrPins[31];
	delay(1);
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
        printf("CC count %d : last %d : queue %d\n",q->count,q->last,q->q[q->last]);

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
         //       printf("BB count %d : last %d : queue %d : x %d\n",q->count,q->last,q->q[q->last],x);
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


