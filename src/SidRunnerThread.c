#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "SidRunnerThread.h"
#include "rpi.h"

pthread_t sidThreadHandle;

unsigned char *buffer;
unsigned int bufReadPos,bufWritePos;

void setupSid() {

	buffer = malloc((size_t) BUFFER_SIZE);
	bufReadPos = 0;
	bufWritePos = 0;

	if(map_peripheral(&gpio) == -1) {
		printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
		return;
	}
	if(map_peripheral(&timer) == -1) {
		printf("Failed to map the physical Timer into the virtual memory space.\n");
		return;
	}

	if (pthread_create(&sidThreadHandle, NULL, sidThread, NULL) == -1)
		perror("cannot create thread");
}

void *sidThread() {
	printf("Sid Thread Running...\n");
	while (1) {
		if(bufWritePos > bufReadPos) {
			if(buffer[bufReadPos] != 0xff)
				writeSid(buffer[bufReadPos],buffer[bufReadPos+1]);

			delay(buffer[bufReadPos+2]);

			if(bufReadPos >= BUFFER_SIZE - 3)
				bufReadPos = 0;
			else
				bufReadPos+=3;
		}
	}
}

void sidDelay(int cycles) {
	if(bufWritePos >= BUFFER_SIZE - 3)
			bufWritePos = 0;

	buffer[bufWritePos] = 0xff;
	buffer[bufWritePos + 1] = 0;
	buffer[bufWritePos + 2] = cycles;
}
void sidWrite(int reg,int value,int writeCycles) {
	if(bufWritePos >= BUFFER_SIZE - 3)
		bufWritePos = 0;
	buffer[bufWritePos] = reg;
	buffer[bufWritePos + 1] = value;
	buffer[bufWritePos + 2] = writeCycles;
	bufWritePos +=3;
}
void delay(int cycles) {
	long long int * beforeCycle,afterCycle;
	long difference; // 64 bit timer

	beforeCycle = (long long int *)((char *)timer.addr + TIMER_OFFSET);

	printf("Delay %d : Current cycle %llu\n",cycles,*beforeCycle);
	usleep(cycles);

	afterCycle = (long long int *)((char *)timer.addr + TIMER_OFFSET);

	difference = *afterCycle - *beforeCycle;

	printf("Current cycle %llu : difference %lu\n",*afterCycle, difference);

}
void writeSid(int reg,int val) {
	printf("Write reg %x val %x\n",reg,val);
}

