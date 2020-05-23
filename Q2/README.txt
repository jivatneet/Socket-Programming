----------Q2:  File transfer using Selective Repeat protocol over UDP-------


I. Included Files

i. client.c
ii. server.c
iii. relay.c
iv. packet.h
iv. client (executable binary file)
v. server (executable binary file)
vi. relay (executable binary file)


II.Execution Instructions:

i. For compiling and running .c files

1. Open four different terminals (for CLIENT, SERVER, RELAY1 and RELAY2)
2. Instructions for compiling the files:
	gcc -o client client.c
	gcc -o server server.c
	gcc -o relay relay.c
3. Run the server first using ./server.
4. To run the relay file, provide command line arguments (1/2) for relay node 1 and 2 respectively
	./relay 1
	./relay 2
5. Run the client at the end using ./client

ii.For running the binary files - just open four different terminals and run ./server first in one terminal followed by relay file (./relay 1 and ./relay 2). Run client at the end using ./client in the second terminal.

[NOTE: A log file (logfile.txt) is created for storing the output of events based on timestamp value. The file is opened in "append" mode so that client, server and relay can append log of their events. Hence, for running the program again, previous logfile.txt needs to be deleted from the folder.]

[NOTE: The output events are also printed separately on the four different consoles.]


III. Input and Output text files

client.c opens and reads input from "input.txt" as mentioned in the assignment.
server.c opens and writes the received data in "destination_file.txt".


IV. Handling acknowledgements for the packets in the window

A receiveAck flag has been maintained which sets the corresponding bits according to the sequence no. of the packet acknowledged. As soon as the window slides to send the next packet(if acknowledgement for the base packet is received), it resets the corresponding bit to 0. Also, track has been kept of the previous unacknowledged packets, for eg, if packet 1 and 2 have been acked and then we receive ack for packet 0, three packets will be sent together by the client.

V. Managing multiple Timers

Timers have been maintained for every packet in the window using the 'startTime' field of type clock_t in the packet structure. When no ack is received within a specified time interval (indicated by the timeval struct passed to select function in client), client starting from the base packet of the window checks if TIMEOUT has occurred for any packet. If it has, client retransmits that packet.


VI. SEQ. NO and PKT NO.

The seq. no indicates the position of the packet in the window and hence the sequence numbers are in the range [0, WINDOW SIZE -1]. The packet numbers indicate the overall sequence of the packets. Both of them have been printed in the log for easier reference.


VII. MACROS USED
For submission purposes:
i. Packet size taken as 100 bytes
ii. Window size taken as 8
ii. TIMEOUT taken as 2 s
iii. PDR taken as 10%
iv. Buffer Capacity taken as 10 packets


VII. Handles cases:
i. The total number of packets is less than WINDOW SIZE
ii. File size is not a multiple of packet size
iii. Dropping packets when buffer is full


VIII. Additional Comments
For easier reference:
i. SEQ NO. and PKT NO. both have been printed on the console
ii. A log file is also created for maintaining record of events based on timestamp 