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


int main(void){

    FILE * out  = fopen("logfile.txt","a");

	struct sockaddr_in serverAddress, clientAddress;
	int slen = sizeof(clientAddress);
    int recv_len;

    //step-1: create a UDP socket
    int serverSocket = 0;
	fd_set rset; //set of socket descriptors

    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(serverSocket == -1){
    	die("socket");
    }

    int op = 1;
    if( setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&op, sizeof(op)) < 0 )   
    {   
        die("setsockopt");   
    }  

    memset((char *) &serverAddress, 0, sizeof(serverAddress));
     
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORTS);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //step-2: bind socket to port
    if( bind( serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress) ) == -1)
    {
        die("bind");
    }

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

    receiveBuffer receive_buffer[BUFFER_CAPACITY];

    for(int i=0; i<BUFFER_CAPACITY; i++){
        receive_buffer[i].seq_no = 0;
        receive_buffer[i].isLastPacket = 0;
        receive_buffer[i].size = 0;
    }

    int buffFilled = 0, lastPktWritten = 0;

    PKT rcv_pkt, send_ack;
    int writeFromBuffer = 0;
    int offset = 0, pktDrop = 0, drop_seq_no = 0, firstDrop = INT_MAX;

    FILE *fp;     
    fp = fopen("destination_file.txt", "wb"); 
    if(NULL == fp)
    {
      printf("Error opening file");
        return 1;
    }

    time_t timestamp;
    struct tm *ptm;

    printf("\nNODE NAME   EVENT TYPE    TIMESTAMP\t    PACKET TYPE\t    SEQ. NO   PKT NO\tSOURCE\t\t DEST\n\n");

	while(1){

		unsigned char recvBuff[PACKET_SIZE]={0}; //for writing into file

		//clear the descriptor set
    	FD_ZERO(&rset);

    	int fdp1, nready, maxfdp1;

    	fdp1 = serverSocket;
    	if(fdp1 >0 )
    		FD_SET(fdp1, &rset);

    	maxfdp1 = serverSocket;

    	//select the ready descriptor
		nready = select(maxfdp1 +1, &rset, NULL, NULL, NULL); //timeout NULL - wait indefinitely

        if(nready == 0)
            break;

		if ((nready < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }  


        if(FD_ISSET(serverSocket, &rset)){

        	if ((recv_len = recvfrom(serverSocket, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &clientAddress, &slen)) == -1)
             {
                die("recvfrom()");
             }

             if(recv_len == 0){
					printf("File transfer successful\n");
					break;
			}


			/*timestamp = time(NULL);
			ptm = localtime(&timestamp);
			//printf("SERVER\t\tR\t%s\t\tDATA\t\t%d\tRELAY%d\t\tSERVER\n",returnTime(ptm),rcv_pkt.header.seq_no, rcv_pkt.header.relayNode);
            fprintf(out, "SERVER\t\tR\t%s\t\tDATA\t\t%d\tRELAY%d\t\tSERVER\n",returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.relayNode);
            fflush(out);
            printf("SERVER\t\tR\t%s\t\tDATA\t\t%d\tRELAY%d\t\tSERVER\n",returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.relayNode);*/

			//packet in order
			if(rcv_pkt.header.seq_no == offset){
                offset += 1;
                
				fwrite(rcv_pkt.data, 1, rcv_pkt.header.size, fp);
                if(rcv_pkt.header.isLastPacket == 1)
                    lastPktWritten = 1;

                if(buffFilled > 0){
                            
                    int i, tempbuff = buffFilled;

                    for(i=0; i < buffFilled ;i++){

                        if(offset == receive_buffer[i].seq_no){
                            writeFromBuffer = 1;
                            fwrite(receive_buffer[i].buffer, 1, receive_buffer[i].size, fp);
                            if(receive_buffer[i].isLastPacket == 1)
                                lastPktWritten = 1;
                            offset += 1;
                            tempbuff --;
                        }

                        else
                            break;
                        
                    }

                    if(writeFromBuffer == 1){

                        int prev = buffFilled;
                        buffFilled = tempbuff;
                        int elementsDiscard = prev - buffFilled, arrIndex = 0;

                        while(arrIndex < buffFilled){
                            receive_buffer[arrIndex] = receive_buffer[elementsDiscard +arrIndex];
                            arrIndex ++;
                        }

                        
                        writeFromBuffer = 0;

                    }

                }

                pktDrop = 0;
			}

			//buffer if out of order
			else{


				if(buffFilled == (BUFFER_CAPACITY)){
                    pktDrop = 1;    //drop further packets because buffer full

                }

                else{
                    buffFilled ++;

                    int i=0;
                    for(i=0; i<buffFilled-1 ; i++){
                        if(receive_buffer[i].seq_no > rcv_pkt.header.seq_no)
                            break;
                    }

                    if(i==0){

                        for(int j=buffFilled-1; j>=1; j--){

                            receive_buffer[j].seq_no = receive_buffer[j-1].seq_no;
                            receive_buffer[j].size = receive_buffer[j-1].size;
                            receive_buffer[j].isLastPacket = receive_buffer[j-1].isLastPacket;
                            strcpy(receive_buffer[j].buffer, receive_buffer[j-1].buffer);

                            
                        }

                        receive_buffer[0].seq_no = rcv_pkt.header.seq_no;
                        receive_buffer[0].size = rcv_pkt.header.size;
                        receive_buffer[0].isLastPacket = rcv_pkt.header.isLastPacket;
                        strcpy(receive_buffer[0].buffer, rcv_pkt.data);

                    }


                    else if(i == (buffFilled-1)){

                        receive_buffer[buffFilled-1].seq_no = rcv_pkt.header.seq_no;
                        receive_buffer[buffFilled-1].size = rcv_pkt.header.size;
                        receive_buffer[buffFilled-1].isLastPacket = rcv_pkt.header.isLastPacket;
                        strcpy(receive_buffer[buffFilled-1].buffer, rcv_pkt.data); 
                    }

                    else{   //inserted in middle

                        int j;
                        for(j=buffFilled-1; j>=(i+1); j--){

                            receive_buffer[j].seq_no = receive_buffer[j-1].seq_no;
                            receive_buffer[j].size = receive_buffer[j-1].size;
                            receive_buffer[j].isLastPacket = receive_buffer[j-1].isLastPacket;
                            strcpy(receive_buffer[j].buffer, receive_buffer[j-1].buffer);
                        }

                        receive_buffer[j].seq_no = rcv_pkt.header.seq_no;
                        receive_buffer[j].size = rcv_pkt.header.size;
                        receive_buffer[j].isLastPacket = rcv_pkt.header.isLastPacket;
                        strcpy(receive_buffer[j].buffer, rcv_pkt.data);

                    }
                    
                }
			}

            if(pktDrop == 0){

                timestamp = time(NULL);
                ptm = localtime(&timestamp);
                printf("SERVER\t\tR\t%s\t\tDATA\t\t%d\t%d\tRELAY%d\t\tSERVER\n",returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.seq_no, rcv_pkt.header.relayNode);
                fprintf(out, "SERVER\t\t\tR\t\t%s\t\tDATA\t\t%d\t\t%d\t\tRELAY%d\t\tSERVER\n",returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.seq_no, rcv_pkt.header.relayNode);
                fflush(out);
                //printf("SERVER\t\tR\t%s\t\tDATA\t\t%d\tRELAY%d\t\tSERVER\n",returnTime(ptm),rcv_pkt.header.seq_in_window, rcv_pkt.header.relayNode);

    			send_ack.header.seq_no = rcv_pkt.header.seq_no;
    			send_ack.header.isData = 0;
    			send_ack.header.isLastPacket = rcv_pkt.header.isLastPacket;
    			send_ack.header.relayNode = rcv_pkt.header.relayNode;
                send_ack.header.seq_in_window = rcv_pkt.header.seq_in_window;
    				
    			if (sendto(serverSocket, &send_ack, sizeof(send_ack), 0, (struct sockaddr *) &clientAddress, slen)==-1){
            		die("send()");
    	    	}
    		
    			timestamp = time(NULL);
    			ptm = localtime(&timestamp);	
    			printf("SERVER\t\tS\t%s\t\tACK\t\t%d\t%d\tSERVER\t\tRELAY%d\n",returnTime(ptm),send_ack.header.seq_in_window,send_ack.header.seq_no,send_ack.header.relayNode);
                fprintf(out,"SERVER\t\t\tS\t\t%s\t\tACK\t\t\t%d\t\t%d\t\tSERVER\t\tRELAY%d\n",returnTime(ptm),send_ack.header.seq_in_window,send_ack.header.seq_no,send_ack.header.relayNode);
                fflush(out);
                //printf("SERVER\t\tS\t%s\t\tACK\t\t%d\tSERVER\t\tRELAY%d\n",returnTime(ptm),send_ack.header.seq_in_window,send_ack.header.relayNode);

                if(lastPktWritten == 1 && buffFilled == 0)
                    break;

                else if(lastPktWritten == 1 && buffFilled!=0){
                     for(int i=0; i < buffFilled ;i++){

                            fwrite(receive_buffer[i].buffer, 1, receive_buffer[i].size, fp);
                        
                    }
                    break;

                }
            }
					
        }

	}
  
	fclose(fp);
	close(serverSocket);
	return 0;
}