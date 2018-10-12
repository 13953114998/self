/*
 * =====================================================================================
 *
 *       Filename:  timer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月05日 14时20分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#include <sys/time.h>
#include <pthread.h>

#include "list.h"

extern int timeradd;
extern int timerdel;
extern struct timerq_t timerq;

struct timer_t {
	struct timeval timeout;
	void(* handler)(void *);
	void *arg;

	struct list_head list;
};

struct timerq_t {
	struct list_head head;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	pthread_t ticker;
};
                                 
#define INIT_WAIT_TIME  5
#define NANO_PER_MICRO  1000

#define ALMOST_NOW      1000
#define MAX_USEC        999999
#define MICRO_PER_SEC   1000000
#define TV_LESS_THAN(t1,t2)	(((t1).tv_sec < (t2).tv_sec) || \
	(((t1).tv_sec == (t2).tv_sec) && 			\
	 ((t1).tv_usec < (t2).tv_usec)))

#define TV_MINUS(t1,t2,tgt) 					\
if ((t1).tv_usec >= (t2).tv_usec) { 				\
	(tgt).tv_sec = (t1).tv_sec - (t2).tv_sec; 		\
	(tgt).tv_usec = (t1).tv_usec - (t2).tv_usec; 		\
} else { 							\
	(tgt).tv_sec = (t1).tv_sec - (t2).tv_sec -1; 		\
	(tgt).tv_usec = (t1).tv_usec + (MAX_USEC - (t2).tv_usec);\
}

int timer_add(long sec, long usec, void(*handler)(void *), void *arg);
void timer_init();
#endif /*__TIMER_H__*/
