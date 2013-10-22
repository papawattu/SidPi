package com.wattu.sidpi;

public class SIDWrite {
	private int address;
	private int value;
	private int cycles;
	private boolean delay;
	
	public SIDWrite(byte address,byte value,int cycles) {
		this.address = address;
		this.value = value;
		this.cycles = cycles;
		this.delay = false;
	}

	public SIDWrite(int cycles) {
		this.cycles = cycles;
		this.delay = true;
	}
	
	public int getAddress() {
		return address;
	}

	public int getValue() {
		return value;
	}

	public int getCycles() {
		return cycles;
	}

	public boolean isDelay() {
		return delay;
	}
}