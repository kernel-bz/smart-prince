 /**
 *  file name:  networks.c
 *  author:     JungJaeJoon(rgbi3307@nate.com)
 *	  comments:   TCP/IP Networks Module
 *  Copyright (c) www.kernel.bz
 *  This is under the GPL License (see <http://www.gnu.org/licenses/>)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "common/config.h"
#include "devices/networks.h"

#define BUFF_SIZE   200

unsigned int NetMlResult=1;

void* net_server_thread(void* arg)
{
    int   server_socket;
    int   client_socket;
    int   client_addr_size;
    struct sockaddr_in   server_addr;
    struct sockaddr_in   client_addr;
    char   buff_rcv[BUFF_SIZE+5];
    char   buff_snd[BUFF_SIZE+5];
    int  option, length;

   //server_socket  = socket(PF_INET, SOCK_STREAM, 0);
   server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (-1 == server_socket) {
      printf( "server socket create error!\n");
      return 0;
   }

   memset( &server_addr, 0, sizeof( server_addr));
   server_addr.sin_family     = AF_INET;
   server_addr.sin_port       = htons(RunCfg.port);
   //server_addr.sin_addr.s_addr= htonl( INADDR_ANY);
   //server_addr.sin_addr.s_addr= inet_addr( "127.0.0.1");
   server_addr.sin_addr.s_addr= inet_addr(RunCfg.ip);	//192.168.16.212

   option = 1;          //SO_REUSEADDR 의 옵션 값을 TRUE(bind error clear)
   setsockopt( server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option) );

   if (-1 == bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr) ) ) {
      printf( "bind() error!\n");
      return 0;
   }

   while(1)
   {
        if( -1 == listen(server_socket, 5)) {
             printf( "listen() error!\n");
             return 0;
        } else {
            ///printf("listening...\n");
        }

        client_addr_size  = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_size);

        if ( -1 == client_socket) {
            printf( "accept() error!\n");
            return 0;
        }

        //length = read (client_socket, buff_rcv, BUFF_SIZE);
        length = recv(client_socket, buff_rcv, BUFF_SIZE, 0);
        if (length > 0) {
            ///printf("RECV: length=%d, client_socket=%d, client_ip=%s\n"
            ///    , length, client_socket, inet_ntoa(client_addr.sin_addr));
            printf("Net Recv: %s\n", buff_rcv);

            if (strstr(buff_rcv, NET_START)) {
                NetMlResult = 0;
                sprintf(buff_snd, "%u\n", NetMlResult);
                printf("Net Send Start: %s\n", buff_snd);
            } else if (strstr(buff_rcv, NET_REQUEST)) {
                sprintf(buff_snd, "%u\n", NetMlResult);
                printf("Net Send Result: %s\n", buff_snd);
            }
            if (NetMlResult > 0) NetMlResult = 1;   ///Sended
        }
        write( client_socket, buff_snd, strlen( buff_snd)+1);   //+1: NULL까지 포함해서 전송
        close( client_socket);
        usleep(20000); ///20ms
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
      printf( "connect() error(server_ip=%s)!\n", server_ip);
      return -3;
   }
   write( client_socket, cmd, strlen(cmd)+1);      //NULL까지 포함해서 전송

   read ( client_socket, buff, BUFF_SIZE);
   //printf( "ML Server Result: %s\n", buff);
   close( client_socket);

   return atoi(buff);
}
