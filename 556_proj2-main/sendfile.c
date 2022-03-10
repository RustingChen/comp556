#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "crc32.h"

int main(int argc, char **argv)
{
    //The command line syntax for your sendfile program is given below:
    //sendfile -r <recv host>:<recv port> -f <subdir>/<filename>
    if (argc!=5)
    {
        printf("Incorrect number of arguments!\n");
        exit(1);
    }
    /* Packet Format
     * DGRAM Packet Structure:
     * Packet Type: SEQ or FIN (1 Byte - 0:SEQ, 1: FIN)
     * sq_no: sequence number (1 byte)
     * Bytes read: 4 Bytes int
     * Directory: 60 Bytes
     * File name: 20 Bytes
     * Data: 1382 Bytes
     * CRC Error Code: 4 Bytes int
       total = 1+1+4+60+20+1382+4 = 1472 */
    char packet_type;
    char sq_no = (char)0;

    int data_size = 1382;
//    int size = 1472;
    int bytes_read;
    char dir[60];
    char file[20];
    unsigned int crc;
    short ack_size = 3;
    char rcv_buffer[ack_size];
    int fix_packet_size = 1472;
    //get server add and file location from the args
    char* host_port = argv[2];
    char* file_loc = argv[4];
   
    char ffile[strlen(file_loc)];
    strcpy(ffile, file_loc);
    
    char* token1 = strtok(host_port, ":");
    char* host = token1;
    token1 = strtok(NULL, ":");
    char* port = token1;
    char* token2 = strtok(file_loc, "/");
    char* directory = token2;
    token2 = strtok(NULL, "/");
    char* file_name = token2;
    strncpy(dir, directory, 60);
    strncpy(file, file_name, 20);
    //our port
    unsigned short portNumber = atoi(port);
    //our client socket 
    int sockfd;
    struct sockaddr_in sin;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed.\n");
        abort();
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(portNumber);
    sin.sin_addr.s_addr = inet_addr(host);

    //Read the file into packets in buffer
    char* buffer; //store file data
    char* packet; //store a complete message
    buffer = (char*)malloc(data_size*sizeof(char));
    packet = (char*)malloc(fix_packet_size*sizeof(char));

      FILE *fp;
      fp = fopen(ffile,"r");
      if(!fp)
    {
      perror("Can't open this file");
      exit(1);
    }
    else
      printf("File open succeed.\n");
    
    int sum_bytes=0;
    int pack_count=0;
    struct timeval t;	  
	while((bytes_read = fread(buffer, 1, data_size, fp))>0)
	  {  printf("**********************new packet*******************\n");
	    sum_bytes+=bytes_read;
	    pack_count++;
	    printf("sum_bytes=%d\n",sum_bytes);
	    printf("pack_count=%d\n",pack_count);
	    int real_packet_size = bytes_read + sizeof(bytes_read) + sizeof(packet_type) + sizeof(sq_no) + sizeof(dir) + sizeof(file) + sizeof(crc);
	    printf("bytes_read = %d\n",bytes_read);
	    printf("packet_size = %d\n",real_packet_size);
	    //  printf("buffer = %s\n",buffer);
	    memset(packet, 0, fix_packet_size); //reset the packet for new data
	    if(bytes_read < data_size)
	    {
	        memset(packet, 1, 1);
	    }
	    else {
        memset(packet, 0, 1);
      }
	   
	    //fill the packet
	    memset(packet+1, sq_no, 1);  //sq_no
	    *(int*)(packet+2) = htonl(bytes_read); //
	    memcpy(packet+6, dir, 60);
	    memcpy(packet+66, file, 20);
	    memcpy(packet+86, buffer, data_size);
	    //  printf("dir=%s\n",packet+6);
	    //  printf("file = %s\n",packet+66);
	    printf("sq_no = %d\n",sq_no);
            //clear buffer
	    memset(buffer, 0, data_size);

	    //compute crc
	    crc = crc32(packet,real_packet_size-sizeof(crc));
	   // printf("crc = %d\n",crc);
	   // printf("packet_size= %d\n",packet_size);
	   // printf("packet_size-sizeof(crc)=%ld\n",real_packet_size-sizeof(crc));
	    // *(int*)(packet+(packet_size-sizeof(crc))) = htonl(crc);
	    memcpy(packet+real_packet_size-sizeof(crc), &crc, sizeof(crc));
	    printf("crc in packet is %d\n",*(int*)(packet+(real_packet_size-sizeof(crc))));
        //keep sending same packet if needed
	    while(1)
	      {
	        sendto(sockfd, (const char *)packet, real_packet_size, 0, (const struct sockaddr *)&sin, sizeof(sin));

	        t.tv_sec = 2; // timeout value
		    t.tv_usec = 0;

		    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t); //set timeout
	        int bytes_rcvd = recvfrom(sockfd, rcv_buffer, ack_size, MSG_WAITALL, (struct sockaddr *)&sin, &addr_len);

	        if(bytes_rcvd > 0)
	        {
	              //first check csum of ack
		          char csum_ack = csum(rcv_buffer, 2);
		          if (csum_ack != rcv_buffer[2])
		          {
                      printf("Ack corrupted.\n");
                      printf("Resending packet since ACK is corrupted\n");
                      continue;
                  }

                  //if ack is correct then check its value
                  char ck_ack = rcv_buffer[0];
		          char recv_sq_no = rcv_buffer[1];
                  if (recv_sq_no == sq_no && ck_ack == 0)   //sq_no correspond and no crrupt in packet send
                  {  printf("SUCCESS! State send nex packet!\n");
                      if (sq_no == 1)
                      {
                        sq_no = (char)0;
                      }
                      else if (sq_no == 0)
                      {
                        sq_no = (char)1;
                      }

                        break;  //stop loop and start to end new packet
                  }
                  else if(recv_sq_no != sq_no) //packet send corrupted and stay in loop
                  {
                   printf("Resending packet since sq_no is not correspond to packet send.\n");
                  }
                  else if(ck_ack == 1)
                  {
                      printf("packet sent corrupt.\n");
                  }
	         }
	        else  //timeout and resend
            {
                  perror("Time out");
                  printf("Resending packet\n");
            }
	    }
	  }
    close(sockfd);
    return 0;
}
