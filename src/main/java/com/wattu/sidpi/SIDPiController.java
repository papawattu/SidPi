package com.wattu.sidpi;

import com.wattu.sidpi.GPIOController;
import com.wattu.sidpi.impl.GPIOControllerImpl;
import com.wattu.test.sidpi.impl.GPIOControllerTestImpl;

public class SIDPiController {

	private static boolean logging;
	
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
		if(logging) {
			System.out.println("SIDPiController - Constructor - start");
		}
		gpioController = new GPIOControllerImpl(); 
		setClockSpeed(DEFAULT_SID_SPEED_HZ);
		startClock();
		reset();
		setCSHigh();
		setWriteMode();
		if(logging) {
			System.out.println("SIDPiController - Constructor - finish");
		}
	}

	public void setReadMode() {
		if(logging) {
			System.out.println("SIDPiController - Set read mode - pin " + RW + " to high");
		}
		gpioController.setPin(RW, GPIOController.VALUE_HIGH);
	}
	
	public void setWriteMode() {
		if(logging) {
			System.out.println("SIDPiController - Set write mode - pin " + RW + " to low");
		}
		gpioController.setPin(RW, GPIOController.VALUE_LOW);
	}
	
	private void setResetHigh() {
		if(logging) {
			System.out.println("SIDPiController - Set reset high - pin " + RES + " to high");
		}
		gpioController.setPin(RES, GPIOController.VALUE_HIGH);
	}

	public void setCSLow() {
		if(logging) {
			System.out.println("SIDPiController - Set CS - pin " + CS + " to low");
		}
		gpioController.setPin(CS, GPIOController.VALUE_LOW);
	}

	public void setCSHigh() {
		if(logging) {
			System.out.println("SIDPiController - Set CS - pin " + CS + " to high");
		}
		gpioController.setPin(CS, GPIOController.VALUE_HIGH);
	}

	public int readRegister(int address) {
		if(logging) {
			System.out.println("SIDPiController - Read reg - address " + address );
		}
		
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
		if(logging) {
			System.out.println("SIDPiController - read value " + value);
		}
		return value;
	}

	public void writeRegister(int address, int value) {
		
		if(logging) {
			System.out.println("SIDPiController - Write register - address " + address + " to value " + value);
		}
		
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
		if(logging) {
			for(int i=0;i<5;i++) {
				System.out.println("SIDPiController - Address Pin " + i + " GPIO " + ADDR[i] + " to " + addr[i]);
			}
			for(int i=0;i<8;i++) {
				System.out.println("SIDPiController - Data Pin " + i + " GPIO " + DATA[i] + " to " + vals[i]);
			}
		}
		gpioController.setPins(ADDR, addr);
		setCSLow();
		gpioController.setPins(DATA,vals);
		gpioController.delay(1);
		setCSHigh();
	}

	public void setClockSpeed(int speed) {
		if(logging) {
			System.out.println("SIDPiController - Set clock speed to " + speed);
		}
		currentSidSpeed = speed;
	}
	
	public boolean isClockRunning() {
		if(logging) {
			System.out.println("SIDPiController - Is clock running " + clockRunning);
		}
		return clockRunning;
	}
	public int getClockSpeed() {
		if(logging) {
			System.out.println("SIDPiController - Set clock speed to " + currentSidSpeed);
		}
		return currentSidSpeed;
	}

	public void reset() {
		if(logging) {
			System.out.println("SIDPiController - Reset - pin " + RES + " to low");
		}
		gpioController.setPin(RES, GPIOController.VALUE_LOW);
		
		waitForCycles(8);
		
		if(logging) {
			System.out.println("SIDPiController - Reset - pin " + RES + " to low");
		}
		gpioController.setPin(RES, GPIOController.VALUE_HIGH);
		
	}
	
	public void waitForCycles(int cycles) {
		if(logging) {
			System.out.println("SIDPiController - Wait for cycles  " + cycles);
		}
		long target = gpioController.getClock() + cycles;
			
		while(gpioController.getClock() < target) Thread.yield(); 
	}
	
	public void advanceClock() {
		if(logging) {
			System.out.println("SIDPiController - Advance clock");
		}
		advanceClock(1);
	}
	
	private void advanceClock(int cycles)  {
	
	}
	public long getCurrentCycle() {
		if(logging) {
			System.out.println("SIDPiController - get current cycle " + gpioController.getClock());
		}
		return gpioController.getClock();
	}

	private void clockHigh() {
		if(!clockRunning) {
			if(logging) {
				System.out.println("SIDPiController - Clock High - pin " + CLK + " to low " );
			}
			gpioController.setPin(CLK, GPIOController.VALUE_HIGH);
		}
	}
	
	private void clockLow() {
		
		if(!clockRunning) {
			if(logging) {
				System.out.println("SIDPiController - Clock High - pin " + CLK + " to low " );
			}
			gpioController.setPin(CLK, GPIOController.VALUE_LOW);
		}
	}

	public void startClock() {
		if(logging) {
			System.out.println("SIDPiController - Start clock - pin " + CLK + " to speed " + currentSidSpeed);
		}
		gpioController.clockSpeed(CLK, currentSidSpeed);
		clockRunning = true;
	}
	
	public void stopClock() {
		if(logging) {
			System.out.println("SIDPiController - Stopping clock " );
		}
		gpioController.setPinMode(CLK,GPIOController.MODE_OUT);
		gpioController.setPin(CLK, GPIOController.VALUE_LOW);
		clockRunning = false;
	}

	public boolean isLogging() {
		return logging;
	}

	public void setLogging(boolean logging) {
		SIDPiController.logging = logging;
	}
}
