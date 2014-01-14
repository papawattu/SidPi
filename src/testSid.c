/*
 * testSid.c
 *
 *  Created on: 14 Jan 2014
 *      Author: jamie
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "SidRunnerThreadh.h"

int main(void) {

	setupSid();

	while(1) {
		writeSid(0,0);
		sleep(1);
		writeSid(31,255);
		sleep(1);
		writeSid(10,170);
		sleep(1);
		writeSid(21,85);
		sleep(1);
	}

	return 0;
}

