#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "serialsid.h"
#include "rpi.h"

#define DEFAULT_SID_SPEED_HZ 1000000

#define SER 	17
#define SCLK 	3
#define RCLK 	2
#define CS 		27
#define CLK		4

uint8_t reset = 1;
unsigned int bit_counter = 0;

void set_output(uint8_t pin) {
	INP_GPIO(pin);
	OUT_GPIO(pin);

} // set_output

void write_bit(uint8_t bit) {

	GPIO_CLR = 1 << SCLK;
	GPIO_CLR = 1 << RCLK;

	if(bit == 1) {
		printf("Setting bit true : bitcount %d : bit %d\n",bit_counter,bit);
		GPIO_SET = 1 << SER;
	} else if(bit == 0) {
		printf("Setting bit false : bitcount %d : bit %d\n",bit_counter,bit);
		GPIO_CLR = 1 << SER;
	} else {
		perror("can only pass 1 or 0 to write_bit.\n");
	}
	GPIO_SET = 1 << RCLK;
	delay();
	GPIO_CLR = 1 << RCLK;

	bit_counter ++;
} // write_bit

void setup_sid() {

	int i, fSel, shift;

	mmap_devices();

	set_output(SER);
	set_output(RCLK);
	set_output(SCLK);
	set_output(CS);
	set_output(CLK);

	//SET_GPIO_ALT(CLK,0);
	fSel = gpioToGPFSEL[CLK];
	shift = gpioToShift[CLK];
	*(gpio.addr + fSel) = (*(gpio.addr + fSel) & ~(7 << shift)) | (4 << shift);

	start_sid_clock(DEFAULT_SID_SPEED_HZ);

	//reset_sid();
} // setup_sid

void reset_sid() {
	reset = 0;
	write_sid(0,0);
	reset = 1;
} // reset_sid

void write_sid(uint8_t addr,uint8_t data) {

	int i;
	data &= 0xff;
	addr &= 0x1f;

	printf("Addr %d Data %d\n",addr,data);

	for(i = 7;i >= 0;i--) {
		write_bit((data >> i) & 1);
	}

	write_bit(0); // NC
	write_bit(0); // NC

	write_bit(reset);

	for(i = 4;i >= 0;i--) {
		write_bit((addr >> i) & 1);
	}

	GPIO_SET = 1 << SCLK;
	delay();
	GPIO_CLR = 1 << SCLK;
	GPIO_CLR = 1 << CS;
	delay();
	GPIO_SET = 1 << CS;
	printf("Bit counter is %d\n",bit_counter);

} // write_sid

void delay() {

} // delay

void mmap_devices() {
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
} // mmap_devices
void start_sid_clock(int freq) {
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

	*(gpio_timer.addr + TIMER_CONTROL) = 0x0000280;
	*(gpio_timer.addr + TIMER_PRE_DIV) = 0x00000F9;
} // start_sid_clock
