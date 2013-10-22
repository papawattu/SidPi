package com.wattu.sidpi;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class SIDRunnerThread extends Thread {
		
	private SIDPiController sid;
	private BlockingQueue<SIDWrite> commandQ;
	
	public SIDRunnerThread(SIDPiController sid) {
		this.sid = sid;
		commandQ = new LinkedBlockingQueue<SIDWrite>();
		
	}
	
	@Override
	public void run() {
		try {
			synchronized (commandQ) {
			
				commandQ.wait();
			
			}
			while (true) {
				SIDWrite write = commandQ.poll();

				if (!write.isDelay()) {
					sid.writeRegister(write.getAddress(), write.getValue());
				} 
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
