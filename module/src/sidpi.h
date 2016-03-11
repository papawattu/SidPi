/*
 * SidRunnerThread.h
 *
 *  Created on: 12 Jan 2014
 *      Author: jamie
 */

#ifndef __SIDRUNNERTHREAD_H_
#define __SIDRUNNERTHREAD_H_

#include <linux/ioctl.h>

// Fixed for PI2 as base address has changed
#define	BCM2708_PERI_BASE_2     0x3F000000 //Pi 2 base address
#define BCM2708_PERI_BASE_1     0x20000000 //Pi 1 base address
#define GPIO_BASE               0x200000	// GPIO controller
#define GPIO_TIMER 				0x003000  // Timer
#define GPIO_CLOCK				0x00101000
#define TIMER_OFFSET 			(4)
#define GPIO_BLOCK_SIZE			(4*1024)
#define BCM_PASSWORD			0x5A000000
#define GPIO_CLOCK_SOURCE       1
#define TIMER_CONTROL			(0x408 >> 2)
#define TIMER_IRQ_RAW			(0x410 >> 2)
#define TIMER_PRE_DIV			(0x41C >> 2)

#define SID_IOCTL_RESET     _IOW('S', 0, int)
#define SID_IOCTL_FIFOSIZE  _IOR('S', 1, int)
#define SID_IOCTL_FIFOFREE  _IOR('S', 2, int)
#define SID_IOCTL_SIDTYPE   _IOR('S', 3, int)
#define SID_IOCTL_CARDTYPE  _IOR('S', 4, int)
#define SID_IOCTL_MUTE      _IOW('S', 5, int)
#define SID_IOCTL_NOFILTER  _IOW('S', 6, int)
#define SID_IOCTL_FLUSH     _IO ('S', 7)
#define SID_IOCTL_DELAY     _IOW('S', 8, int)
#define SID_IOCTL_READ      _IOWR('S', 9, int*)


#define CS 	18
#define RW 	0
#define RES 0
#define CLK 4

#define SID_BUFFER_SIZE (8192 << 2)
#define SID_PAL 1015

#define DEFAULT_SID_SPEED_HZ 985248
#define THREAD_NAME "sidpithread"

#define SIDPI_PARALLEL_INTERFACE 0
#define SIDPI_SERIAL_INTERFACE   1
#define SIDPILOG "SIDPi : "
#define ERROR -1
#define OK 1

typedef struct Sid {
	struct semaphore bufferSem;
	struct semaphore todoSem;
	struct semaphore wait;
	struct semaphore todoReset;
	unsigned int interfaceType;
	unsigned char volatile buffer[SID_BUFFER_SIZE]; /* body of queue */
	unsigned int lastCommand; /* position of first element */
	unsigned int currentCommand; /* position of last element */
	atomic_t todoCount; /* number of queue elements */
	__u64 targetTime;
	atomic_t reset;
	void (*writeSid) (struct Sid *sid, unsigned char reg,
	                                 unsigned char value);

} Sid;

extern const int DATA[];
extern const int ADDR[];

#endif /* __SIDRUNNERTHREAD_H_ */
