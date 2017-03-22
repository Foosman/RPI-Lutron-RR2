//
// Auhor: Foosman
// August 1, 2014
// Citations:
// Silver Moon, www.binarytides.com/socket-programming-c-linux-tutorial
// Gordon, WiringPi, wiringpi.com

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>    //write
#include <time.h>

#define WAIT_TIME_FOR_RESPONSE (int) 1 
#define SOCKET_RECEIVE_TIMEOUT (int) 10
#define REPEATER_IP_ADDRESS "192.168.0.115"
#define BUF_SIZE 64000 
#define MAX_LINES 256 
#define MAX_LINE_SIZE 512 
#define GET_COMMAND (const char *) "GET /DbXmlInfo.xml HTTP/1.1\r\nHost: 192.168.0.206:44444\r\nConnection: close\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, likeGecko) Chrome/51.0.2704.103 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nAccept-Encoding: gzip, deflate, sdch\r\nAccept-Language: en-US,en;q=0.8\r\n\r\n"

#define DBG 1
#define IFDBG if(DBG==1)printf

// Global variables

int main (int argc, char *argv[])
{
   char buf[BUF_SIZE];
   int ret;
   int socket_desc;
   struct sockaddr_in server;
   char output[MAX_LINES][MAX_LINE_SIZE];
   int num_lines;
   int i; 
   int xml_dump=0;

   if(argc < 2)
   {
     puts("xml_parse called incorrectly.  Usage: xml_parse <ip address of lutron repeater>");
     puts("Ex: xml_parse 192.168.0.115 or xml_parse 192.168.0.115 -debug");
     return 0; 
   }
   
   if(argc > 2)
   {
     if(!strncmp(argv[2],"-debug",6))
     {
      xml_dump=1;
     }
   }

   socket_desc = socket(AF_INET, SOCK_STREAM, 0);

   if (socket_desc == -1)
   {
    printf("Could not create socket\n");
    return -1;
   }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(80); 

    //Connect to repeater
    if (connect(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
     printf("connect error");
     return -2;
    }

    struct timeval tv;
    tv.tv_sec = SOCKET_RECEIVE_TIMEOUT; //  sec timeout
    tv.tv_usec = 0;
    setsockopt(socket_desc,SOL_SOCKET,SO_RCVTIMEO,(char *)&tv,sizeof(struct timeval));
    IFDBG("Successful connection to Lutron repeater\n");

    ret = send_get_request(socket_desc, buf, xml_dump);
    close(socket_desc);

    num_lines = split(buf,"Output Name=",output);
    for(i=0; i < num_lines; i++) 
       printf("Light %d: %s\n",i,output[i]);

    return 1;
}


//------------------
// send_get_request
//------------------
int send_get_request(int socket_desc, char *buf, int xml_dump) 
{
    int ret;
    time_t clk = time(NULL);

    //Send HTTP GET command
    strcpy(buf,GET_COMMAND);
    strcat(buf,"\r\n"); 
    if(send(socket_desc,buf,strlen(buf), 0) < 0)
    {
        printf("Send failed");
        return -1;
    }

    sleep(WAIT_TIME_FOR_RESPONSE);

    //Receive a reply from the server
    ret=1;
    buf[0]='\0';
    // MSG_WAITALL = Block until full request is fulfilled, needed as there is a lot of data
    ret = recv(socket_desc,buf,BUF_SIZE-1,MSG_WAITALL); 
    if(ret < 0)
    {
       printf("Test Command Response failed\n");
       return -2;
    }
    
    if(xml_dump)
    {
     printf("%s\n",buf);
    }

    IFDBG("Size of GET request response: %d\n",ret);

    return 1;
}



//------------------------------------------
int split(char *expr, char *match_expr, char output[][MAX_LINE_SIZE])
//------------------------------------------
{

   char *src_ptr;
   char *dest_ptr;
   char word[MAX_LINES][MAX_LINE_SIZE];
   int index;
   int quote_index;

   int match_expr_len = strlen(match_expr);

   // Initialize all words to NULL
   for(index=0; index < MAX_LINES; index++)
     output[index][0]= '\0';

   src_ptr = expr;
   index=0;
   quote_index = 0;
   dest_ptr = output[0];

   while((*src_ptr != '\0') && (index < MAX_LINES))
   {
     // <Output Name="Kitchen: sink" IntegrationID="26"
     if(!strncmp(src_ptr, match_expr, match_expr_len))
     {
      *dest_ptr = '\0';
      // Copy until 4th " is encountered
      dest_ptr = output[index];
      while(quote_index < 4)
      {
        if(*src_ptr == '\"')
          quote_index++;
 	*dest_ptr = *src_ptr;	
        src_ptr++;
        dest_ptr++;
      }
      index++;
      quote_index = 0;
     }

     else
       {
        src_ptr++;
       } 
   }
   return index;
}
