package com.wattu.sidpi;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;


public class SIDMenuConsole {
	
	private enum COMMANDS {NOP,ADVANCE_CLK,READ_REG,WRITE_REG,SET_CLK,START_CLK,STOP_CLK,RESET,CS_HIGH,CS_LOW,EXIT};
	private SIDPiController sid = new SIDPiController();
	public static void main (String args[]) throws IOException {

		SIDMenuConsole console = new SIDMenuConsole();
		console.commandLoop();
		
	}

	private void commandLoop() throws IOException {
		COMMANDS lastCommand = COMMANDS.NOP;
		BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(System.in));
		
		while(lastCommand != COMMANDS.EXIT) {
			displayMenu();
			String line = bufferedReader.readLine();
			if(line.length() > 0) {
				line.toLowerCase();
				Character cmd  = line.charAt(0);
				
				lastCommand = processCommand(cmd);
			}	
		}
	}
	
	private COMMANDS processCommand(char cmd) {
		switch (cmd) {
			case 'c' : {
				setClockSpeed();
				return COMMANDS.SET_CLK;
			}
			case 's' : {
				startClock();
				return COMMANDS.START_CLK;
			}
			case 't' : {
				stopClock();
				return COMMANDS.STOP_CLK;
			}
			case 'e' : {
				reset();
				return COMMANDS.RESET;
			}
			case 'w' : {
				writeReg();
				return COMMANDS.WRITE_REG;
			}
			case 'r' : {
				readReg();
				return COMMANDS.READ_REG;
			}
			case 'h' : {
				highCS();
				return COMMANDS.CS_HIGH;
			}
			case 'l' : {
				lowCS();
				return COMMANDS.CS_LOW;
			}
			case 'x' : {
				return COMMANDS.EXIT;
			}
			default : {
				return COMMANDS.NOP;	
			}
	
		}
		
	}
	private void lowCS() {
		// TODO Auto-generated method stub
		
	}

	private void highCS() {
		// TODO Auto-generated method stub
		
	}

	private void readReg() {
		// TODO Auto-generated method stub
		
	}

	private void writeReg() {
		// TODO Auto-generated method stub
		
	}

	private void reset() {
		// TODO Auto-generated method stub
		
	}

	private void stopClock() {
		// TODO Auto-generated method stub
		
	}

	private void startClock() {
		// TODO Auto-generated method stub
		
	}

	private void setClockSpeed() {
		// TODO Auto-generated method stub
		
	}

	private void displayMenu() {
		System.out.println("Sid Menu");
		System.out.println("========");
		System.out.println("C - Set Clock Speed");
		System.out.println("S - Start clock");
		System.out.println("T - Stop clock");
		System.out.println("A - Advance clock");
		System.out.println("E - Reset SID");
		System.out.println("R - Read register");
		System.out.println("W - Write register");
		System.out.println("H - Set CS High");
		System.out.println("L - Set CD Low");
		System.out.println("X - Exit");
	}
	
}
