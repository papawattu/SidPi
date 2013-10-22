package com.wattu.sidpi;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class SIDRunnerThread extends Thread {
		
	private SIDPiController sid;
	private BlockingQueue<SIDWrite> commandQ;
	
	public SIDRunnerThread(SIDPiController sid) {
		setPriority(Thread.MAX_PRIORITY);
		this.sid = sid;
		commandQ = new LinkedBlockingQueue<SIDWrite>();
		
	}
	
	@Override
	public void run() {
		try {
			System.out.println("SIDRunner - entered run");
			
			synchronized (commandQ) {
			
				commandQ.wait();
			
			}
			System.out.println("SIDRunner - entering main loop");
			while (true) {
				System.out.println("SIDRunner - polling");
				
				SIDWrite write = commandQ.poll();

				System.out.println("SIDRunner - got write command");
				
				
				if (!write.isDelay()) {
					System.out.println("SIDRunner - write reg");
					
					sid.writeRegister(write.getAddress(), write.getValue());
				} 
				System.out.println("SIDRunner - wait");
				
				sid.waitForCycles(write.getCycles());
			}
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public long getPlaybackClock() {
		return 0;
	}

	public BlockingQueue<SIDWrite> getSidCommandQueue() {
		
		return commandQ;
	}		
}
