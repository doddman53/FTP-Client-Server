/******
* Filename: chatclient.c
* Author: Patrick Dodd
* Description: Sets up a TCP connection with a second host which acts as the client.
	The user enters ./ftserver <port-number> where <port-number> is the port which
	ftserver will listen on for a connection. Once the client initiates a connection,
	ftserver takes a command, a data port number, and possibly a filename that are
	passed as parameters from the client on the command line. The two commands that
	ftserver recognizes are "-l" for display the directory, and "-g" for transfer a
	file. If the command "-l" is received, ftserve initiates a second connection with
	the client, dubbed the data connection. ftserver then sends a list of items in its
	directory to the client. If the "-g" command is received, ftserver takes the filename
	while was passed by the client via the command line and searches its directory for
	a matching file. If the file is found, ftserver initiates a data connection just
	like in the previous case and sends the file to the client over the data connection.
	Once ftserver completes either task, it closes the data connection and continues
	to wait for incoming connections from a client on its original listening port. It
	continues to run infinitely until it is manually shut down via SIGINT. 
* Sources Used: Beej's guide and old coursework from CS344 Operating Systems
				https://stackoverflow.com/questions/19117131/get-list-of-file-names-and-store-them-in-array-on-linux-using-c
				https://stackoverflow.com/questions/174531/easiest-way-to-get-files-contents-in-c
******/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues


/*****
* Function: char* receiveCommand(int charsRead, int establishedConnectionFD, char* buffer)
* Parameters: int charsRead, int establishedConnectionFD, char* buffer
* Description: Receives command from client and performs input validation. If it receives
	an invalid command, it returns the error message to the client, causing the client to
	terminate and the server to continue to listen for new connections. If it receives a 
	valid command, it returns either the corresponding "-l" or "-g", which signaling the
	client to act accordingly.
*****/
char* receiveCommand(int charsRead, int establishedConnectionFD, char* buffer) {

	char* error_msg = "Invalid command. Please send \"-l\" or \"-g\".";
	char ok_msg[3];

	charsRead = recv(establishedConnectionFD, buffer, 1024, 0); // Read the client's message from the socket
	if (charsRead < 0) error("ERROR reading from socket");

	// Validate the message
	if (strcmp(buffer, "-l") == 0) {
		strcpy(ok_msg, buffer);
		return ok_msg;
	}
	else if (strcmp(buffer, "-g") == 0) {
		strcpy(ok_msg, buffer);
		return ok_msg;
	}
	else
		return error_msg;
}

/*****
* Function: void sendResponse(int charsRead, int establishedConnectionFD, char* response)
* Parameters: int charsRead, int establishedConnectionFD, char* response
* Description: Helper function that sends the response to the client
*****/
void sendResponse(int charsRead, int establishedConnectionFD, char* response) {
	charsRead = send(establishedConnectionFD, response, strlen(response), 0);
	if (charsRead < 0) error("ERROR writing to socket");
}

/*****
* Function: int dataConnectionInit(int dataport, struct sockaddr_in clientAddress, char* hostname)
* Parameters: int dataport, struct sockaddr_in clientAddress, char* hostname
* Description: When called, initiates a second tcp connection with the client which is used to send
	data - either the server's directory or a specifc file to the client
*****/
int dataConnectionInit(int dataport, struct sockaddr_in clientAddress, char* hostname) {

	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[1024];

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = dataport; // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(hostname); // Convert the machine name into a special form of address

	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	return socketFD;
}

/*****
* Function: void sendDirectory(int charsRead, int establishedConnectionFD, int dataport, struct sockaddr_in clientAddress, char* hostname)
* Parameters: int charsRead, int establishedConnectionFD, int dataport, struct sockaddr_in clientAddress, char* hostname
* Description: Calls dataConnectionInit() to a establish a data connection with the client. Opens and searches the cwd for files, all the while
	concatenating each filename onto a string. Once complete, the string containing the directory's list of filenames is sent
	to the client.
* Reference: https://stackoverflow.com/questions/19117131/get-list-of-file-names-and-store-them-in-array-on-linux-using-c
*****/
void sendDirectory(int charsRead, int establishedConnectionFD, int dataport, struct sockaddr_in clientAddress, char* hostname) {

	// Establish data connection
	establishedConnectionFD = dataConnectionInit(dataport, clientAddress, hostname);

	char name[1024];
	memset(name, '\0', sizeof(name));
	DIR* d;
	struct dirent* dir;
	d = opendir(".");

	if (d) {
		while ((dir = readdir(d)) != NULL) {
			strcat(name, dir->d_name);
			strcat(name, "\n");
		}
		closedir(d);
	}

	// Send the directory list to the client
	charsRead = send(establishedConnectionFD, name, strlen(name), 0);
	if (charsRead < 0) error("ERROR writing to socket");

	// Close the data socket
	close(establishedConnectionFD);
	printf("Data connection closed. Listening for new connection.\n");
}

/*****
* Function: void sendFile(int charsRead, int establishedConnectionFD, int dataport, struct sockaddr_in clientAddress, char* filename, char* hostname)
* Parameters: int charsRead, int establishedConnectionFD, int dataport, struct sockaddr_in clientAddress, char* filename, char* hostname
* Description: Calls dataConnectionInit() to establish a data connection with the client. Once established, the cwd is searched for the passed
	in filename. If it is found, the contents of the file are sent back to the client over the data connection. If the file is not found, an
	error message is displayed.
* Reference: https://stackoverflow.com/questions/174531/easiest-way-to-get-files-contents-in-c
*****/
void sendFile(int charsRead, int establishedConnectionFD, int dataport, struct sockaddr_in clientAddress, char* filename, char* hostname) {

	// Establish data connection
	establishedConnectionFD = dataConnectionInit(dataport, clientAddress, hostname);

	/* variables to control how much data is being sent over the connection:
		 - total: represents the total # of bytes that have been sent
		 - bytesLeft: the total number left of the file to transmit
		 - diff: the difference between bytesLeft and total
		 - tosend: # of bytes to send per iteration
	*/
	int total = 0, bytesLeft = 0, diff = 0, tosend = 1024;
	char* filebuff = 0;
	DIR* d;
	struct dirent* dir;

	FILE* f;
	long length;

	d = opendir(".");

	if (d) {
		while ((dir = readdir(d)) != NULL) {

			if (strcmp(dir->d_name, filename) == 0) {
				f = fopen(dir->d_name, "r");

				if (f) {
					fseek(f, 0, SEEK_END);
					length = ftell(f);
					fseek(f, 0, SEEK_SET);
					filebuff = malloc(length);
					if (filebuff) {
						fread(filebuff, 1, length, f);
					}
					fclose(f);					
				}

				// bytesLeft is initialized to the # of total bytes to be transmitted
				bytesLeft = strlen(filebuff);

				// Send the file to the client
				while (total < bytesLeft) {
					if (bytesLeft < tosend) tosend = bytesLeft;
					charsRead = send(establishedConnectionFD, filebuff, strlen(filebuff), 0);
					total += charsRead;
					diff = bytesLeft - total;
					if (diff < 1024) tosend = diff;
				}
				if (charsRead < 0) error("ERROR writing to socket");
				break;
			}
		}

		// If no matching filename was found, print an error message
		if (filebuff == 0) {
			printf("%s is not a valid filename. Please enter a valid filename.\n", filename);
		}
	}
	closedir(d);

	// Close the data socket
	close(establishedConnectionFD);
	printf("Data connection closed. Listening for new connection.\n");
}

/*****
* Function: startup(struct sockaddr_in serverAddress, struct sockaddr_in clientAddress, int listenSocketFD, socklen_t sizeOfClientInfo, int establishedConnectionFD, int portNumber)
* Parameters: struct sockaddr_in serverAddress, struct sockaddr_in clientAddress, int listenSocketFD, socklen_t sizeOfClientInfo, int establishedConnectionFD, int portNumber
* Description: The primary function of the program. Perfomrs startup duties for the server, and also determines whether to call sendDirectory() or sendFile()
	based on the parameters passed in from the client. startup() is essentially the "Air Traffic Controller" for the application, direction the network
	traffic in the proper direction, and returning any error messages as appropriate.
*****/
void startup(struct sockaddr_in serverAddress, struct sockaddr_in clientAddress, int listenSocketFD, socklen_t sizeOfClientInfo, int establishedConnectionFD, int portNumber) {
	
	serverAddress.sin_family = AF_INET;					// Create network-capable socket
	serverAddress.sin_port = htons(portNumber);			// Store port number
	serverAddress.sin_addr.s_addr = INADDR_ANY;			// Any address is allowed to connect to process

	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); 		// Create Socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Loop until SIGINT
	while (1) {
		char buffer[1024];
		char hostname[1024];
		int dataport;
		int senddirectory = 0;
		int sendfile = 0;

		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(&clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *) &clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		// Display connection info
		printf("Control connection received\n");

		int charsRead;

		// Get server host info from client
		memset(hostname, '\0', sizeof(hostname));
		charsRead = recv(establishedConnectionFD, hostname, sizeof(hostname), 0);

		// Prevent too much info being sent
		charsRead = send(establishedConnectionFD, " ", 1, 0);

		// Get command from client
		memset(buffer, '\0', sizeof(buffer));
		char* response = receiveCommand(charsRead, establishedConnectionFD, buffer);
		
		// If client wants the directory
		if (strcmp(response, "-l") == 0) {
			senddirectory = 1;

			// Send response to client
			sendResponse(charsRead, establishedConnectionFD, response);

			// Get Data Port from client
			memset(buffer, '\0', sizeof(dataport));
			charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
			dataport = atoi(buffer);

			if (senddirectory == 1) {
				printf("Sending directory to client.\n");
				sendDirectory(charsRead, establishedConnectionFD, dataport, clientAddress, hostname);
			}
		}

		// If client wants a file to be transferred
		if (strcmp(response, "-g") == 0) {
			sendfile = 1;

			// Send response to client
			sendResponse(charsRead, establishedConnectionFD, response);

			// Get Data Port from client
			memset(buffer, '\0', sizeof(buffer));
			charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
			dataport = atoi(buffer);

			// Get requested filename from client
			memset(buffer, '\0', sizeof(buffer));
			charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");

			if (sendfile == 1) {
				printf("Sending %s to client.\n", buffer);
				sendFile(charsRead, establishedConnectionFD, dataport, clientAddress, buffer, hostname);

			}
		}

		// If an invalid command is entered, send erro msg to client
		else {
			charsRead = send(establishedConnectionFD, response, strlen(response), 0);
		}
	}
	printf("Server listening for connection on port: %i\n", portNumber);
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string

	// Greeting message
	printf("Server listening for connection on port: %i\n", portNumber);
	fflush(stdout);
	
	// Set up the socket
	startup(serverAddress, clientAddress, listenSocketFD, sizeOfClientInfo, establishedConnectionFD, portNumber);

	close(establishedConnectionFD); // Close the existing socket which is connected to the client
	close(listenSocketFD); // Close the listening socket

	return 0; 
}
