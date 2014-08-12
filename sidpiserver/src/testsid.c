#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serialsid.h"

int main(int argc, char * argv[]) {

	//if(argc != 3) return -1;

	printf("Setting up SID ...\n");
	//printf("Reg %d value %d\n", atoi(argv[1]), atoi(argv[2]));
	//setup_sid();

	for(i=0;i<100;i++) {
		write_bit(1);
		sleep(1);
		write_bit(0);
		sleep(1);
	}
}
