// CSCI 4220 - Assignment 1 (TFTP Server)
// Team Members: Wendy Hu, Nicholas Jung, Marcus Panozm, Harry Tan
// Note: Use port ranges starting > 1200 otherwise permission denied
// version 2
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
#define OP_RRQ       1
#define OP_WRQ       2
#define OP_DATA      3
#define OP_ACK       4
#define OP_ERR       5






// Packet decription structures
// Placed inside a union for convenience
typedef union __tftp
{
   uint16_t opcode;
   // packet_rrq:  specifies a read request packet
   // NOTE: filename must be null terminated and <= 514 bytes in length
   struct 
   {
      uint16_t opcode;
      char filename [514];
   } packet_rrq;
   // packet_wrq:  specifies a read request packet
   // NOTE: filename must be null terminated and <= 514 bytes in length
   struct 
   {
      uint16_t opcode;
      char filename [517];
   } packet_wrq;
   // packet_data: specifies a data packet.
   // data contents must be null terminated and no greater than 512 bytes
   // total.
   struct 
   {
      uint16_t opcode;
      uint16_t blocknum;
      char data[512];
   } packet_data;
   // packet_ack: specifies an acknowledgement packet.
   struct 
   {
      uint16_t opcode;
      uint16_t blocknum;
   } packet_ack;
   // packet_error: represents an error packet.
   // message (msg) must be null terminated and no greater than
   // 512 bytes total.
   struct 
   {
      uint16_t opcode;
      uint16_t blocknum;
      char msg[512];
   } packet_error;


} tftp_packet;

// ACK packet is sent

ssize_t send_ack_packet (int socket_fd, struct sockaddr_in * sock_INF, socklen_t sock_len, 
                        uint16_t blockNumber) 
{
   // Store ack # and block number in packet data
   tftp_packet msg;
   msg.packet_ack.opcode = htons(OP_ACK);
   msg.packet_ack.blocknum = htons(blockNumber);

   // Return value contains 
   ssize_t ret;
   if((ret = sendto(socket_fd, &msg, sizeof(msg.packet_ack),
            0, (struct sockaddr *) sock_INF, sock_len)) < 0) 
   {
      printf("[Error] sendto() was unsuccessful");
      exit(EXIT_FAILURE);
   }
   return ret;
}

//  errorMsg sent over with packet error info containing errorMsg, code, and opcode.

ssize_t errorMsg (int socket_fd, struct sockaddr_in *sock_INF, socklen_t sock_len, 
                  const char* errorMsg, int errorCode)
{
   tftp_packet msg;
   msg.opcode = htons(OP_ERR);

   strncpy(msg.packet_error.msg, errorMsg, sizeof(msg.packet_error.msg));
   msg.packet_error.blocknum = errorCode;

   ssize_t ret;

   // send information over
   if ((ret = sendto(socket_fd, &msg, strlen(errorMsg) + 4, 0, (struct sockaddr*)  sock_INF, 
                     sock_len)) < 0)
   {
      printf("[Error] sendto() was unsuccessful");
      exit(EXIT_FAILURE);
   }
   return ret;
}



//wrq request
void handle_wrq(int socket_fd, struct sockaddr_in *sock_INF,tftp_packet msg){

   tftp_packet filePacket;
   memset(filePacket.packet_data.data,0,517);
   char backupBuffer[517];
   memset(backupBuffer,0,517);

    ssize_t n = 0;
    int tid = ntohs(sock_INF->sin_port);
    char* buffer = msg.packet_wrq.filename;
    unsigned short * opcode_ptr = (unsigned short *)msg.packet_wrq.opcode;
    char filename[80];
    FILE * fp;
    int block = 0;
    socklen_t sock_len;
    int more = 1;
    int last_block = 0;
    int count = 0;

   

    //get file
    strcpy(filename, msg.packet_wrq.filename); 
    //strcpy(buffer, filename);
    fp = fopen(filename, "w");

    //Ack packet, block #0
    *opcode_ptr = htons(OP_ACK);
    *(opcode_ptr + 1) = htons(block);
    printf("fileName is %s\n", filename);
    printf("Buffer is%s \n",msg.packet_wrq.filename);


    //store contents of this packet
    for(int i = 0; i < 517; i++)
        backupBuffer[i] = msg.packet_wrq.filename[i];
    int last_packet_size = 4;
    

    //send the first ack to begin receiving data
    //n = sendData(socket_fd, buffer, 4, sock_INF);
    //sendto(socket_fd, filePacket.packet_data.data, n, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));
    n = sendto(socket_fd, filename, n, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));

    while(more){
        n = recvfrom(socket_fd, msg.packet_wrq.filename, 517, 0, (struct sockaddr *)&sock_INF, &sock_len);
        if(n < 0) {
            if(errno == EINTR) continue;
            if(errno == EWOULDBLOCK){//1 second timeout
                if(++count >= 10){
                    printf("transaction timed out\n");
                    break;
                }
                //restore last packet
                for(int i = 0; i < 517; i++)
                    msg.packet_wrq.filename[i] = backupBuffer[i];
                n = sendto(socket_fd, msg.packet_wrq.filename, last_packet_size, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));
                continue;
            }
            perror("recvfrom");
            exit(-1);
        }
        //check the tid
        if(ntohs(sock_INF->sin_port) != tid){
            //different client
            *opcode_ptr = htons(OP_ERR);
            *(opcode_ptr+1) = htons(5);
            *(msg.packet_wrq.filename+4) = 0;
            n = sendto(socket_fd, msg.packet_wrq.filename, 5, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));
            continue;
        }

        //reset the timout counter
        count = 0;

        /* check the opcode */ 
        if(ntohs(*opcode_ptr) != OP_DATA) {
            if(ntohs(*opcode_ptr) == OP_WRQ){
                //restore last packet
                for(int i = 0; i < 517; i++)
                    msg.packet_wrq.filename[i] = backupBuffer[i];
                n = sendto(socket_fd, msg.packet_wrq.filename, last_packet_size, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));
            }
            continue;
        }

        /* At this point, the tid has been verified and it is a DATA packet */

        block = ntohs(*(opcode_ptr+1));
        if(block == last_block){
            //restore last packet
            for(int i = 0; i < 517; i++)
                msg.packet_wrq.filename[i] = backupBuffer[i];
            n = sendto(socket_fd, msg.packet_wrq.filename, last_packet_size, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));
            continue;
        }

        msg.packet_wrq.filename[n] = '\0';
        fprintf(fp, "%s\n", msg.packet_wrq.filename+4);

        if(n < 516) //last packet
            more = 0;

        //send an ack
        *opcode_ptr = htons(OP_ACK);
        *(opcode_ptr+1) = htons(block);

        //store contents of this packet
        for(int i = 0; i < 517; i++)
            backupBuffer[i] = msg.packet_wrq.filename[i];
        last_packet_size = 4;

        n = sendto(socket_fd, msg.packet_wrq.filename, last_packet_size, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));;
    }    
    fclose(fp);  
}



//rrq request (reading request)
ssize_t handle_rrq (int socket_fd, struct sockaddr_in *sock_INF,tftp_packet msg) 
{

   int tid = ntohs(sock_INF->sin_port);
   int block = 0;
   
   socklen_t sock_len;
   
   tftp_packet filePacket;
   memset(filePacket.packet_data.data,0,516);
   char backupBuffer[516];
   memset(backupBuffer,0,516);
   unsigned short * opcode_ptr = (unsigned short *)filePacket.packet_data.data;
   char filename[80];
   int count = 0;
  
  
   FILE * fp;
   strcpy(filename, msg.packet_rrq.filename); 
   fp = fopen(msg.packet_rrq.filename, "r");
  
   if(fp == NULL)
   {
      errorMsg(socket_fd, sock_INF, sock_len, "[Error] file not found",1);
      exit(EXIT_FAILURE);
   }
  
  while(1)
  {
    block++;
    //creatinga a data packet to send
    int n = 0;
    *opcode_ptr = htons(OP_DATA);
    *(opcode_ptr+1) = htons(block);
    for(n = 4; n < 516; n++)
    {
        if(feof(fp))
        {
           n -= 1;
           break;
        } 
        filePacket.packet_data.data[n] = fgetc(fp);
        
    } 
    
    for(int i = 0; i < n; i++)
    {
        backupBuffer[i] = filePacket.packet_data.data[i];
    } 
    
    
   // struct packet dataPack = dataPacket(block, buffer, n);
    int reply = sendto(socket_fd, filePacket.packet_data.data, n, 0, (struct sockaddr *)sock_INF, sizeof(*sock_INF));
    
    //wait for reply
        
    try_rec:
    reply = recvfrom(socket_fd, filePacket.packet_data.data, n, 0, (struct sockaddr *)&sock_INF,&sock_len);
        if(reply < 0) {
            if(errno == EINTR) goto try_rec;
            if(errno == EWOULDBLOCK){//1 second timeout
                if(++count >= 10){
                    printf("transaction timed out\n");
                    break;
                }
                //restore last packet
                for(int i = 0; i < n; i++)
                {
                  backupBuffer[i] = filePacket.packet_data.data[i];
                } 
              
                reply = sendto(socket_fd, filePacket.packet_data.data, reply, 0, (struct sockaddr *)sock_INF, sizeof(*sock_INF));
 
                goto try_rec;
            }
            perror("recvfrom");
            exit(-1);
        }
        //check the tid
        if(ntohs(sock_INF->sin_port) != tid){
            //different client
            *opcode_ptr = htons(OP_ERR);
            *(opcode_ptr+1) = htons(5);
            *(filePacket.packet_data.data) = 0;
            reply = sendto(socket_fd, filePacket.packet_data.data, n, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));//dest_sock_info  is sock_INF
            goto try_rec;
        }

        //reset the timout counter
        count = 0;

        /* check the opcode */ 
        if(ntohs(*opcode_ptr) != OP_ACK) {
            if(ntohs(*opcode_ptr) == OP_RRQ){
                //restore last packet
                for(int i = 0; i < n; i++)
                    filePacket.packet_data.data[i] = backupBuffer[i];
                  
                reply = sendto(socket_fd, filePacket.packet_data.data, n, 0, (struct sockaddr*)sock_INF, sizeof(sock_INF));
            }
            goto try_rec;
        }
        if(ntohs(*(opcode_ptr+1)) != block)
            goto try_rec;

        if(n < 516)
            break;
    }

    fclose(fp);
  
  return EXIT_SUCCESS;
}



// SIGNAL handlers
void sigchld_handler(int v)
{
   pid_t pidC;   
   pidC = wait(NULL); // Block parent until child completes
  
   printf("[Parent] Parent confirms child (pid=%d) finished.\n",pidC);
   
}

void sigalrm_handler()
{
   printf("[Info] Child alarm triggered. \n");
   exit(EXIT_FAILURE);
   
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
      perror("[Error] Invalid number of arguments!\n\
      Args: port_start_range port_end_range\n");
      exit(EXIT_FAILURE);
   }
   // Parse the range borders (inclusive)
   int rangeStart = atoi(argv[1]);
   int rangeEnd = atoi(argv[2]);
   int portCounter = rangeStart;

   // Set up SIGNAL handlers for when clients connect
   // or if there are SIGNAL errors 
   signal(SIGCHLD,sigchld_handler);
   signal(SIGALRM,sigalrm_handler);
   
   // Set up server socket
   struct sockaddr_in servaddr; // Stores information about the socket
   int socket_fd; // Socket file descriptor
  

   // Essential variables
   tftp_packet myPacket;
   uint16_t ret;
   unsigned short int opc; // operational code
   uint16_t* opc_ptr; // pointer for operational code
   int bres;
   
   int itr = 0;
   while(portCounter <= rangeEnd)
   {

      pid_t pv = getpid();
      printf("[Server %d] portC: %d, rangeEnd: %d\n",pv,portCounter,rangeEnd);
      // Configure a fresh connection each time we connect with a new client
      socklen_t addrlen = sizeof(servaddr); // Total bytes length of the socket description
      memset(&servaddr,0,addrlen); // Set the socket description to blank

      // tftp server setup using udp protocol
      
      socket_fd = socket(PF_INET,SOCK_DGRAM,0);
      if (socket_fd < 0)
      {
         // Error: Socket file descriptor could not be initialized
         printf("[Error %d] socket file descriptor failure!\n",pv);
         exit(EXIT_FAILURE);
      }
      servaddr.sin_family = AF_INET;
      servaddr.sin_addr.s_addr = INADDR_ANY;
      servaddr.sin_port = htons(portCounter);
      bres = bind(socket_fd,(struct sockaddr *)&servaddr,addrlen);

      if (bres < 0)
      {
         // Binding failure!
         printf("[Error iteration %d] Binding failure!\n",itr);
         exit(EXIT_FAILURE);
      }

      
     
      // Set server to listen on specific port in the range
     
      
      // Confirm the socket port
      struct sockaddr_in clientaddr;
      socklen_t slen = sizeof(clientaddr);
      getsockname(socket_fd,(struct sockaddr *)&servaddr,&addrlen);
      
      printf("[Server %d] Bound on port. Waiting for a client %d\n",pv,ntohs(servaddr.sin_port));
      ret = recvfrom(socket_fd,&myPacket,sizeof(myPacket),0,
               (struct sockaddr *)&clientaddr,&slen);
      
      if (ret < 0)
      {
         if (errno == EINTR)
         {
            continue; // TODO: saw this but no idea
         }

         // Error on packet?
         printf("[Error %d] recvfrom() failure! Could not receive from client\n",pv);
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
            myPacket.opcode = opc;
            
            alarm(10); // Put an alarm on the child
            pid_t pt = getpid();
            //printf("[Child %d] Handling request from client\n",pt);
            // We are a child process
            if (opc == OP_RRQ)
            {               
               // TODO: insert RRQ call here
               printf("[debug] packet contents: opcode: %d %s\n",myPacket.opcode,myPacket.packet_rrq.filename);
               printf("[Child %d] Handling RRQ from client\n",pt);
               handle_rrq(socket_fd,&clientaddr,myPacket);
            }
            if (opc == OP_WRQ)
            {               
               // TODO: insert WRQ call here
               printf("[debug] packet contents: opcode: %d %s\n",myPacket.opcode,myPacket.packet_wrq.filename);
               // Read all characters from the write request
               char* strMath = myPacket.packet_wrq.filename;
               uint16_t str1Len = strlen(strMath);
               char* str2 = (strMath + str1Len);
               printf("[debug] filename: %s \n data: %s\n",strMath,str2);
               
               printf("[Child %d] Handling WRQ from client\n",pt);
              handle_wrq(socket_fd,&clientaddr,myPacket);
            }
           
            return 0;
         } else {
            // We are a parent process
            // Nothing more to do
         }

      } else {
         // Our received packet after binding was 

      }    
      portCounter += 1; // Increment port counter
      itr += 1;
      close(socket_fd);
   }
   return 0;
}