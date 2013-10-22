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
		while (true) {
			
			SIDWrite write = commandQ.poll();
			if(write != null) {
			
				if (!write.isDelay()) {
				
					sid.writeRegister(write.getAddress(), write.getValue());
				} 
			
				sid.waitForCycles(write.getCycles());
			}
		}
	}
	public void ensureDraining() {
		synchronized (commandQ) {
			commandQ.notify();
		}
	}
	public long getPlaybackClock() {
		return sid.getCurrentCycle();
	}

	public BlockingQueue<SIDWrite> getSidCommandQueue() {
		
		return commandQ;
	}		
}
