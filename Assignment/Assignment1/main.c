// CSCI 4220 - Assignment 1 (TFTP Server)
//

#include <stdio.h>  
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <setjmp.h>
#include <sys/wait.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>

#define DEBUG  1

// Operation codes (opcode)
#define OP_RRQ       0
#define OP_WRQ       1
#define OP_DATA      2
#define OP_ACK       3
#define OP_ERR       4






// Packet decription structures
// Placed inside a union for convenience
typedef union 
{
   // packet_rrq:  specifies a read request packet
   // NOTE: filename must be null terminated and <= 514 bytes in length
   struct packet_rrq
   {
      uint16_t opcode;
      char filename [514];
   };
   // packet_wrq:  specifies a read request packet
   // NOTE: filename must be null terminated and <= 514 bytes in length
   struct packet_wwq
   {
      uint16_t opcode;
      char filename [514];
   };
   // packet_data: specifies a data packet.
   // data contents must be null terminated and no greater than 512 bytes
   // total.
   struct packet_data
   {
      uint16_t opcode;
      uint16_t blocknum;
      char data[512];
   };
   // packet_ack: specifies an acknowledgement packet.
   struct packet_ack
   {
      uint16_t opcode;
      uint16_t blocknum;
   };
   // packet_error: represents an error packet.
   // message (msg) must be null terminated and no greater than
   // 512 bytes total.
   struct packet_error
   {
      uint16_t opcode;
      uint16_t blocknum;
      char msg[512];
   };


} tftp_packet;

// format_packet(): Will generate a tftp_packet based on the opcode specified

tftp_packet format_packet(int opcode)
{

}


// SIGNAL handlers
void sigchld_handler(int v)
{
   pid_t pidC;   
   pidC = wait(NULL); // Block parent until child completes
  
   printf("[Parent] Parent confirms child (pid=%d) finished.\n",pidC);
   
}

// main(): Starting point for TFTP server. 
// Designed to handle two additional arguments (argc=3)
// argv[0] (implied)
// argv[1] port number (start of port listen range)
// argv[2] port number (end of port listen range)
// The TFTP server program will listen at and between these port numbers
// while executing
int main(int argc, char** argv)
{
   // Do a sanity check to make sure we have enough arguments
   if (argc < 3)
   {
      perror("[Error] Invalid number of arguments!\n
      Args: port_start_range port_end_range\n");
      exit(EXIT_FAILURE);
   }
   // Parse the range borders (inclusive)
   int portCounter = rangeStart;
   int rangeStart = atoi(argv[1]);
   int rangeEnd = atoi(argv[2]);
   

   // Set up SIGNAL handlers for when clients connect
   // or if there are SIGNAL errors 
   signal(SIGCHLD,sigchld_handler);
   
   // Set up server socket
   struct sockaddr_in servaddr; // Stores information about the socket
   int socket_fd; // Socket file descriptor
   socklen_t addrlen = sizeof(servaddr); // Total bytes length of the socket description
   memset(&sock_desc,0,addrlen); // Set the socket description to blank

   // tftp server setup using udp protocol
   
   socket_fd = socket(PF_INET,SOCK_DGRAM,0);
   if (socket_fd < 0)
   {
      // Error: Socket file descriptor could not be initialized
      perror("[Error] socket file descriptor failure!\n");
      exit(EXIT_FAILURE);
   }

   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = INADDR_ANY;

   // Essential variables
   tftp_packet myPacket;
   uint16_t ret;
   unsigned short int opc; // operational code
   uint16_t* opc_ptr; // pointer for operational code
   while(portCounter <= rangeEnd)
   {
      // Set server to listen on specific port in the range
      servaddr.sin_port = htons(port_cntr);
      if (bind(socket_fd,(struct sockaddr *)&servaddr,addrlen) < 0)
      {
         // Binding failure!
         perror("[Error] Binding failure!\n");
         exit(EXIT_FAILURE);
      }
      // Confirm the socket port
      struct sockaddr_in clientaddr;
      socklen_t slen = sizeof(clientaddr);
      getsockname(socket_fd,(struct sockaddr *)&servaddr,&addrlen);
      printf("[Server] Bound on port %d\n",ntohs(servaddr.sin_port));
      ret = recvfrom(socket_fd,&myPacket,sizeof(myPacket),0,
               (struct sockaddr *)&clientaddr,&slen);

      if (ret < 0)
      {
         if (errno == EINTR)
         {
            continue; // TODO: saw this but no idea
         }

         // Error on packet?
         perror("[Error] recvfrom() failure! Could not receive from client\n");
         exit(EXIT_FAILURE);
      }
      // --------------
      // Packet received! If we have a rrq or wrq, fork
      // off process and conduct the transfer of data

      //  Extract the opcode from the received initial packet
      uint16_t opcTemp;  // Used to get the opcode at this current iteration
      opc_ptr = &opcTemp;
      *opc_ptr = myPacket.opcode;
      opc = ntohs(*opc_ptr); // Convert value to perm storage across loops
      printf("[debug] received packet with opcode %d\n",opc);
      if (opc == OP_RRQ || opc == OP_WRQ)
      {
         // We have a read or write request
         // use the appropriate handling
         pid_t pval = fork();
         if (pval == 0)
         {
            // We are a child process
            if (opc == OP_RRQ)
            {
               // TODO: insert RRQ call here
            }
            if (opc == OP_WRQ)
            {
               // TODO: insert WRQ call here
            }
         } else {
            // We are a parent process
            // Nothing more to do
         }
      } else {

      }    
      port_cntr += 1; // Increment port counter
   }
   return 0;
}