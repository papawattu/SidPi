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

unsigned char *buffer;
unsigned int bufReadPos, bufWritePos;
unsigned long dataPins[256];
unsigned long addrPins[32];

void setupSid() {

	int reg,val;

	buffer = malloc((size_t) BUFFER_SIZE);
	bufReadPos = 0;
	bufWritePos = 0;

	mmapRPIDevices();

	generatePinTables();

	setPinsToOutput();

	startSidClk(DEFAULT_SID_SPEED_HZ);

	if (pthread_create(&sidThreadHandle, NULL, sidThread, NULL) == -1)
		perror("cannot create thread");
}

void *sidThread() {
	printf("Sid Thread Running...\n");
	while (1) {
		if (bufWritePos > bufReadPos) {
			if ((unsigned char) buffer[bufReadPos] != 0xff) {
				writeSid(buffer[bufReadPos], buffer[bufReadPos + 1]);
				delay(((buffer[bufReadPos + 3] & 0xff00) << 8) | (buffer[bufReadPos + 3] & 0xff));

			} else {
				printf("delay\n ");

				delay(((buffer[bufReadPos + 3] & 0xff00) << 8) | (buffer[bufReadPos + 3] & 0xff));
			}

			if (bufReadPos >= BUFFER_SIZE - 4)
				bufReadPos = 0;
			else
				bufReadPos += 4;
		} else {
			sleep(1);
		}
	}
}

void sidDelay(int cycles) {
	printf("siddelay : cycles %d\n ",cycles);

	if (bufWritePos >= BUFFER_SIZE - 4)
		bufWritePos = 0;

	buffer[bufWritePos] = 0xff;
	buffer[bufWritePos + 1] = 0;
	buffer[bufWritePos + 3] = (cycles & 0xff00) << 8;
	buffer[bufWritePos + 2] = cycles & 0xff;

}
void sidWrite(int reg, int value, int writeCycles) {
	if (bufWritePos >= BUFFER_SIZE - 4)
		bufWritePos = 0;
	buffer[bufWritePos] = reg;
	buffer[bufWritePos + 1] = value;
	buffer[bufWritePos + 3] = (writeCycles & 0xff00) << 8;
	buffer[bufWritePos + 2] = writeCycles & 0xff;
	bufWritePos += 4;
}
void delay(int cycles) {
	long long int * beforeCycle, *afterCycle, target;
	struct timespec tim;
	printf("delay : cycles %d : \n",cycles);

	target = *(long long int *) ((char *) gpio_timer.addr + TIMER_OFFSET)
			+ cycles;
	if (cycles < 10)
		return;
	if (cycles < 100)
		while (*(long long int *) ((char *) gpio_timer.addr + TIMER_OFFSET)
				< target)
			;
	else {
		tim.tv_sec = 0;
		tim.tv_nsec = cycles -10;
		nanosleep(&tim, NULL);
	}

	//usleep(100);
}
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
