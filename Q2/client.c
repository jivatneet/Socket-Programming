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
#define WINDOW_SIZE 8
#define TIMEOUT 2 // 2 seconds

int main(void){

	FILE * out  = fopen("logfile.txt","a");
	fprintf(out, "NODE NAME  EVENT TYPE     TIMESTAMP\t    PACKET TYPE\t SEQ. NO   PKT NO\tSOURCE\t\t DEST\n\n");
    fflush(out);
	printf("NODE NAME   EVENT TYPE    TIMESTAMP\t    PACKET TYPE\t    SEQ. NO   PKT NO\tSOURCE\t\t DEST\n\n");
	//step-1: create sockets for UDP communication between client and relay nodes R1, R2

	int clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == -1) { 
		die("socket");

	}

	struct sockaddr_in R1address, R2address, saddr;
	memset (&R1address,0,sizeof(R1address));
	memset (&R2address,0,sizeof(R2address));

	R1address.sin_family = AF_INET;
	R1address.sin_port = htons(PORT_R1);
	R1address.sin_addr.s_addr = inet_addr("127.0.0.1");
	R2address.sin_family = AF_INET;
	R2address.sin_port = htons(PORT_R2);
	R2address.sin_addr.s_addr = inet_addr("127.0.0.1");
	int slen = sizeof(R1address);


	FILE *fp = fopen("input.txt","rb");
    if(fp==NULL)
    {
        printf("File open error");
        return 1;   
    }   

    struct  timeval t = {0.001, 1000}; //1000us
    clock_t start_windowtimer, end_windowtimer;
   	double total_time;
   	time_t timestamp;
   	struct tm *ptm;

    fd_set rset;
    PKT send_pkt, rcv_ack;
    int maxfdp1, offset = 0, recv_len =0, sentLastpkt = 0;

	//step-2: Read data from file  
	fseek(fp, offset, SEEK_SET); //offset wrt beginning of file

	PKT window[WINDOW_SIZE]; //window of packets
	int windowFilled = 0;
	int packetno = 0; //see even/ odd
	int resetWindow = 1, exitClient = 0;
	int rcvAckFlag = 0, unackedPkts = 0, nread = 0, whichPktAcked = 0, packetNumber = 0;
	unsigned char buff[PACKET_SIZE+1]={0};

	//make initial window to be sent in the beginning
	for(int i=0; i<WINDOW_SIZE; i++){

		windowFilled ++;

		nread = fread(buff,1,PACKET_SIZE,fp);
		buff[nread] = '\0';

	    //If read was success, send data
	    if(nread > 0)
	    {
	    	//unackedPkts_channel1 = 0; 
	    	send_pkt.header.seq_no = packetno ++;
	    	send_pkt.header.seq_in_window =i;

	    	if((send_pkt.header.seq_in_window%2) == 0)
	    		send_pkt.header.relayNode = 2;	//R2 for even no packets
	    	else
	    		send_pkt.header.relayNode = 1;  //R1 for odd no packets
	    	send_pkt.header.size = nread;
	    	send_pkt.header.isData = 1;

	    	offset += 1;
	    	strcpy(send_pkt.data, buff);

	    	unackedPkts ++;

	    	if(feof(fp)){

        		send_pkt.header.isLastPacket = 1;
        		sentLastpkt = 1;
        		window[i] = send_pkt;
        		break;
        	}
        	else {
	    		send_pkt.header.isLastPacket = 0;
	    	}

	    	window[i] = send_pkt;
	    	
	    }

	    if (nread < PACKET_SIZE)
	    {
	        if (feof(fp)){
	            ;//printf("Read complete file\n");
	            
	        }
	        if (ferror(fp)){
	            printf("Error reading\n");
	            return -1;
	        }
	    }   	
	}

	//send complete window
	for(int i =0; i<windowFilled; i++){
 
		window[i].header.startTime = clock(); //timer started when packet sent
		send_pkt = window[i];

		if(send_pkt.header.relayNode == 1)
			saddr = R1address;

		else
			saddr = R2address;

		if (sendto(clientSocket, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &saddr, slen)==-1)
    	{
        	  die("sendto()");
    	}

	    timestamp = time(NULL);
	    ptm = localtime(&timestamp);
		printf("CLIENT\t\tS\t%s\t\tDATA\t\t%d\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window, send_pkt.header.seq_no,send_pkt.header.relayNode);
		fprintf(out, "CLIENT\t\t\tS\t\t%s\t\tDATA\t\t%d\t\t%d\t\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
		//printf("CLIENT\t\tS\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.relayNode);
		fflush(out);

	}

	int base = 0;

	while(1){

		//read file in chunks of PACKET_SIZE
	    unsigned char buff[PACKET_SIZE+1]={0};

		//clear the descriptor set
		FD_ZERO(&rset);

		//add client sockets to rset
		FD_SET(clientSocket, &rset);

		maxfdp1 = clientSocket;
		int nread = select(maxfdp1 +1, &rset, NULL, NULL, &t);

		if(FD_ISSET(clientSocket, &rset)){ //receive ACK from relay node

			if ((recv_len = recvfrom(clientSocket, &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &saddr, &slen)) == -1)
         	{
            	die("recvfrom()");
         	}

         	unackedPkts -- ;
         	whichPktAcked = rcv_ack.header.seq_in_window;

         	if(rcv_ack.header.seq_no >= packetNumber)
         		packetNumber = rcv_ack.header.seq_no; //highest acked pkt

         	rcvAckFlag |= (1 << whichPktAcked);

			timestamp = time(NULL);
			ptm = localtime(&timestamp);
			printf("CLIENT\t\tR\t%s\t\tACK\t\t%d\t%d\tRELAY%d\t\tCLIENT\n",returnTime(ptm),rcv_ack.header.seq_in_window, rcv_ack.header.seq_no,rcv_ack.header.relayNode);
			fprintf(out, "CLIENT\t\t\tR\t\t%s\t\tACK\t\t\t%d\t\t%d\t\tRELAY%d\t\tCLIENT\n",returnTime(ptm),rcv_ack.header.seq_in_window, rcv_ack.header.seq_no,rcv_ack.header.relayNode);
			//printf("CLIENT\t\tR\t%s\t\tACK\t\t%d\tRELAY%d\t\tCLIENT\n",returnTime(ptm),rcv_ack.header.seq_in_window, rcv_ack.header.relayNode);
			fflush(out);

			if(sentLastpkt == 1){  //exit from client if all packets in window Acked

				int flag = 0,limitwind = 0;

				if(windowFilled < WINDOW_SIZE)
					limitwind = windowFilled;
				else limitwind = WINDOW_SIZE;

				for(int i=0; i<limitwind; i++){
					if(((1<<i) & rcvAckFlag) == 0){
						flag = 1;
						break;
					}
				}

				if(flag == 0)
					break;

			}

			int prevPktsAcked = 1;

			int checkposn = base % WINDOW_SIZE;
			if(((1<<checkposn) & rcvAckFlag) == 0){	//ith packet unacked
					prevPktsAcked = 0;
			}


			if(prevPktsAcked == 1){

				int prevbase = base, count = 0;

				int temp = rcvAckFlag;

				for(int i=base; i<=packetNumber; i++){
					int shift = i%WINDOW_SIZE;
					if(((temp >> shift) & 1) == 1){
						count++;
					}

					else
						break;
					temp = rcvAckFlag;
				}

				if(count >= 1){
					base = base + count;
				}

				int sq;
				for(sq = prevbase ; sq <= (prevbase + count -1) ; sq++){

					nread = fread(buff,1,PACKET_SIZE,fp);
					buff[nread] = '\0';

				    //If read was success, send data
				    if(nread > 0)
				    {
				    	//unackedPkts_channel1 = 0; 
				    	int index = sq % WINDOW_SIZE;
				    	send_pkt.header.seq_no = packetno ++;
				    	send_pkt.header.seq_in_window = index;

				    	if((send_pkt.header.seq_in_window % 2) == 0)
				    		send_pkt.header.relayNode = 2;	//R2 for even no packets
				    	else
				    		send_pkt.header.relayNode = 1;  //R1 for odd no packets
				    	send_pkt.header.size = nread;
				    	send_pkt.header.isData = 1;

				    	if(feof(fp)){

			        		send_pkt.header.isLastPacket = 1;
			        		sentLastpkt = 1;
			        	}
			        	else {
				    		send_pkt.header.isLastPacket = 0;
				    	}

				    	offset += 1;
				    	strcpy(send_pkt.data, buff);

				    	unackedPkts ++;

				    	window[index] = send_pkt;

				    	window[index].header.startTime = clock(); //timer started when packet sent
						send_pkt = window[index];

						if(send_pkt.header.relayNode == 1)
							saddr = R1address;
						else
							saddr = R2address;

						if (sendto(clientSocket, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &saddr, slen)==-1)
						{
							  die("sendto()");
						}

						timestamp = time(NULL);
						ptm = localtime(&timestamp);
						printf("CLIENT\t\tS\t%s\t\tDATA\t\t%d\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
						fprintf(out, "CLIENT\t\t\tS\t\t%s\t\tDATA\t\t%d\t\t%d\t\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
						//printf("CLIENT\t\tS\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.relayNode);
						fflush(out);

						rcvAckFlag &= (~(1 << index)); //re-setting the ack bit in rcvAckFlag
				    	
				    }

				    if (nread < PACKET_SIZE)
				    {
				        if (feof(fp)){
				           ; //printf("Read complete file\n");
				          
				        }
				        if (ferror(fp)){
				            printf("Error reading\n");
				            return -1;
				        }
				    }

				}

			}
		}

		else{ //check pkt drop and timeout

			end_windowtimer = clock();
			for(int i = base; i<(base + WINDOW_SIZE) ; i++){ //check which packets unacked

				int index = i % WINDOW_SIZE;
				if(((1<<index) & rcvAckFlag) == 0){ //ith packet unacked
					send_pkt = window[index];
					total_time = (double)(end_windowtimer - send_pkt.header.startTime) / CLOCKS_PER_SEC;

					
					if(total_time >= TIMEOUT){	//retransmit ith packet

						timestamp = time(NULL);
						ptm = localtime(&timestamp);
						printf("CLIENT\t\tTO\t%s\t\tDATA\t\t%d\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
						fprintf(out, "CLIENT\t\t\tTO\t\t%s\t\tDATA\t\t%d\t\t%d\t\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
						//printf("CLIENT\t\tTO\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.relayNode);
						fflush(out);

						if(send_pkt.header.relayNode == 1)
							saddr = R1address;
						else
							saddr = R2address;
				        if (sendto(clientSocket, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &saddr, slen)==-1)
			        	{
			            	  die("sendto()");
			        	}


			        	window[index].header.startTime = clock(); //timer re-started when packet retransmitted
						send_pkt = window[index];

						
					    timestamp = time(NULL);
					    ptm = localtime(&timestamp);
					    
						printf("CLIENT\t\tRE\t%s\t\tDATA\t\t%d\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
						fprintf(out, "CLIENT\t\t\tRE\t\t%s\t\tDATA\t\t%d\t\t%d\t\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.seq_no,send_pkt.header.relayNode);
						//printf("CLIENT\t\tRE\t%s\t\tDATA\t\t%d\tCLIENT\t\tRELAY%d\n",returnTime(ptm), send_pkt.header.seq_in_window,send_pkt.header.relayNode);
						fflush(out);
					}

				}

				break;
			}	
			
		}
	}
}