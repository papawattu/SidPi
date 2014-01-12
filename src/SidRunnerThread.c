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

void setupSid() {

	buffer = malloc((size_t) BUFFER_SIZE);

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
		usleep(100);
	}
}

void sidDelay(int cycles) {
	printf("Delay cycles %d\n",cycles);
}
void sidWrite(int reg,int value,int writeCycles) {
	printf("Write reg %d : value %d : cycles %d\n",reg,value,writeCycles);
}
