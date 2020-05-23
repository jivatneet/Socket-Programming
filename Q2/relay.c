#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include "packet.h"
#define MAXPENDING 10 //queue limit

int main(int argc, char *argv[])
{

	FILE * out  = fopen("logfile.txt","a");

	char *port = argv[1];
	int relay = 0;

	//int PORTC, PORTS;
	int PORT;
	if(strcmp(port,"1") == 0){ //relayNode R1
		PORT = PORT_R1;
		relay = 1;
	}

	if(strcmp(port,"2") == 0){ //relayNode R2
		PORT = PORT_R2;
		relay = 2;
	}

	struct sockaddr_in relayAddress, clientAddress;
	int slen = sizeof(clientAddress);
    int recv_len;
    fd_set rset; //set of socket descriptors - read and write both at relay node


    //step-1: create a UDP socket
    int relaySocket = 0;

    relaySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(relaySocket == -1){
    	die("socket");
    }
    
    int op = 1;
    if( setsockopt(relaySocket, SOL_SOCKET, SO_REUSEADDR, (char *)&op, sizeof(op)) < 0 )   
    {   
        die("setsockopt");   
    }  

    //relay node receives packet from client
    memset((char *) &relayAddress, 0, sizeof(relayAddress));
     
    relayAddress.sin_family = AF_INET;
    relayAddress.sin_port = htons(PORT);
    relayAddress.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //step-2: bind socket to port
    if( bind( relaySocket, (struct sockaddr*)&relayAddress, sizeof(relayAddress) ) == -1)
    {
        die("bind");
    }
	
    //relay node sends packet to server
	struct sockaddr_in serverAddress, saddr;
	memset (&serverAddress,0,sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORTS); //where relay node connected to server
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

	struct  timeval t = {50, 0}; //10s
	time_t timestamp;
    struct tm *ptm;
    PKT rcv_pkt, rcv_ack;
    PKT send_pkt, send_ack;
    srand(time(0));
    int sleepDuration = 0, pktDrop = 0;

	printf("NODE NAME   EVENT TYPE    TIMESTAMP\t    PACKET TYPE\t    SEQ. NO   PKT NO\tSOURCE\t\t DEST\n\n");

	while(1){

		//clear the descriptor set
		FD_ZERO(&rset);

		//add relay socket to rset
		FD_SET(relaySocket, &rset);

    	int fdp1, nready, maxfdp1;

    	maxfdp1 = relaySocket;

    	//select the ready descriptor
		nready = select(maxfdp1 +1, &rset, NULL, NULL, NULL); //timeout NULL - wait indefinitely

		if(nready == 0)
			break;

		if ((nready < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }  
		
        if(FD_ISSET(relaySocket, &rset)){	//receives packet

        	if ((recv_len = recvfrom(relaySocket, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &saddr, &slen)) == -1)
             {
                die("recvfrom()");
             }

             if(recv_len == 0){
					printf("File transfer successful\n");
					break;
			}

			if(rcv_pkt.header.isData == 1){ //received packet from client

				clientAddress = saddr;
				// printf("RELAY%d\t\tR\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm),rcv_pkt.header.seq_no, rcv_pkt.header.relayNode);
				// fprintf(out, "RELAY%d\t\tR\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.relayNode);


				int drop = rand()%101; //random no between [1,100]

				if(drop <= PDR){

					//printf("packet dropping %d\n",rcv_pkt.header.seq_no);
					timestamp = time(NULL);
					ptm = localtime(&timestamp);
					printf("RELAY%d\t\tD\t%s\t\tDATA\t\t%d\t%d\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm), rcv_pkt.header.seq_in_window, rcv_pkt.header.seq_no,rcv_pkt.header.relayNode);
					fprintf(out, "RELAY%d\t\t\tD\t\t%s\t\tDATA\t\t%d\t\t%d\t\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm), rcv_pkt.header.seq_in_window,rcv_pkt.header.seq_no, rcv_pkt.header.relayNode);
					fflush(out);
					//printf("RELAY%d\t\tD\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm), rcv_pkt.header.seq_in_window,rcv_pkt.header.relayNode);
					pktDrop = 1;
				}

				if(pktDrop == 0){

					timestamp = time(NULL);
					ptm = localtime(&timestamp);
					printf("RELAY%d\t\tR\t%s\t\tDATA\t\t%d\t%d\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm),rcv_pkt.header.seq_in_window,rcv_pkt.header.seq_no, rcv_pkt.header.relayNode);
					fprintf(out, "RELAY%d\t\t\tR\t\t%s\t\tDATA\t\t%d\t\t%d\t\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.seq_no,rcv_pkt.header.relayNode);
					fflush(out);
					//printf("RELAY%d\t\tR\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.relayNode);

					send_pkt = rcv_pkt; //send packet to server

					sleepDuration = rand()%2001; //random delay between [0,2] milliseconds (2000 us)
					
					usleep(sleepDuration);

					if (sendto(relaySocket, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &serverAddress, slen)==-1)
		        	{
		            	 die("sendto()");
		        	}

		        	timestamp = time(NULL);
					ptm = localtime(&timestamp);
		        	printf("RELAY%d\t\tS\t%s\t\tDATA\t\t%d\t%d\tRELAY%d\t\tSERVER\n",send_pkt.header.relayNode,returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
		        	fprintf(out, "RELAY%d\t\t\tS\t\t%s\t\tDATA\t\t%d\t\t%d\t\tRELAY%d\t\tSERVER\n",send_pkt.header.relayNode,returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
		        	fflush(out);
		        	//printf("RELAY%d\t\tR\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",rcv_pkt.header.relayNode,returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.relayNode);
	        	}

	        	pktDrop = 0;

			}


			else{ //received ACK from server

				rcv_ack = rcv_pkt;
				timestamp = time(NULL);
				ptm = localtime(&timestamp);
				printf("RELAY%d\t\tR\t%s\t\tACK\t\t%d\t%d\tSERVER\t\tRELAY%d\n",rcv_ack.header.relayNode,returnTime(ptm),rcv_ack.header.seq_in_window,rcv_ack.header.seq_no, rcv_ack.header.relayNode);
				fprintf(out,"RELAY%d\t\t\tR\t\t%s\t\tACK\t\t\t%d\t\t%d\t\tSERVER\t\tRELAY%d\n",rcv_ack.header.relayNode,returnTime(ptm),rcv_ack.header.seq_in_window, rcv_ack.header.seq_no,rcv_ack.header.relayNode);
				fflush(out);
				//printf("RELAY%d\t\tR\t%s\t\tACK\t\t%d\tSERVER\t\tRELAY%d\n",rcv_ack.header.relayNode,returnTime(ptm),rcv_ack.header.seq_in_window, rcv_ack.header.relayNode);

				send_ack = rcv_ack; //send ack to client
				if (sendto(relaySocket, &send_ack, sizeof(send_ack), 0 , (struct sockaddr *) &clientAddress, slen)==-1)
	        	{
	            	 die("sendto()");
	        	}

	        	timestamp = time(NULL);
				ptm = localtime(&timestamp);
	        	printf("RELAY%d\t\tS\t%s\t\tACK\t\t%d\t%d\tRELAY%d\t\tCLIENT\n",send_ack.header.relayNode,returnTime(ptm), send_ack.header.seq_in_window,send_ack.header.seq_no,send_ack.header.relayNode);
	        	fprintf(out, "RELAY%d\t\t\tS\t\t%s\t\tACK\t\t\t%d\t\t%d\t\tRELAY%d\t\tCLIENT\n",send_ack.header.relayNode,returnTime(ptm), send_ack.header.seq_in_window,send_ack.header.seq_no,send_ack.header.relayNode);
	        	fflush(out);
	        	//printf("RELAY%d\t\tS\t%s\t\tACK\t\t%d\tRELAY%d\t\tCLIENT\n",send_ack.header.relayNode,returnTime(ptm), send_ack.header.seq_in_window,send_ack.header.relayNode);


			}
			
        }

	}
	
	close(relaySocket);
	return 0;
}
