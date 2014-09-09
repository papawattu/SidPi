# kbuild part of makefile
obj-m  += sidpi.o 
sidpi-objs := src/sidpi.o src/sidpithread.o 
EXTRA_CFLAGS = -O2

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

modules_install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install
