package com.wattu.sidpi;

public class Runner implements Runnable {
	public int count=0;
	public GPIOController gpioController;

	public Runner() {
		count = 0;
		gpioController = new GPIOController();
		gpioController.setup();
		gpioController.setPinMode(4, 3);
		gpioController.setClock(4, 1000000);
		gpioController.setPinMode(3, 0);
	}

	@Override
	public void run() {
		while(true) {
			while(gpioController.read(3)==1);
			count++;
		}
		
	}
}