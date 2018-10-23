/*
 * ============================================================================
 *
 *       Filename:  http.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年07月11日 14时06分49秒
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
#include <errno.h>
#include <string.h>

#include "http.h"
#include "arg.h"
#include "config.h"

char *send_list[10] = {NULL};/* 保存可以发送采集数据的服务器地址 */

void init_http_send_client_server_list()
{
	int i = 0, readl = 0;
	for (i = 0; i < args_info.url_given; i++) {
		if (send_list[i] != NULL) {
			free(send_list[i]);
			send_list[i] = NULL;
		}
		send_list[i] = strdup(args_info.url_arg[i]);
	}
	FILE *rfd = fopen(NOT_SEND_CLI_SERVER_FILE, "r");
	if (!rfd) return;
	char line[20] = {0};
	while (!feof(rfd)) {
		readl = fscanf(rfd, "%s\n", line);
		for (i = 0; i < args_info.url_given; i++) {
			if (send_list[i] && strstr(send_list[i], line)) {
				free (send_list[i]);
				send_list[i] = NULL;
			}
		}
		memset(line, 0, readl);
	}
	fclose(rfd);
}

size_t just_return( char *ptr, size_t size, size_t  nmemb,  void *userdata)
{
	return size * nmemb;
}

void _http_send(char *url, char *data, void *callback,
	struct curl_slist * headers)
{
	if(callback == NULL)
		callback = just_return;
	CURL *handle;
	handle = curl_easy_init();
	if(handle == NULL) {
		printf("Init curl failed:%m\n");
		exit(-1);
	}
	CURLcode ret;
	if(args_info.curldebug_flag) {
		/* set verbose */
		ret = curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
		if(ret != CURLE_OK) {
			printf("set verbose option failed:%m\n");
			exit(-1);
		}
	}
	/* http headers */
	if(headers) {
		ret = curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
		if(ret != CURLE_OK) {
			printf("set httpheader option failed:%m\n");
			exit(-1);
		}
	}
	/* set post data */
	if(data) {
		/* set post mode */
		ret = curl_easy_setopt(handle, CURLOPT_POST, 1);
		if(ret != CURLE_OK) {
			printf("set post option failed:%m\n");
			exit(-1);
		}

		ret = curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data);
		if(ret != CURLE_OK) {
			printf("set post data option failed:%m\n");
			exit(-1);
		}
	}
	if(callback != NULL) {
		/* set callback function */
		ret = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, callback);
		if(ret != CURLE_OK) {
			printf("set post callback option failed:%m\n");
			exit(-1);
		}
	}
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
	ret = curl_easy_setopt(handle, CURLOPT_TIMEOUT,
		args_info.httptimeout_arg);
	if(ret != CURLE_OK) {
		printf("set recve timeout option failed:%m\n");
		exit(-1);
	}
	ret = curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 5);
	if(ret != CURLE_OK) {
		printf("set connect timeout option failed:%m\n");
		exit(-1);
	}
	ret = curl_easy_setopt(handle, CURLOPT_URL, url);
	if(ret != CURLE_OK) {
		printf("set url option failed:%m\n");
		exit(-1);
	}

	ret = curl_easy_perform(handle);
	if(ret != CURLE_OK) {
		if (ret == CURLE_HTTP_POST_ERROR)
			printf("curl perform failed:post error!\n");
		else if (ret == CURLE_COULDNT_CONNECT)
			printf("curl perform failed:cannot connect to server\n");
		else if (ret == CURLE_REMOTE_ACCESS_DENIED)
			printf("curl perform failed: connection refused\n");
		else
			printf("curl perform failed\n");
	} else {
		printf("curl perform success:%s\n", url);
	}
	curl_easy_cleanup(handle);
	if(headers)
		curl_slist_free_all(headers);
	return;
}

void http_send(char *api, char *send_string, void *callback,
	struct curl_slist * headers)
{
	unsigned int url_num = args_info.url_given;
	unsigned int i = 0;
	char send_url[128];
	for (i = 0; i < url_num; i++) {
		memset(send_url, 0, sizeof(send_url));
		snprintf(send_url, 128, "%s/%s", args_info.url_arg[i], api);
		printf("send url:%s\n", send_url);
		_http_send(send_url, send_string, callback, headers);
	}
}
void http_send_client(char *api, char *send_string, void *callback,
	struct curl_slist * headers)
{
	unsigned int url_num = args_info.url_given;
	unsigned int i = 0;
	char send_url[128];
	for (i = 0; i < url_num; i++) {
		if (send_list[i] != NULL) {
			memset(send_url, 0, sizeof(send_url));
			snprintf(send_url, 128, "%s/%s", send_list[i], api);
			printf("send url:%s\n", send_url);
			_http_send(send_url, send_string, callback, headers);
		}
	}
}
