/*
 * ============================================================================
 *
 *       Filename:  timer.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月05日 14时20分03秒
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
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
//#include "clictl_thread.h"

#include "config.h"
#include "arg.h"
#include "timer.h"

int timeradd = 0;
int timerdel = 0;
struct timerq_t timerq;

static void timer_start()
{
	assert(!list_empty(&timerq.head));

	struct itimerval relative = {{0,0}, {0,0}};
	struct timeval now;
	gettimeofday(&now, NULL);

	struct timer_t *last;
	last = list_first_entry(&timerq.head, struct timer_t, list);
	if(TV_LESS_THAN(now, last->timeout)) {
		TV_MINUS(last->timeout, now, relative.it_value);
	} else {
		relative.it_value.tv_sec = 0;
		relative.it_value.tv_usec = ALMOST_NOW;
	}

//	printf("start itimer: %ld.%ld\n", 
//		relative.it_value.tv_sec, relative.it_value.tv_usec);
	if(setitimer(ITIMER_REAL, &relative, NULL)) {
		printf("Setitimer failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}

	return;
}

static struct timer_t *timer_dequeque()
{
	if(list_empty(&timerq.head))
		return NULL;

	struct timeval now;
	gettimeofday(&now, NULL);

	struct timer_t *last;
	last = list_first_entry(&timerq.head, 
		struct timer_t, list);

	if(TV_LESS_THAN(last->timeout, now)) {
		list_del(&last->list);
		return last;
	}

	return NULL;
}

static void *chronometer(void *arg)
{
	int ret;
	struct timespec timeout;
	timeout.tv_sec = args_info.timererr_arg;
	timeout.tv_nsec= 0;

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	if(pthread_sigmask(SIG_BLOCK, &mask, NULL)) {
		printf("Pend SIGALRM failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}

	struct timer_t *ti;
	while(1) {
		pthread_mutex_lock(&timerq.mutex);
		while(list_empty(&timerq.head))
			pthread_cond_wait(&timerq.cond, &timerq.mutex);

		timer_start();
		pthread_mutex_unlock(&timerq.mutex);

		ret = sigtimedwait(&mask, NULL, &timeout);
		if(ret == EAGAIN)
			printf("Timer sigtimedwait no sigalrm\n");

retry:
		pthread_mutex_lock(&timerq.mutex);
		ti = timer_dequeque();
		pthread_mutex_unlock(&timerq.mutex);
		if(ti) {
			printf("Dequeue ti: %p\n", ti);
			timerdel++;
			ti->handler(ti->arg);
			free(ti);
			goto retry;
		}
	}
	return NULL;
}

int timer_add(long sec, long usec, void(*handler)(void *), void *arg)
{
	assert((handler != NULL) && (sec > 0 || usec > 0));

	int ret = 0;
	struct timer_t *new;
	new = (struct timer_t *)malloc(sizeof(struct timer_t));
	if (new == NULL) {
		printf("malloc new error,%s(%d)!\n", strerror(errno), errno);
		exit(-1);
	}

	pthread_mutex_lock(&timerq.mutex);

	struct timeval *ti = &new->timeout;
	gettimeofday(ti, NULL);
	ti->tv_sec += sec + (ti->tv_usec + usec) / 
		MICRO_PER_SEC;
	ti->tv_usec = (ti->tv_usec + usec) % MICRO_PER_SEC;

	new->handler = handler;
	new->arg = arg;

	timeradd++;
	/* list is empty */
	if(list_empty(&timerq.head)) {
		_list_add(&(new->list), &timerq.head);
		pthread_cond_signal(&timerq.cond);
		ret = 1;
		goto end;
	}

	struct timer_t *pos;
	list_for_each_entry_reverse(pos, &timerq.head, list) {
		if(TV_LESS_THAN(pos->timeout, new->timeout)) {
			_list_add(&(new->list), &(pos->list));
			ret = 1;
			goto end;
		}
	}

	_list_add(&(new->list), &timerq.head);
	timer_start();
	ret = 1;
end:
	pthread_mutex_unlock(&timerq.mutex);
	return ret;
}

void timer_init()
{
	printf("Timer initing...\n");
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);

	if(pthread_sigmask(SIG_BLOCK, &mask, NULL)) {
		printf("Pend SIGALRM failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
	if(pthread_mutex_init(&timerq.mutex, NULL)) {
		printf("Mutex init failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
	if(pthread_cond_init(&timerq.cond, NULL)) {
		printf("Cond init failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
	INIT_LIST_HEAD(&timerq.head);
	if(pthread_create(&timerq.ticker, NULL, chronometer, NULL)) {
		perror("Create pthread failed!");
		exit(-1);
	}
	return;
}
