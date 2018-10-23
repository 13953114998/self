/*
 * ============================================================================
 *
 *       Filename:  net.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年06月10日 09时32分27秒
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
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "arg.h"
#include "epoll_active.h"
#include "config.h"
#define SERVER_PORT 43200
int epoll_fd = -1;

int server_socket_init(int listen_port)
{
	int ret, listenfd;
	struct sockaddr_in server_addr;

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(listen_port);

	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if(listenfd == -1) {
		printf("socket failed %s(%d)", strerror(errno), errno);
		return -1;
	}
	int flag = 1;
	ret =setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(ret < 0) {
		printf("Set socket failed: %s(%d)\n",
			strerror(errno), errno);
		close(listenfd);
		return -1;
	}
	struct timeval timeout = {10,0};
	ret = setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO,
		&timeout, sizeof(timeout));
	if(ret < 0) {
		printf("Set socket recv time out failed: %s(%d)\n",
			strerror(errno), errno);
		close(listenfd);
		return -1;
	}
	struct timeval s_timeout = {2, 0};
	ret = setsockopt(listenfd, SOL_SOCKET, SO_SNDTIMEO,
		&s_timeout, sizeof(s_timeout));
	if(ret < 0) {
		printf("Set socket send time out failed: %s(%d)\n",
			strerror(errno), errno);
		close(listenfd);
		return -1;
	}

	ret = bind(listenfd, (struct sockaddr *)&server_addr,
		sizeof(struct sockaddr));
	if (ret == -1) {
		printf("bing socket failed %s(%d)", strerror(errno), errno);
		close(listenfd);
		return -1;
	}
	return listenfd;
}

int Epoll_init()
{
	if (epoll_fd == -1)
		epoll_fd = epoll_create(1);
	if(epoll_fd == -1) {
		printf("epoll create failed %s(%d)",
			strerror(errno), errno);
		return -1;
	}
	printf("Epoll inited!\n");
	return 0;
}

int Epoll_wait(struct epoll_event *ev)
{
	int ret;
	ret = epoll_wait(epoll_fd, ev, MAXWAIT, -1);
	if(ret < 0) {
		printf("epoll_wait failed: %s(%d)\n",
			strerror(errno), errno);
	}
	return ret;
}

int Epoll_add(int fd, char *cliaddr)
{
	assert(fd > 0);
	struct cliinfo *cli = malloc(sizeof(struct cliinfo));
	if (!cli)
		return -1;
	struct epoll_event *ep = malloc(sizeof(struct epoll_event));
	if (!ep) {
		free(cli);
		return -1;
	}
	cli->fd = fd;
	cli->addr = strdup(cliaddr);
	ep->events = EPOLLIN;
	ep->data.ptr = cli;

	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, ep);
	if(ret == -1) {
		printf("epoll ctl failed %s(%d)",
			strerror(errno), errno);
		return -1;
	}
	return 0;
}

int Epoll_del(struct cliinfo *info)
{
	int fd;
	fd = info->fd;
	assert(fd > 0);
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	if(ret == -1) {
		printf("epoll ctl failed %s(%d)",
			strerror(errno), errno);
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}
