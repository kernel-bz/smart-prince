/**
 *	file name:  networks.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   TCP/IP Networks Module
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFF_SIZE   200
#define WhoIAm      "GET /esp_wifi/ HTTP/1.1"

int Relay_Index = 0;

int net_server (char *server_ip, int server_port)
{
    int   server_socket;
    int   client_socket;
    int   client_addr_size;
    struct sockaddr_in   server_addr;
    struct sockaddr_in   client_addr;
    char   buff_rcv[BUFF_SIZE+5];
    char   buff_snd[BUFF_SIZE+5];
    int  option, length;
    char *relay_switch[] = {"UNKNOWN", "GPON1", "GPON2", "GPON3", "GPON4", "GOFF1", "GOFF2", "GOFF3", "GOFF4"};

   //server_socket  = socket(PF_INET, SOCK_STREAM, 0);
   server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (-1 == server_socket) {
      printf( "server socket create error!\n");
      return -1;
   }

   memset( &server_addr, 0, sizeof( server_addr));
   server_addr.sin_family     = AF_INET;
   server_addr.sin_port       = htons(server_port);
   //server_addr.sin_addr.s_addr= htonl( INADDR_ANY);
   //server_addr.sin_addr.s_addr= inet_addr( "127.0.0.1");
   server_addr.sin_addr.s_addr= inet_addr(server_ip);	//192.168.16.212

   option = 1;          //SO_REUSEADDR 의 옵션 값을 TRUE(bind error clear)
   setsockopt( server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option) );

   if (-1 == bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr) ) ) {
      printf( "bind() error!\n");
      return -1;
   }

   while(1)
   {
        if( -1 == listen(server_socket, 5)) {
             printf( "listen() error!\n");
             return -1;
        } else {
            printf("listening...\n");
        }

        client_addr_size  = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_size);

        if ( -1 == client_socket) {
            printf( "accept() error!\n");
            return -1;
        }

        //length = read (client_socket, buff_rcv, BUFF_SIZE);
        length = recv(client_socket, buff_rcv, BUFF_SIZE, 0);
        if (length > 0) {
            printf("RECV: length=%d, client_socket=%d, client_ip=%s\n"
                , length, client_socket, inet_ntoa(client_addr.sin_addr));
            ///printf("DATA: %s\n", buff_rcv);
            if (strstr(buff_rcv, WhoIAm)) {
                sprintf(buff_snd, "\n%s\n", relay_switch[Relay_Index]);
                printf(buff_snd);
            }
        }
        write( client_socket, buff_snd, strlen( buff_snd)+1);   //+1: NULL까지 포함해서 전송
        close( client_socket);
   }
}


int net_client (char *server_ip, int server_port, char *cmd)
{
   int   client_socket;
   struct sockaddr_in   server_addr;
   char   buff[BUFF_SIZE+5];

   client_socket  = socket( PF_INET, SOCK_STREAM, 0);
   if( -1 == client_socket)
   {
      printf("socket create error!\n");
      return -2;
   }

   memset(&server_addr, 0, sizeof( server_addr));
   server_addr.sin_family     = AF_INET;
   server_addr.sin_port       = htons(server_port);
   server_addr.sin_addr.s_addr= inet_addr(server_ip);

   if( -1 == connect( client_socket, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
   {
      printf( "connect() error!\n");
      return -3;
   }
   write( client_socket, cmd, strlen(cmd)+1);      //NULL까지 포함해서 전송

   read ( client_socket, buff, BUFF_SIZE);
   printf( "RF Test Result: %s\n", buff);
   close( client_socket);

   if (strcmp(cmd, buff)) return -1;	//FAIL
   else return 1;	//OK
}
