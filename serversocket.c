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
#include "sid.h"

#define PORT "6581"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define DATA_READ_SIZE 65536 * 4 * 16384
#define DATA_WRITE_SIZE 260

void *sid_thread(void *ptr);
void signal_callback_handler(int signum);

pthread_mutex_t mutex1, mutex2 = PTHREAD_MUTEX_INITIALIZER;
unsigned char *dataRead, *dataWrite;
unsigned int dataWritePos = 0;
unsigned int dataReadPos = 0;

void sigchld_handler(int s) {
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

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
	pthread_t sidThread;

	signal(SIGINT, signal_callback_handler);

	dataRead = malloc((size_t) DATA_READ_SIZE);
	dataWrite = malloc((size_t) DATA_WRITE_SIZE);

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

		if (!fork()) { // this is the child process

			if (pthread_create(&sidThread, NULL, sid_thread, (void*) data)
					== -1)
				perror("cannot create thread");

			printf("SID thread started");

			close(sockfd); // child doesn't need the listener
			memset(&data, 0, sizeof data);
			rv = read(new_fd, dataRead, 16384);
			while (rv > -1) {
				if (rv > 0) {
					processReadBuffer(rv);
					pthread_mutex_lock(&mutex1);
					bufWritePos = (bufWritePos + 1) % COMMAND_BUFFER_SIZE;
					pthread_mutex_unlock(&mutex1);

					if (send(new_fd, &data, 2, 0) == -1)
						perror("send failed");

				}
				rv = read(new_fd, buffer, 1024);

			}
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

void *sid_thread(void * ptr) {
	unsigned char *buffer;

	buffer = (unsigned char *) ptr;
	while (1) {
		pthread_mutex_lock(&mutex2);
		if (bufWritePos > bufReadPos || bufReadPos > bufWritePos) {
			printf("buffer write pos is %d\n", bufWritePos);
			printf("buffer read pos is %d\n", bufReadPos);
			bufReadPos = (bufReadPos + 1) % COMMAND_BUFFER_SIZE;
		}
		pthread_mutex_unlock(&mutex2);
		usleep(100);
	}
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

	switch (command) {
	case FLUSH:
		if (dataLength != 0) {
			invalidCommandException("FLUSH needs no data");
		}

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
			invalidCommandException("RESET needs 1 byte (volume after reset)",
					dataLength);
		}

		dataWrite[dataWritePos++] = OK;
		break;

		/* SID command queuing section. */
	case TRY_DELAY: {
		if (dataLength != 2) {
			invalidCommandException(
					"TRY_DELAY needs 2 bytes (16-bit delay value)");
		}

		/*if (isBufferHalfFull) {
			eventConsumerThread.ensureDraining();
		}

		if (isBufferFull) {
			dataWrite.put((byte) Response.BUSY.ordinal());
			break;
		} */

		int cycles = (dataRead[dataReadPos+4] << 8) | dataRead[dataReadPos+5];
		handleDelayPacket(sidNumber, cycles);
		dataWrite[dataWritePos++] = OK;
		break;
	}

	case TRY_WRITE: {
		if (dataLength < 4 && (dataLength % 4) != 0) {
			invalidCommandException(
					"TRY_WRITE needs 4*n bytes, with n > 1 (hardsid protocol)");
		}

		/*if (isBufferHalfFull) {
			eventConsumerThread.ensureDraining();
		}

		if (isBufferFull) {
			dataWrite.put((byte) Response.BUSY.ordinal());
			break;
		} */

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
			invalidCommandException("GET_INFO needs no data", dataLength);
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
			invalidCommandException("SET_SID_LEVEL needs 1 byte", dataLength);
		}

		dataWrite[dataWritePos++] = OK;
		break;

	case TRY_SET_SID_MODEL:
		if (dataLength != 1) {
			invalidCommandException("SET_SID_LEVEL needs 1 byte");
		}

		dataWrite[dataWritePos++] = OK;
		break;

	default:
		invalidCommandException("Unsupported command");
	}

	void invalidCommandException(char * errMsg) {
		perror(*errMsg);
		exit(-1);
	}
	void handleDelayPacket(int sidNumber, int cycles) {

	}
	void handleWritePacket(int dataLength) {

	}
	}

}
