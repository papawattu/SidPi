package com.wattu.sidpi;

public interface GPIOController {
	
	public static final int[] PIN_NUMBERS = {2,3,4,17,27,22,10,9,11,14,15,18,23,24,25,8,7};
	public static final String[] PIN_NAMES = {"GPIO02","GPIO03","GPIO04","GPIO17","GPIO27","GPIO22"
								,"GPIO10","GPIO09","GPIO11","GPIO14","GPIO15","GPIO18"
								,"GPIO23","GPIO24","GPIO25","GPIO08","GPIO07"};
	
	public static final int MODE_IN = 0;
	public static final int MODE_OUT = 1;
	public static final int MODE_CLOCK = 3;
	
	public static final int VALUE_HIGH = 1;
	public static final int VALUE_LOW = 0;
	
	public void clockSpeed(int pin,int freq);
	
	public void setPins(int pins[],int values[]);

	public void setPin(int pin, int value);
	
	public int getPin(int pin);
	
	public int[] getPins(int pins[]);
	
	public void setPinMode(int pin,int mode);

}
