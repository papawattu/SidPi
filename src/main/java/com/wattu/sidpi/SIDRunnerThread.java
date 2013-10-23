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
			System.out.println("Sync - start");
			synchronized (commandQ) {
				commandQ.wait();
			}
			System.out.println("Sync - end");
			
			while (true) {
				System.out.println("In main loop");
				
				SIDWrite write = commandQ.poll();
				System.out.println("after poll");
				
				if(write != null) {
					System.out.println("Got right command");
					
					if (!write.isDelay()) {
						System.out.println("is a write");
						
						sid.writeRegister(write.getAddress(), write.getValue());
					} 
					System.out.println("waitint for cycles " + write.getCycles());
					
					sid.waitForCycles(write.getCycles());
				}
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
			System.err.println(e);
			
		}
	}
	public void ensureDraining() {
		System.out.println("Ensure draining");
		
		synchronized (commandQ) {
			commandQ.notify();
		}
		System.out.println("Ensure draining end");
		
	}
	public long getPlaybackClock() {
		return sid.getCurrentCycle();
	}

	public BlockingQueue<SIDWrite> getSidCommandQueue() {
		
		return commandQ;
	}		
}
