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


// main(): Starting point for TFTP server. 
// Designed to handle two additional arguments (argc=3)
// argv[0] (implied)
// argv[1] port number (start of port listen range)
// argv[2] port number (end of port listen range)
// The TFTP server program will listen at and between these port numbers
// while executing
int main(int argc, char** argv)
{
   // Set up SIGNAL handlers for when clients connect
   // or if there are SIGNAL errors 

   // Set up server socket
   struct sockaddr_in sock_desc; // Stores information about the socket
   int socket_fd; // Socket file descriptor
   socklen_t sd_len = sizeof(sock_desc); // Total bytes length of the socket description

   

}