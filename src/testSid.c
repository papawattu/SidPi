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

	char readLine[50];
	int reg,val;

	printf("Command (reg value) : ");

	gets(readLine);
	while(readLine[0] != 'x') {
		reg = atoi()
	}
	return 0;
}

