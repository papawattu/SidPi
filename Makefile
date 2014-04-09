# kbuild part of makefile
obj-m  += chardev.o 
chardev-objs := src/chardev.o src/SidRunnerThread.o src/rpi.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

modules_install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install
