package com.wattu.sidpi.impl;

import com.wattu.sidpi.GPIOController;

public class GPIOControllerImpl implements GPIOController {
	
	static {
	      System.loadLibrary("GPIOController"); 
	      if(wiringPiSetup() < 0) {
				throw new RuntimeException("Wiring Pi Setup failed");
		  }
	}
	
	private native static int wiringPiSetup();
	
	public native void clockSpeed(int pin,int freq);
	
	public native void setPins(int pins[],int values[]);

	public native void setPin(int pin, int value);
	
	public native int getPin(int pin);
	
	public native int[] getPins(int pins[]);
	
	public native void setPinMode(int pin,int mode);
	
}
