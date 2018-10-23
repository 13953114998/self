/*
 * =====================================================================================
 *
 *       Filename:  http.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年07月11日 17时32分56秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __HTTP_H__
#define __HTTP_H__

#include <curl/curl.h>
#define NOT_SEND_CLI_SERVER_FILE "/mnt/sd/not_send"
pthread_mutex_t http_send_lock; // http_send调用锁

void init_http_send_client_server_list();
void _http_send(char *url, char *data, void *callback,
	struct curl_slist * headers);
void http_send(char *api, char *send_string, void *callback,
	struct curl_slist * headers);
void http_send_client(char *api, char *send_string, void *callback,
	struct curl_slist * headers);
#endif /* __HTTP_H__ */
