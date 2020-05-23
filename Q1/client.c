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
#define TIMEOUT 2 //in seconds

//FTP-client sends file, server ACKs
int main(void)
{

	//step-1: TCP connection between client and server

	//create two sockets for two connections
	int clientSocket1 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket1 == -1) { 
		die("socket");
	}

	int clientSocket2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket2 == -1) { 
		die("socket");
	}

	struct sockaddr_in serverAddress;
	memset (&serverAddress,0,sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	int slen = sizeof(serverAddress);

	//step-2: Attempt a connection
    if(connect(clientSocket1, (struct sockaddr *)&serverAddress, sizeof(serverAddress))<0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

     if(connect(clientSocket2, (struct sockaddr *)&serverAddress, sizeof(serverAddress))<0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

	int state = 0, sentLastpkt = 0, recvdLastAck_c1 = 0, recvdLastAck_c2 = 0;
	int unackedPkts_channel1 = 0, unackedPkts_channel2 = 0; //to ensure no packet left to reach server before closing connection
	int seq_no = 0;
	int channel,nread;
	int offset = 0;
	int rcv_len ;

    struct  timeval t = {0.001, 1000}; //1000us
    clock_t start_c1timer, end_c1timer;
   	clock_t start_c2timer, end_c2timer;
   	double total_c1, total_c2;
   
	fd_set rset;
	int maxfdp1;
	PKT send_pkt, rcv_ack, copy1, copy2;
	

	//clear the descriptor set
	FD_ZERO(&rset);

	//add client sockets to rset
	FD_SET(clientSocket1, &rset);
	FD_SET(clientSocket2, &rset);

	maxfdp1 = max(clientSocket1, clientSocket2) + 1;
	
	//step-3: Open the file the client wants to upload
    FILE *fp = fopen("input.txt","rb");
    if(fp==NULL)
    {
        printf("File open error");
        return 1;   
    }   

	//step-4: Read data from file and send it 
	fseek(fp, offset, SEEK_SET); //offset wrt beginnig of file

	//read file in chunks of PACKET_SIZE
    unsigned char buff[PACKET_SIZE+1]={0};
    nread = fread(buff,1,PACKET_SIZE,fp);
    
    //If read was success, send data
    //send packet through channel 0
    if(nread > 0)
    {
    	unackedPkts_channel1 = 0; 
    	send_pkt.header.seq_no = offset;
    	seq_no = send_pkt.header.seq_no;
    	send_pkt.header.channel = 0;
    	send_pkt.header.size = nread;
    	send_pkt.header.isData = 1;

    	if(feof(fp)){
	    	send_pkt.header.isLastpacket = 1;
			sentLastpkt = 1;
    	}
    	else {
    		send_pkt.header.isLastpacket = 0;
    	}
    	

    	offset += PACKET_SIZE;
    	strcpy(send_pkt.data, buff);
    	copy1 = send_pkt;

        if (send(clientSocket1, &send_pkt, sizeof(send_pkt), 0 )==-1){
	            	die("send()");
		}

	    else{
	    	start_c1timer = clock();
	    }

	    unackedPkts_channel1 = 1;
	    printf("SENT PKT: Seq. No %d of size %d bytes from channel %d\n",seq_no,send_pkt.header.size, send_pkt.header.channel );
    }

    if (nread < PACKET_SIZE)
    {
        if (feof(fp)){
            //printf("End of file\n");
            //return 0;
        }
        if (ferror(fp)){
            printf("Error reading\n");
            return -1;
        }
        
    }

    nread = fread(buff,1,PACKET_SIZE,fp);

    //send packet through channel 1
    if(nread > 0)
    {
    	unackedPkts_channel2 = 0;
    	send_pkt.header.seq_no = offset;
    	seq_no = send_pkt.header.seq_no;
    	send_pkt.header.channel = 1;
    	send_pkt.header.size = nread;
    	send_pkt.header.isData = 1;

    	if(feof(fp)){
	    	send_pkt.header.isLastpacket = 1;
			sentLastpkt = 1;
    	}
    	else {
    		send_pkt.header.isLastpacket = 0;
    	}

    	offset += PACKET_SIZE;
    	strcpy(send_pkt.data, buff);
    	copy2 = send_pkt;

        if (send(clientSocket2, &send_pkt, sizeof(send_pkt), 0)==-1){
	            	die("send()");
		}


	    else{
	    	start_c2timer = clock();
	    }

	    unackedPkts_channel2 = 1;
	    printf("SENT PKT: Seq. No %d of size %d bytes from channel %d\n\n",seq_no,send_pkt.header.size, send_pkt.header.channel );
    }

    if (nread < PACKET_SIZE)
    {
        if (feof(fp)){
            //printf("End of file\n");
            //return 0;
        }
        if (ferror(fp)){
            printf("Error reading\n");
            return -1;
        }
    }
   

	while(1){

		//clear the descriptor set
		FD_ZERO(&rset);

		//add client sockets to rset
		FD_SET(clientSocket1, &rset);
		FD_SET(clientSocket2, &rset);

		maxfdp1 = max(clientSocket1, clientSocket2) + 1;
		int nread = select(maxfdp1, &rset, NULL, NULL, &t);

		if(FD_ISSET(clientSocket1, &rset)){

			rcv_len = recv(clientSocket1, &rcv_ack, sizeof(rcv_ack), 0);
			unackedPkts_channel1 = 0; 
			printf("RCVD ACK: for PKT with Seq. No %d from channel %d\n",rcv_ack.header.seq_no,rcv_ack.header.channel);

			if(rcv_ack.header.isLastpacket == 1)
				recvdLastAck_c1 = 1;

			nread = fread(buff,1,PACKET_SIZE,fp);
	        //If read was success, send data

	        //send packet through socket 1
	        if(nread > 0)
	        {
	        	send_pkt.header.seq_no = offset;
	        	seq_no = send_pkt.header.seq_no;
	        	send_pkt.header.channel = rcv_ack.header.channel;
	        	send_pkt.header.size = nread;
	        	send_pkt.header.isData = 1;

	        	if(feof(fp)){

	        		send_pkt.header.isLastpacket = 1;
	        		sentLastpkt = 1;
	        	}
	        	else {
		    		send_pkt.header.isLastpacket = 0;
		    	}

	        	offset += PACKET_SIZE;
	        	strcpy(send_pkt.data, buff);
	        	copy1 = send_pkt;

	        	//printstruct(send_pkt);
	            if (send(clientSocket1, &send_pkt, sizeof(send_pkt), 0 )==-1){
	            	die("send()");
			    }

			    else{
			    	start_c1timer = clock();
			    }

			    unackedPkts_channel1 = 1;
			    printf("SENT PKT: Seq. No %d of size %d bytes from channel %d\n\n",seq_no,send_pkt.header.size, send_pkt.header.channel );
	        }

	        if ((nread < PACKET_SIZE)  && (unackedPkts_channel1 == 0 && unackedPkts_channel2 == 0))
	        {
	
	             if (feof(fp)){
		            //printf("End of file\n");
		            break;
		            //return 0;
		        }
		        if (ferror(fp)){
		            printf("Error reading\n");
		            return -1;
		        }
	        }

		}

		else{

		
			end_c1timer = clock();
			total_c1 = (double)(end_c1timer - start_c1timer) / CLOCKS_PER_SEC;

			if(recvdLastAck_c1 == 0){
				if(total_c1 >= TIMEOUT){
					
					printf("TIMEOUT: Seq No. %d at channel %d\n",copy1.header.seq_no,copy1.header.channel);
					send(clientSocket1, &copy1, sizeof(copy1), 0);
					start_c1timer = clock();
					printf("SENT PKT: Seq. No %d of size %d bytes from channel %d\n\n",copy1.header.seq_no,copy1.header.size, copy1.header.channel );
				}

				if((unackedPkts_channel2 == 0) && (unackedPkts_channel1 == 0) && sentLastpkt == 1){
					if (feof(fp)){
	                	//printf("End of file\n");
	                	break;
					}
	            	//return 0;
				}

			}

		}

		if(FD_ISSET(clientSocket2, &rset)){

			rcv_len = recv(clientSocket2, &rcv_ack, sizeof(rcv_ack), 0);
			unackedPkts_channel2 = 0;
			printf("RCVD ACK: for PKT with Seq. No %d from channel %d\n",rcv_ack.header.seq_no,rcv_ack.header.channel);

			if(rcv_ack.header.isLastpacket == 1)
				recvdLastAck_c2 = 1;

			nread = fread(buff,1,PACKET_SIZE,fp);
	        //If read was success, send data

	        //send packet through socket 1
	        if(nread > 0)
	        {
	        	send_pkt.header.seq_no = offset;
	        	seq_no = send_pkt.header.seq_no;
	        	send_pkt.header.channel = rcv_ack.header.channel;
	        	send_pkt.header.size = nread;
	        	send_pkt.header.isData = 1;

	        	if(feof(fp)){

	        		send_pkt.header.isLastpacket = 1;
	        		sentLastpkt = 1;
	        	}
	        	else {
		    		send_pkt.header.isLastpacket = 0;
		    	}

	        	offset += PACKET_SIZE;
	        	strcpy(send_pkt.data, buff);
	        	copy2 = send_pkt;

	        	//printstruct(send_pkt);
	            if (send(clientSocket2, &send_pkt, sizeof(send_pkt), 0 )==-1){
	            	die("send()");
			    }

			    else{
			    	start_c2timer = clock();
			    }

			    unackedPkts_channel2 = 1;
			    printf("SENT PKT: Seq. No %d of size %d bytes from channel %d\n\n",seq_no,send_pkt.header.size, send_pkt.header.channel );
	        }

	        if ((nread < PACKET_SIZE) && (unackedPkts_channel1 == 0 && unackedPkts_channel2 == 0))
			{ 	
				if (feof(fp)){
		            //printf("End of file\n");
		            
		        }
		        if (ferror(fp)){
		            printf("Error reading\n");
		            return -1;
		        }
	        }

		}

		else{

			end_c2timer = clock();
			total_c2 = (double)(end_c2timer - start_c2timer) / CLOCKS_PER_SEC;

			if(recvdLastAck_c2 == 0){
				if(total_c2 >= TIMEOUT){
					printf("TIMEOUT: Seq No. %d at channel %d\n",copy2.header.seq_no,copy2.header.channel);
					send(clientSocket2, &copy2, sizeof(copy2), 0);
					start_c2timer = clock();
					printf("SENT PKT: Seq. No %d of size %d bytes from channel %d\n\n",copy2.header.seq_no,copy2.header.size, copy2.header.channel );
				}

				if((unackedPkts_channel1 == 0) && (unackedPkts_channel2 == 0) && sentLastpkt == 1){
					if (feof(fp)){
	                	//printf("End of file\n");
	                	break;
					}
	        
				}

			}

		}
		

	}

	printf("File transfer successful\n");
	fclose(fp);
	return 0;
}

