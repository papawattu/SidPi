package com.wattu.sidpi;

public class TestSIDController {
	public static void main(String[] args) throws InterruptedException {
		SIDPiController sidPiController = new SIDPiController();
	
		sidPiController.init();
		sidPiController.reset();
		
		sidPiController.write(24, 0);
		sidPiController.write(24, 15);
		sidPiController.write(5, 190);
		sidPiController.write(6, 248);
		sidPiController.write(1, 17);
		sidPiController.write(0, 37);
		sidPiController.write(4, 17);
		Thread.sleep(8000)	;
		sidPiController.write(24, 0);
			 
		sidPiController.reset();
	}

}
