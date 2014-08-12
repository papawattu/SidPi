#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "serialsid.h"
#include "rpi.h"

int main(int argc, char * argv[]) {

	int i,j;
	//if(argc != 3) return -1;

	printf("Setting up SID ...\n");
	//printf("Reg %d value %d\n", atoi(argv[1]), atoi(argv[2]));
	setup_sid();

	for(i=0;i<32;i++) {
		for(j=0;j<256;j++) {
			write_sid(i,j);
			sleep(1);
		}
	}
}
