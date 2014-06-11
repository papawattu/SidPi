/*
 * Sidrunnerthread.h
 *
 *  Created on: 12 Jan 2014
 *      Author: Jamie Nuttall
 */

#ifndef SIDRUNNERTHREAD_H_
#define SIDRUNNERTHREAD_H_

#define CS 	18
#define RW 	0
#define RES 0
#define CLK 4

#define BUFFER_SIZE (65536 *4 + 16384)

#define DEFAULT_SID_SPEED_HZ 1000000

const int DATA[] = {11,9,10,22,27,17,3,2};
const int ADDR[] = {23,24,25,8,7};

void sidDelay(int cycles);
void sidWrite(int reg,int value,int cycleHigh,int cycleLow);
void setupSid();
void *cmdThread();
void *sidThread();
void delay(long cycles);
void writeSid(int reg,int val);
void startSidClk(int freq);
void mmapRPIDevices();
void generatePinTables();
void setPinsToOutput();
int playbackReady();
void startPlayback();
void stopPlayback();
int getBufferFirst();
int getBufferLast();
int getBufferCount();
int getBufferFull();
int getBufferMax();
long getRealSidClock();
void setThreshold(int value);
void setMultiplier(int value);
void startSidThread();

#endif /* SIDRUNNERTHREAD_H_ */
