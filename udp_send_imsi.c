/*
 * ============================================================================
 *
 *       Filename:  udp_send_imsi.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年06月04日 18时25分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>


#include <net/if.h>
#include <netinet/in.h>

#include "arg.h"
#include "config.h"

/* send IMSI to UDP */
static int udpsock;
static struct sockaddr_in udpaddr;
void send_string_udp(void *send_string, int send_length)
{
	if (send_string == NULL) return;
	int ret = sendto(udpsock, send_string, send_length, 0,
		(struct sockaddr *)&udpaddr, sizeof(struct sockaddr_in));
	if(ret == -1)
		printf("sendto failed! %s(%d)\n", strerror(errno), errno);
}
int init_udp_send_imsi_cli(char *ipaddr, int port)
{
	if (ipaddr == NULL) return 0;
	udpsock = socket(AF_INET, SOCK_DGRAM, 0);
	if( udpsock == -1 ) {
		printf("Creat UDP Socket failed!\n");
		return 0;
	}
	memset(&udpaddr, 0, sizeof(udpaddr)); 
	udpaddr.sin_family = AF_INET;
	udpaddr.sin_port = htons(port);
	int ret = inet_pton(AF_INET, ipaddr, &udpaddr.sin_addr.s_addr);
	if( ret == -1 ) {
		printf("report UDP init inet_pton error!\n");
		return 0;
	}
	return 1;
}

/* send UE info to UDP */
static int ue_sockfd;
static struct sockaddr_in ue_addr;

void send_ue_string_udp(void *send_string, int send_length)
{
	if (send_string == NULL) return;
	int ret = sendto(ue_sockfd, send_string, send_length, 0,
		(struct sockaddr *)&ue_addr, sizeof(struct sockaddr_in));
	if(ret == -1)
		printf("send UE info failed! %s(%d)\n", strerror(errno), errno);
}
int init_ue_udp_send_cli(char *ipaddr, int port)
{
	if (ipaddr == NULL) return 0;
	ue_sockfd= socket(AF_INET, SOCK_DGRAM, 0);
	if(ue_sockfd== -1 ) {
		printf("Creat UDP Socket (Send UE info) failed!\n");
		return 0;
	}
	memset(&ue_addr, 0, sizeof(ue_addr));
	ue_addr.sin_family = AF_INET;
	ue_addr.sin_port = htons(port);
	int ret = inet_pton(AF_INET, ipaddr, &ue_addr.sin_addr.s_addr);
	if( ret == -1 ) {
		printf("report UDP init inet_pton(Send UE info) error!\n");
		return 0;
	}
	return 1;
}
