/*
Michael Rivera
CSCE 3530 Introduction to Computer Networks
Section 001
Submitted : 09/03/2019
Description: Implementation of a proxy server. Must be executed with one argument. This argument will be the port number that the user would like to use. This proxy will handle http requests from a client that is properly connected to it, and relay the website to the client

MUST BE RAN ON cse01.cse.unt.edu for client to connect properly

This program was built off of a supplied server code provided by Professor Qing Yang

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>

#define FLAG_FIN 0b000001
#define FLAG_SYN 0b000010
#define FLAG_RST 0b000100
#define FLAG_PSH 0b001000
#define FLAG_ACK 0b010000
#define FLAG_URG 0b100000
#define SERVER_SEQ 100


//tcp segment structure

struct tcp_hdr
{
  unsigned short int src,des;
  unsigned int seq;
  unsigned int ack;
  unsigned short int hdr_flags,rec,cksum,ptr;
  unsigned int opt;
};



unsigned short int checksum(struct tcp_hdr tcp_seg);

void transmitted(struct tcp_hdr tcp_seg, FILE* fptr);

void received(struct tcp_hdr tcp_seg, FILE* fptr);

int main(int argc, char **argv)
{


    int listen_fd, conn_fd;	//connection and listening  file descriptor
    struct sockaddr_in servaddr;	//structure for the server address
    int portNum;  //port number used from argv[1]
    int i;  //used in for loops
    struct tcp_hdr tcp_seg;

    FILE *fptr;
    fptr = fopen("server.out","w");





    //error message if pserver is not ran correctly
    if(argc!=2){
	printf("Please re-run the program in the following format... pserver <port_number>\n");
    	exit(1);
    }
    else
    	portNum = atoi(argv[1]);        //assign second command line argument to portNum


    /* AF_INET - IPv4 IP , Type of socket, protocol*/
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    //zero server address info
    bzero(&servaddr, sizeof(servaddr));

    //populate servaddr structure
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(portNum);

    /* Binds the above details to the socket */
	bind(listen_fd,  (struct sockaddr *) &servaddr, sizeof(servaddr));
	/* Start listening to incoming connections */
	listen(listen_fd, 10);



      /* Accepts an incoming connection */
	    conn_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL);



      //START OF 3 WAY HANDSHAKE
      //READ FROM CLIENT THE STRUCTURE info
      bzero(&tcp_seg,sizeof(tcp_seg));
      read(conn_fd, &tcp_seg, sizeof(tcp_seg));
      received(tcp_seg, fptr);

      //create copy of the structure tcp_hdr
      struct tcp_hdr copy = tcp_seg;
      //set copy check sum to 0
      copy.cksum = 0;
      copy.cksum = checksum(copy);

      if(tcp_seg.cksum == copy.cksum)
      {
        tcp_seg.ack = tcp_seg.seq +1;
        tcp_seg.seq = SERVER_SEQ;
        tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_SYN | FLAG_ACK;
        tcp_seg.cksum = 0;
        tcp_seg.cksum = checksum(tcp_seg);

        write(conn_fd,&tcp_seg,sizeof(struct tcp_hdr));
        transmitted(tcp_seg, fptr);
      }


      bzero(&tcp_seg,sizeof(tcp_seg));
      read(conn_fd, &tcp_seg, sizeof(tcp_seg));
      received(tcp_seg, fptr);

      //create copy of the structure tcp_hdr
      bzero(&copy,sizeof(copy));
      copy = tcp_seg;
      //set copy check sum to 0
      copy.cksum = 0;
      copy.cksum = checksum(copy);

      if(tcp_seg.cksum == copy.cksum)
      {
        tcp_seg.ack = tcp_seg.seq +1;
        tcp_seg.seq = SERVER_SEQ;
        tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_SYN | FLAG_ACK;
      }




      //START OF CLOSING
      bzero(&tcp_seg,sizeof(tcp_seg));
      read(conn_fd, &tcp_seg, sizeof(tcp_seg)); //first read
      received(tcp_seg, fptr);
      bzero(&copy,sizeof(copy));
      copy = tcp_seg;
      //set copy check sum to 0
      copy.cksum = 0;
      copy.cksum = checksum(copy);

      if(tcp_seg.cksum == copy.cksum)
      {
        tcp_seg.ack = tcp_seg.seq +1;
        tcp_seg.seq = SERVER_SEQ;
        tcp_seg.hdr_flags = 0x0000;
        tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_ACK;
        tcp_seg.cksum = 0;
        tcp_seg.cksum = checksum(tcp_seg);

        write(conn_fd,&tcp_seg,sizeof(struct tcp_hdr)); //send ack
        transmitted(tcp_seg, fptr);

        tcp_seg.hdr_flags = 0x0000;
        tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_FIN;
        tcp_seg.cksum = 0;
        tcp_seg.cksum = checksum(tcp_seg);
        write(conn_fd,&tcp_seg,sizeof(struct tcp_hdr)); //send fin
        transmitted(tcp_seg, fptr);
      }

      bzero(&tcp_seg,sizeof(tcp_seg));
      read(conn_fd, &tcp_seg, sizeof(tcp_seg));
      received(tcp_seg, fptr);





fclose(fptr);
close(conn_fd);


}//end of main function






unsigned short int checksum(struct tcp_hdr tcp_seg)
{
  unsigned short int cksum_arr[12];
  unsigned int i,sum=0, cksum;


  memcpy(cksum_arr, &tcp_seg, 24); //Copying 24 bytes



  for (i=0;i<12;i++)               // Compute sum
   sum = sum + cksum_arr[i];

  cksum = sum >> 16;              // Fold once
  sum = sum & 0x0000FFFF;
  sum = cksum + sum;

  cksum = sum >> 16;             // Fold once more
  sum = sum & 0x0000FFFF;
  cksum = cksum + sum;

  /* XOR the sum for checksum */

  return 0xFFF^cksum;



}//end of checksum function


void transmitted(struct tcp_hdr tcp_seg, FILE* fptr)
{

  fprintf(fptr,"------transmitted--------\n\n");
  fprintf(fptr,"source: 0x%04X\n", tcp_seg.src);
  fprintf(fptr,"destination: 0x%04X\n", tcp_seg.des);
  fprintf(fptr,"sequence: 0x%08X\n", tcp_seg.seq);
  fprintf(fptr,"acknowledgement: 0x%08X\n",tcp_seg.ack);
  fprintf(fptr,"flags: 0x%04X\n",tcp_seg.hdr_flags);
  fprintf(fptr,"receive: 0x%04X\n", tcp_seg.rec);
  fprintf(fptr,"checksum: 0x%04X\n", tcp_seg.cksum);
  fprintf(fptr,"data pointer: 0x%04X\n", tcp_seg.ptr);
  fprintf(fptr,"options: 0x%08X\n\n", tcp_seg.opt);

}

void received(struct tcp_hdr tcp_seg, FILE* fptr)
{

  fprintf(fptr,"------received--------\n\n");
  fprintf(fptr,"source: 0x%04X\n", tcp_seg.src);
  fprintf(fptr,"destination: 0x%04X\n", tcp_seg.des);
  fprintf(fptr,"sequence: 0x%08X\n", tcp_seg.seq);
  fprintf(fptr,"acknowledgement: 0x%08X\n",tcp_seg.ack);
  fprintf(fptr,"flags: 0x%04X\n",tcp_seg.hdr_flags);
  fprintf(fptr,"receive: 0x%04X\n", tcp_seg.rec);
  fprintf(fptr,"checksum: 0x%04X\n", tcp_seg.cksum);
  fprintf(fptr,"data pointer: 0x%04X\n", tcp_seg.ptr);
  fprintf(fptr,"options: 0x%08X\n\n", tcp_seg.opt);

}
