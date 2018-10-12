#ifndef _TCP_DEVID_H_
#define _TCP_DEVID_H_

#define RECVSIZE 10240
#define LISTENNUM 20

void init_tcp_sockfd();
void init_epoll();
int connect_server();
void *thread_connect_client();
int send_to_msg(char *msg, int fd);
int recv_from_msg(int fd);
void *thread_recv_form_server();
void atart_up_client(int fd);
#endif
