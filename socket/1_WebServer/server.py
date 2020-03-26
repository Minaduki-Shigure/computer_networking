# coding=utf-8
#import socket module
from socket import *
import sys

serverSocket = socket(AF_INET, SOCK_STREAM) 
#Prepare a sever socket 
port = 10086
serverSocket.bind(("127.0.0.1", port))
serverSocket.listen(1)

while True:     
    #Establish the connection    
    print ('Ready to serve...') # No more Python 2     
    connectionSocket, addr = serverSocket.accept()
    try:         
        message = connectionSocket.recv(1024).decode()
        filename = message.split()[1]   
        # print (filename)                       
        f = open(filename[1:])
        outputdata = f.read()
        # print(outputdata)

        #Send one HTTP header line into socket
        response_header = 'HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\n' % (len(outputdata))    
        connectionSocket.send(response_header.encode())

        # print(len(outputdata))
        #Send the content of the requested file to the client
        for i in range(0, len(outputdata)):
            connectionSocket.send(outputdata[i].encode())    # encode is required!!
        
        connectionSocket.close()
    except IOError:
        #Send response message for file not found
        f = open('404.html')
        outputdata = f.read()
        response_header = 'HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\n' % (len(outputdata))
        connectionSocket.send(response_header.encode())

        #Send the content of the requested file to the client
        for i in range(0, len(outputdata)):
            connectionSocket.send(outputdata[i].encode())    # encode is required!!
        
        #Close client socket
        connectionSocket.close()

serverSocket.close()
sys.exit()