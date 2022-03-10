#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "crc32.h"

#define BUF_MAX_SIZE 1472
#define BUF_ACK_SIZE 3
#define ACK_SUCCESS 0
#define ACK_ERROR 1
#define PACKET_TYPE 0
#define SEQUENCE_NUMBER 1
#define PACKET_SIZE 2
#define DIRECTORY 6
#define FILE_NAME 66
#define DATA 86
#define DATA_LEN 1382
#define CRC_SIZE 4
/*****************************************/
/* main program                          */
/*****************************************/

/* simple server, takes one parameter, the server port number */
int main(int argc, char *argv[]) {
    printf("Server is running!\n");
    /* socket variable */
    int sock;

    /* server socket address variables */
    struct sockaddr_in sin, addr;
    unsigned short server_port = atoi(argv[2]);

    /* socket address variables for a connected client */
    socklen_t addr_len = sizeof(struct sockaddr_in);

    /* create a server socket */
    if ((sock = socket (PF_INET, SOCK_DGRAM, 0)) < 0) // protocal family internet, negative means error
    {
        perror ("opening socket datagram socket");
        abort ();
    }
  
  /* set option so we can reuse the port number quickly after a restart */
//   if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval)) <0)
//     {
//       perror ("setting TCP socket option");
//       abort ();
//     }

    /* fill in the address of the server socket */
    memset (&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (server_port); // 18000

    /* bind server socket to the address */
    if (bind(sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("binding socket to address");
        abort();
    }
    /* receiver buffer */
    char* recv_buf;


    /* ack: status number (1 byte: 0 success, 1 error), seq number (1 byte), checksum (1 byte) ? */
    char ack_buf[BUF_ACK_SIZE];

    /* create a variable for the existing sequence number */
    char last_sq_no = -1;

    /* previous file name to check append or write to the file */
    char first_file = 0;

    /* now we keep waiting for incoming connection */
    while (1) {
        printf("****************************************************\n");
        recv_buf = (char *)malloc(BUF_MAX_SIZE);
        int recvNum = recvfrom(sock, recv_buf, BUF_MAX_SIZE, 0, (struct sockaddr *)&addr, &addr_len);

        if (recvNum < 0) {
            perror("[recv corrupt packet]");
            continue;
        }

        // get sequence number
        char recv_sq_no = recv_buf[SEQUENCE_NUMBER]; 
        printf("recv sequence number %d\n", recv_sq_no);
        int data_len = (int) ntohl(*(int *)(recv_buf+PACKET_SIZE));
        printf("data size %d\n", data_len);
        
        // check crc for error
        unsigned int crc_received = crc32(recv_buf, 90 + data_len - CRC_SIZE); 
        printf("crc calculated is %d\n", crc_received);
        // unsigned int crc_sended = (unsigned int) ntohl(*(unsigned int*) (recv_buf + BUF_MAX_SIZE - CRC_SIZE)); //no seg fault
        unsigned int crc_sended = *(unsigned int*)(recv_buf + 90 + data_len - CRC_SIZE);

        printf("crc calculated is %d, crc sended is %d\n", crc_received,crc_sended);
        if (crc_received != crc_sended) {
            free(recv_buf);
            printf("[recv corrupt packet]\n");
            ack_buf[0] = ACK_ERROR;
            ack_buf[1] = recv_sq_no;
            ack_buf[2] = csum(ack_buf, 2);
           // printf("ack[0] = %d\n",ack_buf[0]);
            //printf("ack[1] = %d\n",ack_buf[1]);
           // printf("csum of ack = %d\n",ack_buf[2]);
            int sendNum = sendto(sock, &ack_buf, BUF_ACK_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
            if (sendNum < 0) {
                perror("Sending ACK error");
                continue;
            }   
            continue;
        }
        // check sequence number, if same as the previous one, ignore
        else if (last_sq_no == recv_sq_no) {
            free(recv_buf);
            printf("last_sq_no = %d \t recv_sq_no = %d\n",last_sq_no,recv_sq_no);
            printf("[recv data] IGNORED\n");
             ack_buf[0] = ACK_SUCCESS;
             ack_buf[1] = recv_sq_no;
             ack_buf[2] = csum(ack_buf, 2);
             int sendNum = sendto(sock, &ack_buf, BUF_ACK_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                         if (sendNum < 0) {
                             perror("Sending ACK error");
                            continue;
                         }
            continue;
        }

       printf("CORRECT PACKET!");
        // save the file to the disk
        //int data_len = (int) htonl(*(int *)(recv_buf+DATA_LEN));
        last_sq_no = recv_sq_no;
        char fullpath[255];
        char filename[20];
        char directory[60];
        char data[data_len];
        memcpy(filename, recv_buf+FILE_NAME, 20);
        memcpy(directory, recv_buf+DIRECTORY, 60);
        memcpy(data, recv_buf+DATA, data_len);
        strcpy(fullpath, directory);
        strcpy(fullpath, "/");
        strcpy(fullpath, filename);
        strcpy(fullpath, "./recv");
        char mode;
        FILE *fp;
        if (first_file == 1) {
            mode = 'a';
        }
        else {
            mode = 'w';
        }
        fp = fopen(fullpath, &mode);
        if (fp){
            size_t msg_size_written = fwrite(data, 1, data_len, fp);
            if (msg_size_written != data_len){
                perror("File writing issue.");
                return -1;
            }
            else {
                if (first_file == 0) {
                    first_file = 1;
                }
                printf("[recv data] start %d ACCEPTED\n", data_len);
            }
        }
        else{
            perror("File open error.");
            return -1;
        }
        fclose(fp);

        // send ack 
        ack_buf[0] = ACK_SUCCESS;
        ack_buf[1] = recv_sq_no;
        ack_buf[2] = csum(ack_buf, 2);
        //printf("ack check sum is %d\n", ack_buf[2]);
        int sendNum = sendto(sock, &ack_buf, BUF_ACK_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (sendNum < 0) {
            perror("Sending ACK error");
            continue;
        }   

    }

    free(recv_buf);
    printf("[completed]\n");
    close(sock);
    return 0;
}