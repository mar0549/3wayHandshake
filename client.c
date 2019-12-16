
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>


#define FLAG_FIN 0b000001
#define FLAG_SYN 0b000010
#define FLAG_RST 0b000100
#define FLAG_PSH 0b001000
#define FLAG_ACK 0b010000
#define FLAG_URG 0b100000
#define SOURCE 21334
#define DEST 2200
#define CLIENT_SEQ 1

//tcp segment structure

struct tcp_hdr
{
  unsigned short int src,des;
  unsigned int seq;
  unsigned int ack;
  unsigned short int hdr_flags,rec,cksum,ptr;
  unsigned int opt;
};





unsigned short int checksum(struct tcp_hdr tcp_seg);  //computes the check sum of the structure tcp_hdr

void transmitted(struct tcp_hdr tcp_seg, FILE* fptr); //outputs to client.out structure tcp_hdr values when transmitted to server

void received(struct tcp_hdr tcp_seg, FILE* fptr);  //outputs to client.out  structure tcp_hdr values when received from server

int main(int argc,char **argv)
{
    int sockfd, n;	//socket file descriptor and n is used for the read() return value
    int len = sizeof(struct sockaddr);	//length of the structure sockaddr
    struct sockaddr_in servaddr;	//structure for the server address
    int portNum;	//port number

    FILE *fptr; //file pointer
    fptr = fopen("client.out","w"); //client.out open for writing


    struct tcp_hdr tcp_seg; //instance of tcp_hdr

    //assign tcp_seg's structure
    tcp_seg.src = SOURCE;
    tcp_seg.des = DEST;
    tcp_seg.seq = CLIENT_SEQ;
    tcp_seg.ack = 0;
    tcp_seg.hdr_flags = 0x0000;
    tcp_seg.rec = 0;
    tcp_seg.cksum = 0;
    tcp_seg.ptr = 0;
    tcp_seg.opt = 0;

    tcp_seg.hdr_flags =(sizeof(struct tcp_hdr) /4) << 12;

    tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_SYN;



    if(tcp_seg.cksum==0)
    {
      tcp_seg.cksum = checksum(tcp_seg);
    }








    //error message if the wrong number of command line arguments are used
    if(argc!=2){
    	printf("Please re-run the program in the following format... client <port_number>\n");
    	exit(1);
    }
    else
	    portNum = atoi(argv[1]);        //assign second command line argument to portNum


    /* AF_INET - IPv4 IP , Type of socket, protocol*/
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
	    perror("Socket:");
	    exit(1);
    }
    //remove servaddr content
    bzero(&servaddr,sizeof(servaddr));

    //populate servaddr structure
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(portNum); // Server port number

    /* Convert IPv4 and IPv6 addresses from text to binary form */
    //IP address of cse01.cse.unt.edu
	inet_pton(AF_INET,"129.120.151.94",&(servaddr.sin_addr));

    /* Connect to the server */
    if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
    {
    	perror("Connection");
    	exit(1);
    }


    //START OF 3 WAY HANDSHAKE
    //CONNECTED TO SERVER
    write(sockfd,&tcp_seg,sizeof(struct tcp_hdr));
    transmitted(tcp_seg, fptr);

    bzero(&tcp_seg,sizeof(tcp_seg));
    read(sockfd, &tcp_seg, sizeof(tcp_seg));
    received(tcp_seg, fptr);

    //create copy of the structure tcp_hdr
    struct tcp_hdr copy = tcp_seg;
    //set copy check sum to 0
    copy.cksum = 0;
    copy.cksum = checksum(copy);

    if(tcp_seg.cksum == copy.cksum)
    {
      tcp_seg.ack = tcp_seg.seq +1;
      tcp_seg.seq = CLIENT_SEQ;
      tcp_seg.hdr_flags = 0x0000;
      tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_ACK;
      tcp_seg.cksum = 0;
      tcp_seg.cksum = checksum(tcp_seg);
      write(sockfd,&tcp_seg,sizeof(struct tcp_hdr));
      transmitted(tcp_seg, fptr);
    }




    //CLOSING TCP CONNECTION
    bzero(&tcp_seg,sizeof(tcp_seg));

    tcp_seg.src = SOURCE;
    tcp_seg.des = DEST;
    tcp_seg.seq = CLIENT_SEQ;
    tcp_seg.ack = 0;
    tcp_seg.hdr_flags = 0x0000;
    tcp_seg.rec = 0;
    tcp_seg.cksum = 0;
    tcp_seg.ptr = 0;
    tcp_seg.opt = 0;

    tcp_seg.hdr_flags =(sizeof(struct tcp_hdr) /4) << 12;
    tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_FIN;

    //compute checksum
    if(tcp_seg.cksum==0)
    {
      tcp_seg.cksum = checksum(tcp_seg);
    }
    write(sockfd,&tcp_seg,sizeof(struct tcp_hdr)); //send server a close request
    transmitted(tcp_seg, fptr);

    bzero(&tcp_seg,sizeof(tcp_seg));
    read(sockfd, &tcp_seg, sizeof(tcp_seg));//read ack
    received(tcp_seg, fptr);
    bzero(&tcp_seg,sizeof(tcp_seg));
    read(sockfd, &tcp_seg, sizeof(tcp_seg));//read fin
    received(tcp_seg, fptr);

    bzero(&copy,sizeof(copy));
    copy = tcp_seg;
    //set copy check sum to 0
    copy.cksum = 0;
    copy.cksum = checksum(copy);

    if(tcp_seg.cksum == copy.cksum)
    {
      tcp_seg.ack = tcp_seg.seq +1;
      tcp_seg.seq = CLIENT_SEQ+1;
      tcp_seg.hdr_flags = 0x0000;
      tcp_seg.hdr_flags = tcp_seg.hdr_flags | FLAG_ACK;
      tcp_seg.cksum = 0;
      tcp_seg.cksum = checksum(tcp_seg);
      write(sockfd,&tcp_seg,sizeof(struct tcp_hdr));
      transmitted(tcp_seg, fptr);
    }




fclose(fptr);
close(sockfd);
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
