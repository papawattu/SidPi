package com.wattu.sidpi.console;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import com.wattu.sidpi.SIDPiController;
import com.wattu.sidpi.ClientContext;

public class SIDMenuConsole {
	
	private enum COMMANDS {NOP,ADVANCE_CLK,READ_REG,WRITE_REG,SET_CLK,START_CLK,STOP_CLK,RESET,CS_HIGH,CS_LOW,EXIT};
	private static SIDPiController sid;
	private BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(System.in));
	
	public static void main (String args[]) throws IOException {

		sid = new SIDPiController();
		SIDMenuConsole console = new SIDMenuConsole();
		console.commandLoop();
		
	}

	private void commandLoop() throws IOException {
		COMMANDS lastCommand = COMMANDS.NOP;
		
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
	
	private COMMANDS processCommand(char cmd) throws IOException {
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
			case 'a' : {
				advanceClock();
				return COMMANDS.ADVANCE_CLK;
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
			case 'm' : {
				try {
					ClientContext.listenForClients(sid);
				}
				catch (Exception e) {
					System.err.println(e);
				}
				return COMMANDS.NOP;
			}
			default : {
				return COMMANDS.NOP;	
			}
	
		}
		
	}
	private void advanceClock() {
		sid.advanceClock();
	}

	private void lowCS() {
		sid.setCSLow();
		
	}

	private void highCS() {
		sid.setCSHigh();
		
	}

	private void readReg() throws IOException {
		System.out.print("Enter address : ");
		String line = bufferedReader.readLine();
		int address = Integer.parseInt(line);
		
		if(address >= 0 && address < 32) {
			System.out.println("Value for address '" + address + "' is " + sid.readRegister(address));
		} else {
			System.out.println("Error : Invalid input " + line);
		}
		
	}

	private void writeReg() throws IOException {
		System.out.print("Enter address : ");
		String line = bufferedReader.readLine();
		int address = Integer.parseInt(line);
		System.out.print("Enter value : ");
		line = bufferedReader.readLine();
		int value = Integer.parseInt(line);
		
		if(address >= 0 && address < 32 && value < 256 && value >= 0) {
			sid.writeRegister(address,value);
		} else {
			System.out.println("Error : Invalid input " + line);
		}
	}

	private void reset() {
	
		sid.reset();
	}

	private void stopClock() {
		sid.stopClock();
	}

	private void startClock() {
		sid.startClock();
	}

	private void setClockSpeed() throws IOException {
		System.out.print("Enter speed : ");
		String line = bufferedReader.readLine();
		int speed = Integer.parseInt(line);
		sid.setClockSpeed(speed);
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
		System.out.println("L - Set CS Low");
		System.out.println("M - Start Network Listener");
		System.out.println("X - Exit");
		System.out.println("========");
		//System.out.println("Current Clock Speed : " + sid.getClockSpeed() + " is running  : " + sid.isClockRunning());
		//System.out.println("Current Clock Cycle : " + sid.getCurrentCycle());
		
	}
	
}
