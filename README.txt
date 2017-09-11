		*** Project 2 ***
Follow these steps to test the application:

1. Create two directories
2. place ftserver.c and makefile in one directory
3. place ftclient.py in the other directory
4. Navigate to the directory containing ftserver.c and makefile
5. Run the command 'make ftserver'
6. Once compiled, run the command 'ftserver <port-number> with a number of your choosing
7. Open a duplicate session then navigate to the directory containing ftclient.py
8. Run the command chmod +x ftclient.py to make it executable
9. Run the command 'python ftclient.py <server-host-name><server-port><command><filename><data-port>'
   where <server-host-name> is the name of the host the server is running on (flip1, flip2, or flip3),
   <server-port> is the port number the server is listening on, <command> is either '-l' or '-g',
   <filename> is the requested filename (only enter this if you selected command '-g') and <data-port>
   is the port number you desire the data connection to open on.
