package com.wattu.sidpi;

public class PinData {
	
	public static enum GPIO_MODE {INPUT,OUTPUT};
	
	public static enum GPIO_PIN {GPIO02,GPIO03,GPIO04,GPIO07,GPIO08,GPIO09,GPIO10,GPIO11,GPIO14,GPIO15,GPIO17,GPIO18,GPIO22,GPIO23,GPIO24,GPIO25,GPIO27};

	public GPIO_PIN pin;
	public GPIO_MODE mode;
	public String name;

	public PinData() {
	}
}