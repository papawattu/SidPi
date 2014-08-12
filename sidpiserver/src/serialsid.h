/*
 * rpi.h
 *
 *  Created on: 12 Jan 2014
 *      Author: jamie
 */

#ifndef SERIALSID_H_
#define SERIALSID_H_

#include <stdint.h>

void setup_sid(void);

void write_bit(uint8_t);

void write_sid(uint8_t,uint8_t);

void reset_sid(void);

void delay(void);

void set_output(uint8_t);

void start_sid_clock(int freq);

void mmap_devices(void);

#endif
