#!/bin/python

'''
* Filename: ftclient.py
* Author: Patrick Dodd
* Description: Sets up a TCP connection with a second host which acts as a server.
	This program is simulating FTP - the client is used to either A) get a list of
	files in the server's directory, or B) transfer  a given file from the server to
	the client.  In order to do this, the user enters a variation of the following
	command: 
	python ftclient.py <server-host-name> <server-port> <command> <filename> <data-port>
	with the <filename> parameter only used if the client is requesting a file transfer.
	The two commands that ther server recognizes are "-l", for list the directory, and 
	"-g", for transfer a file. If "-g" is entered, the server expects to receive a
	filename. After sending its command to the server and receiving the server's response,
	ftclient.py closes the command connection and exits.
* Sources Used: https://docs.python.org/release/2.6.5/library/internet.html
				http://www.pythonforbeginners.com/files/reading-and-writing-files-in-python
'''


from socket import *
import sys



'''
* Function: initiateContact()
* Parameters: N/A
* Description: Initiates the connection with the server. Takes the server's host name and listening port
	as command line parameters and returns the the port/socket info
'''
def initiateContact():
	# Set up the connection
	client = socket(AF_INET, SOCK_STREAM)

	# Get the server host name
	serverHostInfo = gethostbyname(sys.argv[1])

	# Get the server port number
	portNumber = int(sys.argv[2])

	# Bind the address and port number
	address = (serverHostInfo, portNumber)

	return(client, serverHostInfo, portNumber, address)

'''
* Function: makeRequest(command)
* Parameters: command
* Description: Sends the command to the server and receives the server's response
'''
def makeRequest(command):
	
	# Send command/receive response
	client.send(command)
	response = client.recv(1024)

	return response

'''
* Function: receiveFile()
* Parameters: N/A
* Description: Receives the file's contents from the server and returns the complete file contents
	allows up to 400MB to be transferred.
'''
def receiveFile():
	
	# Receive file from the server
	# Allow files up to 400MB
	filecontent = connSocket.recv(400000000)
	return filecontent

'''
* Function: receiveDirectory()
* Parameters: N/A
* Description: Receives the directory contents (up to 1 KiB) from the server and returns the directory
'''
def receiveDirectory():
	dircontents = connSocket.recv(1024)
	return dircontents

'''
* Function: sendDataPort(dataport)
* Parameters: dataport
* Description: Helper function that sends the data port which is the last command line parameter to the server
'''
def sendDataPort(dataport):
	client.send(dataport)

'''
* Function: sendFileName(filename)
* Parameters: filename
* Description: Helper function that sends the name of the desired file to be transferred to the server
'''
def sendFileName(filename):
	client.send(filename)

'''
* Function: setupDataSocket(dataport)
* Parameters: dataport
* Description: Takes the final command line arg as a parameter and creates a socket which the server will
	connect to to create a data connection to enable the server to transfer the directory list and files
	to the client.
'''
def setupDataSocket(dataport):
	
	# Get the port number
	serveport = dataport
	servesocket = socket(AF_INET, SOCK_STREAM)

	return (serveport, servesocket)



# main
if __name__ == "__main__":

	# Error checking
	if len(sys.argv) < 3:
		print "Invalid number or arguments."
		exit(1)

	# Store command
	command = sys.argv[3]

	# If directory contents is requested, get the Data Port
	if len(sys.argv) == 5:
		dataport = sys.argv[4]

	# If file is requested, get the Filename and Data Port
	if len(sys.argv) == 6:
		filename = sys.argv[4]
		dataport = sys.argv[5]

	# Initiate Contact
	client, serverHostInfo, portNumber, address = initiateContact()
	
	# Connect to the server
	client.connect(address)

	# Testing
	print "Connected to the server"

	# Send host info to server
	client.send(serverHostInfo)

	# Receive blank message - prevents too much data being sent at once
	client.recv(1)

	# Send command/receive response
	response = makeRequest(command)

	# Validate return response from server
	# If directory is requested
	if response == "-l":

		print "Setting up data port for directory transfer."

		# Send data port to server
		sendDataPort(sys.argv[4])

		# Set up data connection socket
		serveport, servesocket = setupDataSocket(sys.argv[4])
		
		# Bind the socket to the port
		servesocket.bind(('', int(serveport)))

		# Listen for connection for server
		servesocket.listen(1)

		# Create new socket for incoming connection and acknowlege receipt
		connSocket, address = servesocket.accept()
		print "Received connection at address {}".format(address)

		# Receive the directory from the server
		dircontents = receiveDirectory()
		print dircontents

		# Close the control connection
		client.close()
		exit(0)

	elif response == "-g":
		
		# Testing
		print "Setting up data port for file transfer."

		# Send data port to server
		sendDataPort(sys.argv[5])

		# Send filename to server
		filename = sys.argv[4]
		sendFileName(filename)

		# Set up data connection socket
		serveport, servesocket = setupDataSocket(sys.argv[5])
		
		# Bind the socket to the port
		servesocket.bind(('', int(serveport)))

		# Listen for connection for server
		servesocket.listen(1)

		# Create new socket for incoming connection and acknowlege receipt
		connSocket, address = servesocket.accept()
		print "Received connection at address {}".format(address)

		# Receive data from server
		filecontent = receiveFile()
		
		# Write file content into file
		# Reference: http://www.pythonforbeginners.com/files/reading-and-writing-files-in-python
		file = open(filename, "w")
		file.write(filecontent)
		file.close()

		print "File transfer complete."

		# Close the control connection
		client.close()
		exit(0)

	else:
		print response

		# Close the control connection
		client.close()
		exit(1)