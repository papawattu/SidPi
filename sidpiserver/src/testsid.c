#include <stdio.h>
#include "serialsid.h"

int main(int argc, char * argv[]) {

	if(argc != 2) return -1;

	printf("Setting up SID ...\n");
	printf("Params %0x %0x\n", argv[0], argv[1]);
	//setup_sid();

}
