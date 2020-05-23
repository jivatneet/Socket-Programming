# Q1: File transfer using multi-channel stop-and-wait protocol


I. Included Files

i. client.c
ii. server.c
iii. packet.h
iv. client (executable binary file)
v. server (executable binary file)


II.Execution Instructions:

i. For compiling and running .c files

1. Open two different terminals
2. Instructions for compiling the files:
	gcc -o client client.c
	gcc -o server server.c
3. Run the server first using ./server followed by client using ./client

ii.For running the binary files client and server- just open two different terminals and run ./server first in one terminal followed by ./client in the second terminal.


III. Input and Output text files

client.c opens and reads input from "input.txt" as mentioned in the assignment.
server.c opens and writes the received data in "destination_file.txt".


IV. Managing two simultaneous channels (TCP connections)

Two client sockets have been created (clientSocket1, clientSocket2) and both initiate a connection with the server. On the server side, one master socket is created which calls accept() repeatedly (inside a while loop) to get a new socket for each client connection. The select() function is used to indicate which of the specified socket descriptors is ready for reading, writing, or has an error condition pending. 
The function FD_ISSET(fd, &fdset) is used to check if the bit for the socket descriptor fd is set in the socket descriptor set pointed to by fdset. 


V. Timers and handling timeouts for the two channels

Timers have been maintained separately for channel 0 and channel 1. The start time is captured as soon as the packet is sent in client using clock() function. When no ack is received within a specified time interval (indicated by the timeval struct passed to select function in client), client checks if the time passed since the last message sent on its channel is >= TIMEOUT. If it is, the packet is retransmitted (its copy was stored when sending) on the same channel.

VI. MACROS used

For submission purposes:
i. Packet size taken as 100 bytes
ii. TIMEOUT taken as 2 s
iii. PDR taken as 10%
iv. Buffer Capacity taken as 10 packets


VII. Handles cases:
i. File size is less than packet size
ii. File size is not a multiple of packet size
iii. Dropping packets when buffer is full


VIII. Additional Comments
For easier reference:
i. Packet drop events have been printed on the server side
ii. TIMEMOUT events have been printed on the client side
