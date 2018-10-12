/*
 * =====================================================================================
 *
 *       Filename:  epoll_active.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年06月10日 09时57分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __EPOLL_ACTIVE_H__
#define __EPOLL_ACTIVE_H__
#include <sys/epoll.h>
#define ALIGN(x,a)    (((x)+(a)-1)&~(a-1)) 

#define MAXWAIT 	(1024)

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

struct cliinfo {
	int fd;
	char *addr;
};

int Epoll_init();
int Epoll_del(struct cliinfo *info);
int server_socket_init();
int Epoll_add(int fd, char *cliaddr);
int Epoll_wait(struct epoll_event *ev);
#endif /* __EPOLL_ACTIVE_H__ */
