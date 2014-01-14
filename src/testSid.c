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
//#include "SidRunnerThread.h"

int main(void) {

	setupSid();
	writeSid(5,9);
	writeSid(6,0);
	writeSid(24,15);
	writeSid(1,25);
	writeSid(10,177);

	while(1) {
		writeSid(4,33);
		sleep(1);
		writeSid(4,32);
		sleep(1);
	}

	return 0;
}

