/*
 * rpi.h
 *
 *  Created on: 12 Jan 2014
 *      Author: jamie
 */

#ifndef RPI_H_
#define RPI_H_


#define BCM2708_PERI_BASE       0x20000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)	// GPIO controller
#define ST_BASE 				(BCM2708_PERI_BASE + 0x003000)  // Timer
#define TIMER_OFFSET 			(4)
#define BLOCK_SIZE 				(4*1024)

// IO Access
struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};

struct bcm2835_peripheral gpio = {GPIO_BASE};

//extern struct bcm2835_peripheral gpio;  // They have to be found somewhere, but can't be in the header

#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET  *(gpio.addr + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR  *(gpio.addr + 10) // clears bits which are 1 ignores bits which are 0

#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))

#endif /* RPI_H_ */
