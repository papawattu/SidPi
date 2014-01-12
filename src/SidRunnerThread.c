#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "SidRunnerThread.h"

pthread_t sidThreadHandle;

unsigned char *buffer;
unsigned int bufReadPos,bufWritePos;

void setupSid() {

	buffer = malloc((size_t) BUFFER_SIZE);
	bufReadPos = 0;
	bufWritePos = 0;
	if (pthread_create(&sidThreadHandle, NULL, sidThread, NULL) == -1)
		perror("cannot create thread");
}

void *sidThread() {
	printf("Sid Thread Running...\n");
	while (1) {
		//pthread_mutex_lock(&mutex2);
		/*if (bufWritePos > bufReadPos || bufReadPos > bufWritePos) {
			printf("buffer write pos is %d\n", bufWritePos);
			printf("buffer read pos is %d\n", bufReadPos);
			bufReadPos = (bufReadPos + 1) % COMMAND_BUFFER_SIZE;
		}
		pthread_mutex_unlock(&mutex2); */
		if(bufWritePos > bufReadPos) {
			printf("Buffer Read pos %d : Buffer Write pos : %d : Write reg %d : value %d : cycles %d\n",bufReadPos,bufWritePos,buffer[bufReadPos],buffer[bufReadPos+1],buffer[bufReadPos+2]);
			bufReadPos+=3;
		}
		usleep(100);
	}
}

void sidDelay(int cycles) {
	if(bufWritePos >= BUFFER_SIZE - 3)
			bufWritePos = 0;

	buffer[bufWritePos++] = 0xff;
	buffer[bufWritePos++] = 0;
	buffer[bufWritePos++] = cycles;
}
void sidWrite(int reg,int value,int writeCycles) {
	if(bufWritePos >= BUFFER_SIZE - 3)
		bufWritePos = 0;
	buffer[bufWritePos++] = reg;
	buffer[bufWritePos++] = value;
	buffer[bufWritePos++] = writeCycles;
}
