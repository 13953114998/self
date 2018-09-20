/*
 * =====================================================================================
 *
 *       Filename:  http.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/18/2018 02:27:20 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _HTTP_H_
#define _HTTP_H_

pthread_mutex_t http_lock;	/*http send 锁*/

void http_send(char *api, char *data);
void making_self_msg_ser(int type, dev_node_t *node);
void online_msg_send(int sockfd, int online);

#endif
