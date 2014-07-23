package com.wattu.sidpi.server;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;

public class Repeater implements Runnable {
	
	public static final int SET_THRESHOLD =			15;
	public static final int SET_LATENCY =			16;
	public static final int SET_DELAY_MULTI =		17;
	
	private boolean running;
	private static boolean commandPending = false;
	private enum COMMANDS {LATENCY,THRESHOLD,MULTIPLIER};
	private static byte command = 0;
	private static int value = 0;
	
	public static void main(String[] args) throws IOException {
        
        if (args.length != 1) {
            System.err.println("Usage: java EchoServer <port number>");
            System.exit(1);
        }
        
        Repeater repeater = new Repeater();
        Thread repeaterThread = new Thread(repeater);
        repeaterThread.start();
        
        BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(System.in));
        String line = null;
        char cmd = ' ';
        
        do  {
        	line = bufferedReader.readLine();
        	if(line.length() > 0) {
        		cmd = line.charAt(0);
        		
        		switch (cmd) {
        			case 't': {
        				setThreshold(Integer.parseInt(line.substring(2, line.length())));
        				break;
        			}
        			case 'm': {
        				setMultiplier(Integer.parseInt(line.substring(2, line.length())));
        				break;
        			}
        			case 'l': {
        				setLatency(Integer.parseInt(line.substring(2, line.length())));
        				break;
        			}
        		}
        		cmd = 0;
        	}
        } while (cmd != 'q');
        
        repeater.stop();
        System.exit(0);
	}    
    public void run() {    
        /* check for new connections. */
    	ServerSocket serverSocket = null;
    	Socket clientSocket = null;
    	Socket sidSocket = null;
        try {
        	System.out.println("Opening listening socket");
        	serverSocket = new ServerSocket(6581);
        	
        	running = true;
        	
        	while(running) {
        		clientSocket = serverSocket.accept();
        		clientSocket.setSoTimeout(100);
		
        		System.out.println("Client connected " + clientSocket.getInetAddress().toString());
        	
        		PrintStream out = new PrintStream(clientSocket.getOutputStream());
        		BufferedInputStream in = new BufferedInputStream(clientSocket.getInputStream());
		
        		sidSocket = new Socket("192.168.1.108", 6581);
        		System.out.println("Opening forwaring client socket");
        
        		PrintStream sidOut = new PrintStream(sidSocket.getOutputStream());
        		BufferedInputStream sidIn = new BufferedInputStream(sidSocket.getInputStream());
		
        		byte[] buffer = new byte[16384];
        		int length =0;
        		
        	
        		while(!clientSocket.isClosed()) {
        			try {
        				if(in.available() > 0) {
        					length = in.read(buffer,0,16384);
        					if(length ==-1) throw new EOFException();
        					sidOut.write(buffer, 0, length);
        					if(sidOut.checkError()) throw new IOException(); 
        				}
        				if(sidIn.available() > 0) {
        					length = sidIn.read(buffer, 0, 16384);
        					if(length ==-1) throw new EOFException();
        					out.write(buffer, 0, length);		
        					if(out.checkError()) throw new IOException(); 
        				}
        			} catch (Exception e) {
        				System.out.println("Closing Socket");
        				
        				in.close();
        				clientSocket.shutdownInput();
        				clientSocket.close();
        			}
        		}
        	}
        	sidSocket.close();
          	clientSocket.close();
        	serverSocket.close();
        } catch (IOException ioe) {
        	System.err.println("Error " + ioe);
        	System.exit(1);
        }  
    }
    
    public void stop() {
    	running = false;
    }
    private String printBuf(byte[] buf,int len) {
    	StringBuffer sb = new StringBuffer();
    	for(int i=0;i<len;i++) {
    		sb.append(String.format("%02X ", buf[i])).append(" ");
    	}
    	return sb.toString();
    }
    static void setLatency(int val) {
    	if(Repeater.command == 0) {
    		Repeater.command = SET_THRESHOLD & 0xff;
    		Repeater.value = val;
    	}
    }
    static void setMultiplier(int val) {
    	if(Repeater.command == 0) {
    		Repeater.command = SET_DELAY_MULTI & 0xff;
    		Repeater.value = val;
    	}
    }
    static void setThreshold(int val) {
    	if(Repeater.command == 0) {
    		Repeater.command = SET_THRESHOLD & 0xff;
    		Repeater.value = val;
    	}
    }
}