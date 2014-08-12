#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "serialsid.h"
#include "rpi.h"

int main(int argc, char * argv[]) {

	int i;
	//if(argc != 3) return -1;

	printf("Setting up SID ...\n");
	//printf("Reg %d value %d\n", atoi(argv[1]), atoi(argv[2]));
	setup_sid();

	for(i=0;i<100;i++) {
		write_bit(1);
		GPIO_SET = 1 << 3;
		sleep(1);
		write_bit(0);
		GPIO_CLR = 1 << 3;

		sleep(1);
	}
}
