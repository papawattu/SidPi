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
	
	private int currentSidSpeed = 0;
	
	private GPIOController gpioController;
	
	public SIDPiController() {
		
		gpioController = new GPIOController(); 
		setCSHigh();
		setResetHigh();
		
	}

	private void setResetHigh() {
		gpioController.setPin(RES, 1);
		
	}

	public void setCSLow() {
		gpioController.setPin(CS, 0);
		
	}

	public void setCSHigh() {
		gpioController.setPin(CS, 1);
	}

	public int readRegister(int address) {
		int[] addr = new int[5];
		
		addr[0] = (address & 1);
		addr[1] = (address & 2) >> 1;
		addr[2] = (address & 4) >> 2;
		addr[3] = (address & 8) >> 3;
		addr[4] = (address & 16) >> 4;
		
		gpioController.setPins(ADDR, addr);
		gpioController.setPin(RW, 1);
		setCSLow();
		int[] data = gpioController.getPins(DATA);
		setCSHigh();
		
		int value = data[0] | 
					(data[1] << 1) |
					(data[2] << 2) |
					(data[3] << 3) |
					(data[4] << 4) |
					(data[5] << 5) |
					(data[6] << 6) |
					(data[7] << 7); 
					
					
		return value;
	}

	public void writeRegister(int address, int value) {
		int[] addr = new int[5];
		
		addr[0] = (address & 1);
		addr[1] = (address & 2) >> 1;
		addr[2] = (address & 4) >> 2;
		addr[3] = (address & 8) >> 3;
		addr[4] = (address & 16) >> 4;
		
		gpioController.setPins(ADDR, addr);
		gpioController.setPin(RW, 0);
		setCSLow();
		int[] vals = new int[8];
		
		vals[0] = value & 1;
		vals[1] = (value & 2) >> 1;
		vals[2] = (value & 4) >> 2;
		vals[3] = (value & 8) >> 3;
		vals[4] = (value & 16) >> 4;
		vals[5] = (value & 32) >> 5;
		vals[6] = (value & 64) >> 6;
		vals[7] = (value & 128) >> 7;
		
		gpioController.setPins(DATA,vals);
		setCSHigh();
	}

	public void setClockSpeed(int speed) {
		gpioController.clockSpeed(CLK, speed);
		
	}
	
}
