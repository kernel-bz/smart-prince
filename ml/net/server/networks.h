#ifndef __NETWORKS_H
#define __NETWORKS_H

int net_server (char *server_ip, int server_port);
int net_client (char *server_ip, int server_port, char *cmd);

#endif
