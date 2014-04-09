/*
 * SidRunnerThread.h
 *
 *  Created on: 12 Jan 2014
 *      Author: jamie
 */

#ifndef __SIDRUNNERTHREAD_H_
#define __SIDRUNNERTHREAD_H_

#define CS 	7
#define RW 	0
#define RES 0
#define CLK 4

#define BUFFER_SIZE (65536 *4 + 16384)

#define DEFAULT_SID_SPEED_HZ 1000000
#define THREAD_NAME "sidpithread"


extern const int DATA[];
extern const int ADDR[];

void sidDelay(int cycles);
void sidWrite(int reg,int value,int cycleHigh,int cycleLow);
void setupSid(void);
void *cmdThread(void);
int sidThread(void);
void delay(long cycles);
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
long getRealSidClock(void);
void setThreshold(int value);
void setMultiplier(int value);
void startSidThread(void);

#endif /* __SIDRUNNERTHREAD_H_ */
