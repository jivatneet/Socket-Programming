#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
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
#include <limits.h>
#define PACKET_SIZE 100 //in bytes
#define PORT_R1 8882 //port for R1
#define PORT_R2 8888 //port for R2
#define PORTS 8880 //port for server
#define PDR 10 //in %
#define MAX_DELAY 2 //in ms

#ifndef _PKT_H
#define _PKT_H

typedef struct header{

 int seq_no; //offset of first byte
 int size; //no of bytes of payload
 int isLastPacket;
 int isData; //1-DATA, 0-ACK
 int relayNode; //R1/R2
 int seq_in_window;
 clock_t startTime; 

}HEADER;

typedef struct packet{

 char data[PACKET_SIZE+1];
 HEADER header;

}PKT;

typedef struct receiveBuffer
{
    int seq_no;
    int size;
    int isLastPacket;
    char buffer[PACKET_SIZE+1]; 

}receiveBuffer;

void die(char *s)
{
    perror(s);
    exit(1);
}

int max(int x, int y) 
{ 
    if (x > y) 
        return x; 
    else
        return y; 
} 

void printstruct(PKT pkt){

    printf("Printing Packet Info:\n");
    printf("seq no:%d\n",pkt.header.seq_in_window);
    printf("packet no:%d\n",pkt.header.seq_no);
    printf("size:%d\n",pkt.header.size);
    printf("isLastPacket:%d\n", pkt.header.isLastPacket);
}

char *returnTime(struct tm *tmptr){

    struct timeval t;
    gettimeofday(&t, NULL);
    char *timestr = (char *)malloc(sizeof(char)*25);
    char *milli = (char *)(malloc(sizeof(char)*8));
    int curr_time = strftime(timestr, 25, "%H:%M:%S", tmptr);
    sprintf(milli, "%.06ld", t.tv_usec);
    strcat(timestr, milli);
    return timestr;

}
#endif
