package com.wattu.sidpi.server;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;

public class Repeater implements Runnable {
	
	private boolean running;
	
	public static void main(String[] args) throws IOException {
        
        if (args.length != 1) {
            System.err.println("Usage: java EchoServer <port number>");
            System.exit(1);
        }
        
        Repeater repeater = new Repeater();
        Thread repeaterThread = new Thread(repeater);
        repeaterThread.start();
        
        BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(System.in));
        
        while(bufferedReader.readLine().length()==0);
        
        repeater.stop();
	}    
    public void run() {    
        /* check for new connections. */
    	ServerSocket serverSocket = null;
    	Socket clientSocket = null;
    	Socket sidSocket = null;
        try {
        	System.out.println("Opening listening socket");
        	serverSocket = new ServerSocket(6581);
        	clientSocket = serverSocket.accept();
		
        	System.out.println("Client connected");
        	
        	PrintStream out = new PrintStream(clientSocket.getOutputStream());
        	BufferedInputStream in = new BufferedInputStream(clientSocket.getInputStream());
		
        	sidSocket = new Socket("192.168.1.66", 6581);
        	System.out.println("Opening forwaring client socket");
        
        	PrintStream sidOut = new PrintStream(sidSocket.getOutputStream());
        	BufferedInputStream sidIn = new BufferedInputStream(sidSocket.getInputStream());
		
        	byte[] buffer = new byte[16384];
        	int length =0;
        	running = true;
        	while(running) {
        		if(in.available() > 0) {
        			length = in.read(buffer,0,16384);
        			sidOut.write(buffer, 0, length);
        		}
        		if(sidIn.available() > 0) {
        			length = sidIn.read(buffer, 0, 16384);
        			out.write(buffer, 0, length);
        			
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
    
}