package com.wattu.sidpi;

public class TestGPIOController {

	public static void main(String[] args) throws InterruptedException {
		
		Runner t = new Runner();
		new Thread(t).start();
		while(true) {
			Thread.sleep(1000);
			System.out.println("Ping " + t.count);
			t.count = 0;
			
	   }
	}
}
