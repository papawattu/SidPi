#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void sidDelay(int cycles) {
	printf("Delay cycles %d\n",cycles);
}
void sidWrite(int reg,int value,int writeCycles) {
	printf("Write reg %d : value %d : cycles %d\n",reg,value,writeCycles);
}
