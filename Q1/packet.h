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
#define PORT 8882 //The port on which to send data
#define PACKET_SIZE 100 //in bytes
#define PDR 10 //in %

#ifndef _PKT_H
#define _PKT_H

typedef struct header{

 int seq_no; //offset of first byte
 int size; //no of bytes of payload
 int isLastpacket;
 int isData; //1-DATA, 0-ACK
 int channel; //0/1

}HEADER;

typedef struct packet{

 char data[PACKET_SIZE+1];
 HEADER header;

}PKT;

typedef struct receiveBuffer
{
    int seq_no;
    int size;
    char buffer[PACKET_SIZE+1]; 

}receiveBuffer;

void die(char *s)
{
    perror(s);
    exit(1);
}

int max(int x, int y) 
{ 
    if (x < y) 
        return y; 
    else
        return x; 
} 

void printstruct(PKT pkt){

	printf("Printing Packet Info:\n");
	printf("seq no:%d\n",pkt.header.seq_no);
	printf("channel:%d\n",pkt.header.channel);
    printf("size:%d\n",pkt.header.size);
	printf("isLastpacket:%d\n", pkt.header.isLastpacket);
}

#endif