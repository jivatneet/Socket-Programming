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
#define BUFFER_CAPACITY 10

//FTP-client sends file, server ACKs
int main(void)
{
	int serverSocket1 = 0 , serverSocket2 = 0;
	int masterSocket, newSocket;
	fd_set rset; //set of socket descriptors

	 //set buffer size based on size of input.txt 
    FILE *fpin = fopen("input.txt","r");
    if(fpin==NULL){
    	printf("input.txt not found\n");
    	return -1;
    }

    fseek(fpin, 0L, SEEK_END);
    long int inp_size = ftell(fpin);
    fclose(fpin);

    inp_size = inp_size/1000; //decide
    //int BUFFER_CAPACITY = inp_size;

    receiveBuffer receive_buffer[BUFFER_CAPACITY];
    int buffFilled = 0, drop_seq_no1 = 0, drop_seq_no2 = 0;

	//step-1: TCP connection between client and server

	//create server socket
	masterSocket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (masterSocket == -1) { 
			die("socket");
	}

	//set master socket to allow multiple connections 
	int op = 1;
    if( setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&op, sizeof(op)) < 0 )   
    {   
        die("setsockopt");   
    }   

	//construct server address struture
	struct sockaddr_in serverAddress, clientAddress;
	memset (&serverAddress,0,sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	int slen = sizeof(serverAddress);

	//step-2: bind master socket to port
    if( bind(masterSocket , (struct sockaddr*)&serverAddress, sizeof(serverAddress) ) == -1)
    {
        die("bind");
    }
	
	//listen
	if(listen(masterSocket, MAXPENDING) == -1)
    {
        die("listen");
    }

    //step-4: Create file where uploaded data will be stored 
    FILE *fp;     
    fp = fopen("destination_file.txt", "wb"); 
    if(NULL == fp)
    {
      printf("Error opening file");
        return 1;
    }

    srand(time(0));
    int count = 0, pktDrop = 0;
    int seq_no, offset = 0;
    PKT send_ack, rcv_pkt;	

    while(1){
    	 

    	unsigned char recvBuff[PACKET_SIZE]={0};

    	//clear the descriptor set
    	FD_ZERO(&rset);

    	//add master socket to rset
    	FD_SET(masterSocket, &rset);
    	int fdp1, nready, maxfdp1 = masterSocket;

    	fdp1 = serverSocket1;
    	if(fdp1 >0 )
    		FD_SET(fdp1, &rset);

    	fdp1 = serverSocket2;
    	if(fdp1 >0 )
    		FD_SET(fdp1, &rset);

    	maxfdp1 = max(serverSocket2, max(serverSocket1, maxfdp1));


    	//select the ready descriptor
		nready = select(maxfdp1 +1, &rset, NULL, NULL, NULL); //timeout NULL - wait indefinitely

		if ((nready < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }  

        //masterSocket is an incoming connection  
        if (FD_ISSET(masterSocket, &rset))   
        {   

            if ((newSocket = accept(masterSocket,(struct sockaddr *)&serverAddress, (socklen_t*)&slen))<0)   
            {   
                die("accept");   
            }   

            if(serverSocket1 == 0)
            	serverSocket1 = newSocket;
            else if(serverSocket2 == 0)
            	serverSocket2 = newSocket; 
        }


        //if serverSocket1 readable, accept connection
		if(FD_ISSET(serverSocket1, &rset)){

			int clientLength = sizeof(clientAddress);
			//int clientSocket = accept(serverSocket1, (struct sockaddr*)&clientAddress ,&clientLength);
			
			if (clientLength < 0) {
				printf ("Error in client socket"); 
				exit(0);
			}

			int recv_len = recv(serverSocket1, &rcv_pkt, sizeof(rcv_pkt), 0);

			if(recv_len == 0){
				printf("File transfer successful\n");
				break;
			}

				

			int drop = rand()%101; //random no. in range [1,100]

			if(drop <= PDR){ //packet dropped

				printf("PKT DROP: Seq. No %d from channel %d\n\n",rcv_pkt.header.seq_no,rcv_pkt.header.channel );
				drop_seq_no1 = rcv_pkt.header.seq_no;
			}

			else{

				
				printf("RCVD PKT: Seq. No %d of size %d bytes from channel %d\n",rcv_pkt.header.seq_no, rcv_pkt.header.size, rcv_pkt.header.channel);
				//packet in order
				if((rcv_pkt.header.seq_no == offset) ||(rcv_pkt.header.seq_no == drop_seq_no1)){
					
					fwrite(rcv_pkt.data, 1, rcv_pkt.header.size, fp);
					
					if(buffFilled > 0){
						
						int i;
						for(i=0; i < buffFilled ;i++){
							fwrite(receive_buffer[i].buffer, 1, receive_buffer[i].size, fp);
						}

						buffFilled = 0;
					}

					pktDrop = 0;
				}

				//buffer if out of order
				else{

					if(buffFilled == (BUFFER_CAPACITY)){
						drop_seq_no1 = rcv_pkt.header.seq_no;
						pktDrop = 1;	//drop further packets because buffer full

					}

					else{
						buffFilled ++;
						receive_buffer[buffFilled-1].seq_no = rcv_pkt.header.seq_no;
						receive_buffer[buffFilled-1].size = rcv_pkt.header.size;
						strcpy(receive_buffer[buffFilled-1].buffer, rcv_pkt.data);
					}

				}

				if(pktDrop == 0){
					send_ack.header.seq_no = rcv_pkt.header.seq_no;
					seq_no = send_ack.header.seq_no;
					offset += PACKET_SIZE;
					send_ack.header.isData = 0;
					send_ack.header.channel = rcv_pkt.header.channel;
					send_ack.header.isLastpacket = rcv_pkt.header.isLastpacket;
						
					if (send(serverSocket1,&send_ack, sizeof(send_ack), 0)==-1){
	            		die("send()");
			    	}
				
					printf("SENT ACK: for PKT with Seq. No %d from channel %d\n\n",seq_no,send_ack.header.channel);
				}
			
			}

		}

		//if serverSocket2 readable, accept connection
		if(FD_ISSET(serverSocket2, &rset)){

			int clientLength = sizeof(clientAddress);
			
			if (clientLength < 0) {
				printf ("Error in client socket"); 
				exit(0);
			}


			int recv_len = recv(serverSocket2, &rcv_pkt, sizeof(rcv_pkt), 0);
			if(recv_len == 0){
				printf("File transfer successful\n");
				break;
			}


			int drop = rand()%101; //random no. in range [1,100]

			if(drop <= PDR){ //packet dropped

				printf("PKT DROP: Seq. No %d from channel %d\n\n",rcv_pkt.header.seq_no,rcv_pkt.header.channel );
				drop_seq_no2 = rcv_pkt.header.seq_no;

			}

			else{

				printf("RCVD PKT: Seq. No %d of size %d bytes from channel %d\n",rcv_pkt.header.seq_no, rcv_pkt.header.size, rcv_pkt.header.channel);

				//packet in order
				if((rcv_pkt.header.seq_no == offset) || (rcv_pkt.header.seq_no == drop_seq_no2)){
					
					fwrite(rcv_pkt.data, 1, rcv_pkt.header.size, fp);
					
					if(buffFilled > 0){
						
						int i;
						for(i=0; i < buffFilled; i++){
							
							fwrite(receive_buffer[i].buffer, 1, receive_buffer[i].size, fp);
						}

						buffFilled = 0;
					}

					
					pktDrop = 0;
				}

				//buffer if out of order
				else{

					if(buffFilled == (BUFFER_CAPACITY)){
						drop_seq_no2 = rcv_pkt.header.seq_no;
						pktDrop = 1;	//drop further packets because buffer full

					}

					else{
						
						buffFilled ++;

						receive_buffer[buffFilled-1].seq_no = rcv_pkt.header.seq_no;
						receive_buffer[buffFilled-1].size = rcv_pkt.header.size;
						strcpy(receive_buffer[buffFilled-1].buffer, rcv_pkt.data);
					}

				}

				if(pktDrop == 0){

					send_ack.header.seq_no = rcv_pkt.header.seq_no;
					seq_no = send_ack.header.seq_no;
					offset += PACKET_SIZE;
					send_ack.header.isData = 0;
					send_ack.header.channel = rcv_pkt.header.channel;
					send_ack.header.isLastpacket = rcv_pkt.header.isLastpacket;
						
					if (send(serverSocket2,&send_ack, sizeof(send_ack), 0)==-1){
	            		die("send()");
			    	}
				
					printf("SENT ACK: for PKT with Seq. No %d from channel %d\n\n",seq_no,send_ack.header.channel);
				}
			}

		}

 	}
 	
 	close(newSocket);
 	fclose(fp);
	return 0;
	
}

