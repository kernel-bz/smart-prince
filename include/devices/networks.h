#ifndef __NETWORKS_H
#define __NETWORKS_H

#define NET_REQUEST     "ml_result"
#define NET_START       "ml_start"

#define NET_MAX         16
#define NET_RETRY       20

extern unsigned int NetMlResult;

void* net_server_thread(void* arg);
int net_client (char *server_ip, int server_port, char *cmd);

#endif
