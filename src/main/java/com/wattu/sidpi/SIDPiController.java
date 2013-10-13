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
		gpioController.setup();
		
	}
	
	public void setClockSpeed(int speed) {
		gpioController.setClock(CLK, speed);
	}
	
	public void startClock() {
		gpioController.startClock();
	}
	public int read(int addr) {
		return gpioController.readByte();
	}
	
	private void setRead() {
		for(int i=0;i<8;i++) gpioController.setPinMode(DATA[i], GPIOController.MODE_IN);
	}

	private void setWrite() {
		for(int i=0;i<8;i++) gpioController.setPinMode(DATA[i], GPIOController.MODE_OUT);
	}

	public void write(int addr,int data) {
		
		addr &= 0x1f;
		data &= 0xff;
		setWrite();
		setAddr(addr);
		
		gpioController.writeByte(data);
		
		return;
	}
	
	private void waitSID() {
		//while(gpioController.read(CLK)!=GPIOController.VALUE_HIGH);
		//while(gpioController.read(CLK)!=GPIOController.VALUE_LOW);
		
	}
	
	private void setAddr(int addr) {
		gpioController.setPinMode(ADDR[0],1);
		gpioController.setPinMode(ADDR[1],1);
		gpioController.setPinMode(ADDR[2],1);
		gpioController.setPinMode(ADDR[3],1);
		gpioController.setPinMode(ADDR[4],1);
		
		gpioController.write(ADDR[0], (addr & 1));
		gpioController.write(ADDR[1], (addr & 2) >> 1);
		gpioController.write(ADDR[2], (addr & 4) >> 2);
		gpioController.write(ADDR[3], (addr & 8) >> 3);
		gpioController.write(ADDR[4], (addr & 16) >> 4);
		
	}
	private void setData(int data) {
		for(int i=0;i<8;i++) {
			if(((data >>> i) & 1) == 1) {
				gpioController.write(DATA[i], GPIOController.VALUE_HIGH);
			} else {
				gpioController.write(DATA[i], GPIOController.VALUE_LOW);
			}
		}
	}
	private int getData() {
		int data = 0;
		
		for(int i=0;i<8;i++) {
			if(gpioController.read(DATA[i]) == GPIOController.VALUE_HIGH) {
				data |= (1 << i);
			} 
		}
		return data;
	}
	public void reset() {
		gpioController.write(RW, GPIOController.VALUE_HIGH);
		gpioController.write(CS, GPIOController.VALUE_LOW);
		gpioController.write(RES, GPIOController.VALUE_LOW);
		try {
			Thread.sleep(1);
		} catch (InterruptedException e) {
		}
		gpioController.write(RES, GPIOController.VALUE_HIGH);
		gpioController.write(CS, GPIOController.VALUE_HIGH);
		setWrite();
		for(int i=0;i<31;i++) {
			setAddr(i);
			gpioController.writeByte(0);
		}
		
		return;
	}
}
