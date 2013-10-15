package com.wattu.test.sidpi.impl;

import com.wattu.sidpi.GPIOController;

public class GPIOControllerTestImpl implements GPIOController {
		
	private int[] test_pins = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 
	
	public void clockSpeed(int pin,int freq) {
		System.out.println("##TEST## Clock Speed : pin : " + pin + " freq : " + freq);
	}
	
	public void setPins(int pins[],int values[]) {
		System.out.println("##TEST## Set Pins");
		for(int i=0;i<pins.length;i++) {
			System.out.println("##TEST## Pin " + i + " : " + pins[i] + " value : " + values[i]);
			test_pins[pins[i]] = values[i];
		}
	}

	public void setPin(int pin, int value) {
		System.out.println("##TEST## Set Pin : Pin " + pin + " value : " + value);
		test_pins[pin] = value;
	}
	
	public int getPin(int pin) {
		System.out.println("##TEST## Get Pin " +pin + " value : " + test_pins[pin]);
		return test_pins[pin];
	}
	
	public int[] getPins(int pins[]) {
		int ret[] = new int[pins.length];
		for(int i=0;i<pins.length;i++) {
			System.out.println("##TEST## Get Pins " + pins[i] + " value : " + test_pins[pins[i]]);
			
			ret[i] = test_pins[pins[i]];
		}
		return ret;
	}
	
	public void setPinMode(int pin,int mode) {
		System.out.println("##TEST## Set Pin mode " + pin + " mode : " + mode);
		
	}
	
}
