/*
 * =====================================================================================
 *
 *       Filename:  tcp_action.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2018 10:49:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _TCP_ACTION_H_
#define _TCP_ACTION_H_

#define PORT		5050
#define MAX_LISTEN	1024
#define MAX_EVENT	1024

struct dev_data
{
	int sockfd;
	unsigned char *buf;
	int len;
};

/*服务器是否下发查询油耗消息*/
int oil_message;
pthread_mutex_t oil_lock;

/*服务器是否下发查询速度消息*/
int speed_message;
pthread_mutex_t speed_lock;

//int tcp_sockfd;
//int epoll_sockfd;

char *inttox(int num);
unsigned int xtoint(unsigned char str);
void init_sockfd_msg();
void init_epoll();
void send_msg_to_dev(int sockfd,unsigned char *str, int len);
void *p_recv_dev_message(struct dev_data *msg_buf);
void slation_data(int sockfd, unsigned char *buf, int buf_len);
void *p_tcp_recv_msg();

#endif
