#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/reboot.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include "cJSON.h"
#include "tcp_devid.h"
#include "bt_model_action.h"
#include "arg.h"

int sockfd = 0;/*tcp套接字*/
int epoll_sockfd = 0;

extern pthread_mutex_t blue_lock;

void init_tcp_sockfd()
{
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
    perror("socket");
    exit(-1);
  }

  struct sockaddr_in tcpaddr;
  memset(&tcpaddr, 0, sizeof(struct sockaddr_in));

  /*服务端*/
  if(args_info.tcp_state_arg)
  {
    tcpaddr.sin_family = AF_INET;
    tcpaddr.sin_port = htons(args_info.tcp_port_arg);
    tcpaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuse = 1;
   if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
   {
	perror("setsockopt");
	exit(-1);
   }
    /* bind */
    if(bind(sockfd, (struct sockaddr *)&tcpaddr, sizeof(struct sockaddr)) != 0)
    {
      perror("bind");
      close(sockfd);
      exit(-1);
    }
  }
  printf("init tcp....\n");
}

/*初始化epoll*/
void init_epoll()
{
  if(epoll_sockfd == 0)
    epoll_sockfd = epoll_create(1);
  if(epoll_sockfd == -1)
  {
    perror("epoll_create");
    exit(-1);
  }
  printf("create epoll......\n");
}

/*客户端连接服务器*/
int connect_server()
{
  if(sockfd < 0)
  {
    perror("sockfd");
    return -1;
  }
  struct sockaddr_in tcpaddr;
  memset(&tcpaddr, 0, sizeof(struct sockaddr_in));

  tcpaddr.sin_family = AF_INET;
  tcpaddr.sin_port = htons(args_info.tcp_port_arg);
  tcpaddr.sin_addr.s_addr = inet_addr(args_info.tcp_ip_arg);

  sleep(2);
  if(connect(sockfd, (struct sockaddr *)&tcpaddr, sizeof(struct sockaddr)) < 0)
  {
    perror("connect");
    return -1;
  }
  printf("connect success......\n");

  return 0;
}

/*服务器端等待客户端连接*/
void *thread_connect_client()
{
  if(sockfd <= 0 && epoll_sockfd <= 0)
  {
    perror("sockfd");
    return NULL;
  }

  struct epoll_event event;
  struct epoll_event *event_num;

  if(listen(sockfd, LISTENNUM) < 0)
  {
    perror("listen");
    exit(-1);
  }
  
  event.data.fd = sockfd;
  event.events = EPOLLIN || EPOLLET;
  if(epoll_ctl(epoll_sockfd, EPOLL_CTL_ADD, sockfd, &event))
  {
    perror("epoll_ctl");
    exit(-1);
  }
  event_num = calloc(5, sizeof(struct epoll_event));
  if(!event_num)
  {
    perror("calloc");
    exit(-1);
  }

  while(1)
  {
    int num = 0;
    printf("epoll wait.....\n");
    num = epoll_wait(epoll_sockfd, event_num, 5, -1);
    for(int i=0; i<num; ++i)
    {
      if((event_num[i].events & EPOLLERR) ||
          (event_num[i].events & EPOLLHUP) ||
          (!(event_num[i].events & EPOLLIN)))
      {
        printf("epoll error(sockfd:%d)\n", event_num[i].data.fd);
        close(event_num[i].data.fd);
        continue;
      }else if(event_num[i].data.fd == sockfd)
      {
        struct sockaddr_in cliaddr;
        socklen_t lenth = sizeof(cliaddr);
        int conn = accept(sockfd, (struct sockaddr *)&cliaddr, &lenth);
        if(conn < 0)
        {
          perror("accept");
          exit(-1);
        }
        printf("client connect(sockfd:%d).....\n", conn);
        memset(&event, 0, sizeof(event));
        event.data.fd = conn;
        event.events = EPOLLIN | EPOLLET;
        if(epoll_ctl(epoll_sockfd, EPOLL_CTL_ADD, conn, &event))
        {
          perror("epoll_ctl");
          exit(-1);
        }
        atart_up_client(conn);
      }else
      {
        if(recv_from_msg(event_num[i].data.fd) <= 0)
		      close(event_num[i].data.fd);
      }
    }
  }
}

/*发送消息*/
int send_to_msg(char *msg, int fd)
{
  if(!msg)
    return 0;

  int ret = write(fd, msg, strlen(msg)+1);
  if(ret < 0)
  {
    perror("write");
  }else
  {
    printf("send msg success : %s\n", msg); 
  }
  
  return ret;
}

/*接收消息*/
int recv_from_msg(int fd)
{
  int ret = 0;
  char msg[RECVSIZE] = {0};

  ret = read(fd, msg, RECVSIZE);
  if(ret < 0)
  {
    perror("read error");
  }else if(ret == 0)
  {
    printf("a sockfd quit (sockfd: %d)\n", fd);
    if(!(args_info.tcp_state_arg))
      exit(-1);
  }else 
  {
   if(args_info.tcp_state_arg)
   {
	  printf("recv message: %s\n", msg);
    pthread_mutex_lock(&blue_lock);
    write_to_blue_tooth(get_bluetooth_fd(), msg);
    pthread_mutex_unlock(&blue_lock);
   }else
   {
	   char *data = calloc(1, RECVSIZE);
	   if(!data)
	   {
	   	perror("calloc");
		  exit(-1);
	   }
	   memcpy(data, msg, RECVSIZE);
    safe_pthread_create(thread_pare_bluetooth_cmd_buf, (void *)data);
   }
  }
  return ret;
}

void *thread_recv_form_server()
{
  if(sockfd <= 0) return NULL;

  int num = 0;

  while(connect_server())
  {
  	sleep(2);
	  if(++num >= 50)
	  {
		  printf("device reboot\n");
		  sleep(5);
		  sync();
		  reboot(RB_AUTOBOOT);
	  }
  }
  while(1)
  {
    sleep(2);
    recv_from_msg(sockfd);
  }
}

void atart_up_client(int fd)
{
  char buf[256] = {0}; 
  snprintf(buf, 256, 
        "%s{\"type\":\"%x\",\"sweep\":\"%d\"}", 
        CMD_START, DEVICE_SCAN_ARFCN_REQ, 1);
  int ret = send_to_msg(buf, fd);
  if(ret < 0) return;
}
