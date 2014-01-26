/*
 ** server.c -- a stream socket server demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include "SidPiServer.h"

pthread_mutex_t mutex1, mutex2 = PTHREAD_MUTEX_INITIALIZER;
unsigned char *dataRead, *dataWrite;
unsigned int dataWritePos = 0;
unsigned int dataReadPos = 0;
long inputClock = 0;
int latency = DEFAULT_LATENCY;
int delayMulti = DEFAULT_DELAY_MULTI;
int delayThreshold = DEFAULT_THRESHOLD;

int main(void) {
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	unsigned char buffer[1024];
	int iret1;
	int i;

	signal(SIGINT, signal_callback_handler);

	dataRead = malloc(DATA_READ_SIZE);
	dataWrite = malloc(DATA_WRITE_SIZE);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while (1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		setupSid();

		startSidThread();

		if (!fork()) { // this is the child process

			setMultiplier(delayMulti);

			//startSidThread();

			close(sockfd); // child doesn't need the listener

			rv = read(new_fd, dataRead, 16384);

			while (rv > -1) {
				if (rv > 0) {

					processReadBuffer(rv);

					if (send(new_fd, dataWrite, dataWritePos, 0) == -1)
						perror("send failed");

				}
				rv = read(new_fd, dataRead, 16384);

			}
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

void signal_callback_handler(int signum) {
	printf("Caught signal %d.\n", signum);
	free(dataRead);
	free(dataWrite);

	exit(signum);

}

void processReadBuffer(int len) {
	int command, sidNumber, dataLength;

	command = dataRead[dataReadPos];
	sidNumber = dataRead[dataReadPos + 1];
	dataLength = (dataRead[dataReadPos + 2] << 8) | dataRead[dataReadPos + 3];
	dataWritePos = 0;

	long clientTimeDifference = inputClock - getSidClock();
	int isBufferFull = (clientTimeDifference > latency?1:0);
	int isBufferHalfFull = (clientTimeDifference > latency / 2?1:0);

	//printf("input clock : %d\n",inputClock);
	//printf("input sid clock : %d\n",getSidClock());
	//printf("first %d last %d count %d\n",getBufferFirst(),getBufferLast(),getBufferCount());

	//printf("Full buffer : %d\n",isBufferFull);
	//printf("Half buffer : %d\n",isBufferHalfFull);

	switch (command) {
	case FLUSH:
		if (dataLength != 0) {
			invalidCommandException("FLUSH needs no data");
		}
		flush();
		dataWrite[dataWritePos++] = OK;
		break;

	case TRY_SET_SID_COUNT:
		if (dataLength != 0) {
			invalidCommandException("TRY_SET_SID_COUNT needs no data");
		}

		dataWrite[dataWritePos++] = OK;
		break;

	case MUTE:
		if (dataLength != 2) {
			invalidCommandException(
					"MUTE needs 2 bytes (voice and channel to mute)");
		}

		dataWrite[dataWritePos++] = OK;
		break;

	case TRY_RESET:
		if (dataLength != 1) {
			invalidCommandException("RESET needs 1 byte (volume after reset)");
		}
		flush();
		writeSid(0x18,dataRead[1] & 0xff);
		dataWrite[dataWritePos++] = OK;
		break;

		/* SID command queuing section. */
	case TRY_DELAY: {
		if (dataLength != 2) {
			invalidCommandException(
					"TRY_DELAY needs 2 bytes (16-bit delay value)");
		}
		printf("delay cmd\n");
/*
		if (isBufferHalfFull) {
			startPlayback();
		}

		if (isBufferFull || getBufferFull() ) {
			dataWrite[dataWritePos++] = BUSY;
			break;
		}

		int cycles = (int) ((dataRead[4] & 0xff) << 8) | dataRead[5];
		handleDelayPacket(sidNumber, cycles); */
		dataWrite[dataWritePos++] = OK;

		break;
	}

	case TRY_WRITE: {
		if (dataLength < 4 && (dataLength % 4) != 0) {
			invalidCommandException(
					"TRY_WRITE needs 4*n bytes, with n > 1 (hardsid protocol)");
		}

		if(isBufferHalfFull) {
			startPlayback();
		}

		if (isBufferFull ) {
			dataWrite[dataWritePos++] = BUSY;
			break;
		}

		handleWritePacket(dataLength);
		dataWrite[dataWritePos++] = OK;

		break;
	}

	case TRY_READ: {
		if ((dataLength - 3) % 4 != 0) {
			invalidCommandException(
					"READ needs 4*n+3 bytes (4*n hardsid protocol + 16-bit delay + register to read)");
		}

		dataWrite[dataWritePos++] = READ;
		dataWrite[dataWritePos++] = 0;

		break;
	}

		/* Metdata method section */
	case GET_VERSION:
		if (dataLength != 0) {
			invalidCommandException("GET_VERSION needs no data");
		}

		dataWrite[dataWritePos++] = VERSION;
		dataWrite[dataWritePos++] = SID_NETWORK_PROTOCOL_VERSION;
		break;

	case TRY_SET_SAMPLING:
		if (dataLength != 1) {
			invalidCommandException(
					"SET_SAMPLING needs 1 byte (method to use: 0=bad quality but fast, 1=good quality but slow)");
		}


		dataWrite[dataWritePos++] = OK;
		break;

	case TRY_SET_CLOCKING:
		if (dataLength != 1) {
			invalidCommandException("SET_CLOCKING needs 1 byte (0=NTSC, 1=PAL)");
		}

		dataWrite[dataWritePos++] = OK;
		break;

	case GET_CONFIG_COUNT:
		if (dataLength != 0) {
			invalidCommandException("GET_COUNT needs no data");
		}
		dataWrite[dataWritePos++] = COUNT;
		dataWrite[dataWritePos++] = 1;
		break;

	case GET_CONFIG_INFO:
		if (dataLength != 0) {
			invalidCommandException("GET_INFO needs no data");
		}

		dataWrite[dataWritePos++] = INFO;
		dataWrite[dataWritePos++] = 0;
		strcpy(dataWrite + dataWritePos,SID_PI);
		dataWritePos += sizeof SID_PI;
		dataWrite[dataWritePos++] = 0;
		break;

	case SET_SID_POSITION:
		if (dataLength != 1) {
			invalidCommandException("SET_SID_POSITION needs 1 byte");
		}

		dataWrite[dataWritePos++] = OK;
		break;

	case SET_SID_LEVEL:
		if (dataLength != 1) {
			invalidCommandException("SET_SID_LEVEL needs 1 byte");
		}

		dataWrite[dataWritePos++] = OK;
		break;

	case TRY_SET_SID_MODEL:
		if (dataLength != 1) {
			invalidCommandException("SET_SID_LEVEL needs 1 byte");
		}

		dataWrite[dataWritePos++] = OK;
		break;
	case SET_DELAY_MULTI:
		if (dataLength != 2) {
			invalidCommandException("SET_SID_LEVEL needs 1 byte");
		}
		setMultiplier((dataRead[dataReadPos + 4] << 8) | dataRead[dataReadPos + 5]);
		dataWrite[dataWritePos++] = OK;
		break;
	case SET_LATENCY:
		if (dataLength != 2) {
			invalidCommandException("SET_SID_LEVEL needs 1 byte");
		}
		latency = (dataRead[dataReadPos + 4] << 8) | dataRead[dataReadPos + 5];
		dataWrite[dataWritePos++] = OK;
		break;
	case SET_THRESHOLD:
		if (dataLength != 2) {
			invalidCommandException("SET_SID_LEVEL needs 1 byte");
		}
		setThreshold((dataRead[dataReadPos + 4] << 8) | dataRead[dataReadPos + 5]);
		dataWrite[dataWritePos++] = OK;
		break;
	default:
		invalidCommandException("Unsupported command");
	}
	dataReadPos = 0;
}
void invalidCommandException(void *errMsg) {
	perror((char *) errMsg);
	exit(-1);
}
void handleWritePacket(int dataLength) {
	unsigned int i,writeCycles;
	unsigned char reg,sid,value;

	for (i = 0; i < dataLength; i += 4) {
		writeCycles = (int) ((dataRead[4 + i] & 0xff) << 8) | dataRead[5 + i];
		reg = dataRead[4 + i + 2];
		sid = ((reg & 0xe0) >> 5);
		reg &= 0x1f;
		value = dataRead[4 + i + 3];
		inputClock += writeCycles;
		sidWrite(reg,value,(dataRead[4 + i] & 0xff),dataRead[5 + i]);
		//printf("***MAIN THREAD*** : Current Cycle %08x\tReg %02x\tValue %02x\tCycles %04x\n",inputClock,reg,value,writeCycles);

	}
	return;
}
void handleDelayPacket(int sidNumber, int cycles) {
	//printf("cmd delay %d",cycles);
	if(getBufferCount() >= getBufferMax()) return;

	inputClock += cycles;
	sidDelay(cycles);
	return;
}

void sigchld_handler(int s) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}
}

