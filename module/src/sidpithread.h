/*
 * SidRunnerThread.h
 *
 *  Created on: 12 Jan 2014
 *      Author: jamie
 */

#ifndef __SIDRUNNERTHREAD_H_
#define __SIDRUNNERTHREAD_H_

#include <linux/ioctl.h>

#define BCM2708_PERI_BASE       0x20000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)	// GPIO controller
#define GPIO_TIMER 				(BCM2708_PERI_BASE + 0x003000)  // Timer
#define GPIO_CLOCK				(BCM2708_PERI_BASE + 0x00101000)
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

#define SID_BUFFER_SIZE (8192 *8)

#define DEFAULT_SID_SPEED_HZ 1000000
#define THREAD_NAME "sidpithread"


extern const int DATA[];
extern const int ADDR[];

int sidDelay(unsigned int cycles);
int sidWrite(int reg,int value,unsigned int cycles);
void setupSid(void);
void closeSid(void);
void *cmdThread(void);
int sidThread(void);
void stopSidThread(void);
void delay(unsigned int cycles);
void writeSid(int reg,int val);
void startSidClk(int freq);
void mmapRPIDevices(void);
void generatePinTables(void);
void setPinsToOutput(void);
int playbackReady(void);
void startPlayback(void);
void stopPlayback(void);
int getBufferFirst(void);
int getBufferLast(void);
int getBufferCount(void);
int getBufferFull(void);
int getBufferMax(void);
unsigned long getRealSidClock(void);
unsigned long getSidClock(void);
void setThreshold(int value);
void setMultiplier(int value);
void startSidThread(void);
void stopSidThread(void);
int mapGPIO(void);
void unmapGPIO(void);
void sidReset(void);
void flush(void);

#endif /* __SIDRUNNERTHREAD_H_ */
