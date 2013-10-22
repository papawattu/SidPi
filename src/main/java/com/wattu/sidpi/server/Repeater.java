package com.wattu.sidpi.server;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;

import com.wattu.sidpi.ClientContext;

public class Repeater {
	
	public static void main(String[] args) throws IOException {
        
        if (args.length != 1) {
            System.err.println("Usage: java EchoServer <port number>");
            System.exit(1);
        }
         
        /* check for new connections. */
		ServerSocket serverSocket = new ServerSocket(6581);
		Socket clientSocket = serverSocket.accept();
		
		PrintStream out = new PrintStream(clientSocket.getOutputStream());
		BufferedInputStream in = new BufferedInputStream(clientSocket.getInputStream());
		
		Socket sidSocket = new Socket("192.168.1.66", 6581);
		
		PrintStream sidOut = new PrintStream(sidSocket.getOutputStream());
		BufferedInputStream sidIn = new BufferedInputStream(sidSocket.getInputStream());
		
		byte[] buffer = new byte[16384];
		int length =0;
		while(true) {
			if(in.available() > 0) {
				length = in.read(buffer,0,16384);
				sidOut.write(buffer, 0, length);
			}
			if(sidIn.available() > 0) {
				length = sidIn.read(buffer, 0, 16384);
				out.write(buffer, 0, length);
			}
		}

	}
}