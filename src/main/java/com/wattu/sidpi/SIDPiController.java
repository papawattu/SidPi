package com.wattu.sidpi;

public class SIDPiController {

	private static final int CS 	= GPIOController.PIN_NUMBERS[14];
	private static final int RW 	= GPIOController.PIN_NUMBERS[15];
	private static final int RES 	= GPIOController.PIN_NUMBERS[16];
	private static final int CLK 	= GPIOController.PIN_NUMBERS[2];
	private static final int[] DATA	= {GPIOController.PIN_NUMBERS[0],GPIOController.PIN_NUMBERS[1],GPIOController.PIN_NUMBERS[3]
										,GPIOController.PIN_NUMBERS[4],GPIOController.PIN_NUMBERS[5],GPIOController.PIN_NUMBERS[6]
										,GPIOController.PIN_NUMBERS[7],GPIOController.PIN_NUMBERS[8]};
	
	private static final int[] ADDR	= {GPIOController.PIN_NUMBERS[13],GPIOController.PIN_NUMBERS[12],GPIOController.PIN_NUMBERS[11]
										,GPIOController.PIN_NUMBERS[10],GPIOController.PIN_NUMBERS[9]};
			
	private static final int DEFAULT_SID_SPEED_HZ = 1000000;
	
	private int currentSidSpeed;
	
	private GPIOController gpioController;
	
	public SIDPiController() {
		
		gpioController = new GPIOController(); 
		//	gpioController.setup();
		
	}
	
	public void setClockSpeed(int speed) {
//		gpioController.setClock(CLK, speed);
	}
	
	public void startClock() {
		gpioController.startClock();
	}
	public int read(int addr) {
		return gpioController.readByte();
	}
	
}
