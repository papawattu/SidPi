package com.wattu.sidpi;

import com.wattu.sidpi.GPIOController;
import com.wattu.sidpi.impl.GPIOControllerImpl;
import com.wattu.test.sidpi.impl.GPIOControllerTestImpl;

public class SIDPiController {

	private static final int CS 	= 25;
	private static final int RW 	= 8;
	private static final int RES 	= 7;
	private static final int CLK 	= 4;
	private static final int[] DATA	= {2,3,17,27,22,10,9,11};
	
	private static final int[] ADDR	= {24,23,18,15,14};
			
	private static final int DEFAULT_SID_SPEED_HZ = 1000000;
	
	private int currentSidSpeed = 0;
	private boolean clockRunning = false;
	private int[] vals = new int[8];
	private int[] addr = new int[5];
	
	private GPIOController gpioController;
	
	public SIDPiController() {
		
		gpioController = new GPIOControllerImpl(); 
		setClockSpeed(DEFAULT_SID_SPEED_HZ);
		startClock();
		reset();
		setCSHigh();
		setWriteMode();
	}

	public void setReadMode() {
		gpioController.setPin(RW, GPIOController.VALUE_HIGH);
	}
	
	public void setWriteMode() {
		gpioController.setPin(RW, GPIOController.VALUE_LOW);
	}
	
	private void setResetHigh() {
		gpioController.setPin(RES, GPIOController.VALUE_HIGH);
	}

	public void setCSLow() {
		gpioController.setPin(CS, GPIOController.VALUE_LOW);
	}

	public void setCSHigh() {
		gpioController.setPin(CS, GPIOController.VALUE_HIGH);
	}

	public int readRegister(int address) {
		
		addr[0] = (address & 1);
		addr[1] = (address & 2) >> 1;
		addr[2] = (address & 4) >> 2;
		addr[3] = (address & 8) >> 3;
		addr[4] = (address & 16) >> 4;
		
		gpioController.setPins(ADDR, addr);
		setReadMode();
		setCSLow();
		clockHigh();
		int[] data = gpioController.getPins(DATA);
		setCSHigh();
		clockLow();
		
		int value = data[0] | 
					(data[1] << 1) |
					(data[2] << 2) |
					(data[3] << 3) |
					(data[4] << 4) |
					(data[5] << 5) |
					(data[6] << 6) |
					(data[7] << 7); 
					
		setWriteMode();			
		return value;
	}

	public void writeRegister(int address, int value) {
		
		addr[0] = (address & 1);
		addr[1] = (address & 2) >> 1;
		addr[2] = (address & 4) >> 2;
		addr[3] = (address & 8) >> 3;
		addr[4] = (address & 16) >> 4;
		
		vals[0] = value & 1;
		vals[1] = (value & 2) >> 1;
		vals[2] = (value & 4) >> 2;
		vals[3] = (value & 8) >> 3;
		vals[4] = (value & 16) >> 4;
		vals[5] = (value & 32) >> 5;
		vals[6] = (value & 64) >> 6;
		vals[7] = (value & 128) >> 7;
		gpioController.setPins(ADDR, addr);
		setCSLow();
		gpioController.setPins(DATA,vals);
		gpioController.delay(1);
		setCSHigh();
	}

	public void setClockSpeed(int speed) {
		currentSidSpeed = speed;
	}
	
	public boolean isClockRunning() {
		return clockRunning;
	}
	public int getClockSpeed() {
		return currentSidSpeed;
	}

	public void reset() {
		gpioController.setPin(RES, GPIOController.VALUE_LOW);
		
		waitForCycles(8);
		
		gpioController.setPin(RES, GPIOController.VALUE_HIGH);
		
	}
	
	public void waitForCycles(int cycles) {
		long target = gpioController.getClock() + cycles;
			
		while(gpioController.getClock() < target) Thread.yield(); 
	}
	
	public void advanceClock() {
		advanceClock(1);
	}
	
	private void advanceClock(int cycles)  {
	
	}
	public long getCurrentCycle() {
		return gpioController.getClock();
	}

	private void clockHigh() {
		if(!clockRunning) {
			gpioController.setPin(CLK, GPIOController.VALUE_HIGH);
		}
	}
	
	private void clockLow() {
	
	}

	public void startClock() {
		gpioController.clockSpeed(CLK, currentSidSpeed);
		clockRunning = true;
	}
	
	public void stopClock() {
		gpioController.setPinMode(CLK,GPIOController.MODE_OUT);
		gpioController.setPin(CLK, GPIOController.VALUE_LOW);
		clockRunning = false;
	}
}
