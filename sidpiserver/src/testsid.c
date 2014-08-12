#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "serialsid.h"
#include "rpi.h"

int main(int argc, char * argv[]) {

	int i;
	//if(argc != 3) return -1;

	printf("Setting up SID ...\n");
	//printf("Reg %d value %d\n", atoi(argv[1]), atoi(argv[2]));
	setup_sid();

	GPIO_CLR = 1 << 3;
	GPIO_CLR = 1 << 2;

	for(i=0;i<100;i++) {

		if(i % 3) {
			GPIO_SET = 1 << 17;
		} else {
			GPIO_CLR = 1 << 17;
		}
		GPIO_SET = 1 << 3;
		GPIO_CLR = 1 << 3;
		if(i % 8) {
			GPIO_SET = 1 << 2;
			GPIO_CLR = 1 << 2;
		}
		sleep(1);
	}
}
