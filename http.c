/*
 * ============================================================================
 *
 *       Filename:  http.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/18/2018 12:56:43 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>

#include "typeda.h"

pthread_mutex_t http_lock;

size_t just_return(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return size * nmemb;
}

/*基于curl建立http post方法
 *使用http协议发送指定数据到指定url
*/
void http_send(char *api, char *data)
{
	if(!api || !data)
	{
		printf("http_send meter is null(meter:%s, %s)\n", api, data);
		return;
	}
	pthread_mutex_lock(&http_lock);/*加锁*/
	char  url_buf[128] = {0};
	sprintf(url_buf, "%s/%s", con_info->http_url, api);
	printf("send to url:%s\nmessage:%s\n", url_buf, data);
	
	/*初始化http*/
	CURL *handht = curl_easy_init();
	if(!handht)
	{
		printf("init curl failure\n");
		exit(-1);
	}
	CURLcode ret;

	/*设置http报告每个异常*/
	ret = curl_easy_setopt(handht, CURLOPT_VERBOSE, 1);
	if(ret != CURLE_OK)
	{
		printf("set verbose option failure\n");
		exit(-1);
	}
	/*添加数据邮件*/
	ret = curl_easy_setopt(handht, CURLOPT_POST, 1);
	if(ret != CURLE_OK)
	{
		printf("set post option failure\n");
		exit(-1);
	}
	ret = curl_easy_setopt(handht, CURLOPT_POSTFIELDS, data);
	if(ret != CURLE_OK)
	{
		printf("set post data option failure\n");
		exit(-1);
	}
	/*设置一个回调函数*/
	void *callback = just_return;
	ret = curl_easy_setopt(handht, CURLOPT_WRITEFUNCTION, callback);
	if(ret != CURLE_OK)
	{
		printf("set post callback option failure\n");
		exit(-1);
	}
	curl_easy_setopt(handht, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(handht, CURLOPT_SSL_VERIFYHOST, 0L);
	/*设置超时回收时间*/
	ret = curl_easy_setopt(handht, CURLOPT_TIMEOUT, con_info->http_timeout);
	if(ret != CURLE_OK)
	{
		printf("set recve timeout option failure\n");
		exit(-1);
	}
	ret = curl_easy_setopt(handht, CURLOPT_CONNECTTIMEOUT, 5);
	if(ret != CURLE_OK)
	{
		printf("set connect timeout option failure\n");
		exit(-1);
	}
	ret = curl_easy_setopt(handht, CURLOPT_URL, url_buf);
	if(ret != CURLE_OK)
	{
		printf("set url option failure\n");
		exit(-1);
	}
	ret = curl_easy_perform(handht);
	if(ret != CURLE_OK)
	{
		if(ret == CURLE_HTTP_POST_ERROR)
			printf("curl perform failed: post error\n");
		else if(ret == CURLE_COULDNT_CONNECT)
			printf("curl perform failed: connect error\n");
		else if(ret == CURLE_REMOTE_ACCESS_DENIED)
			printf("curl perform failed: connection refused\n");
		else 
			printf("curl perform failed\n");
	}else
	{
		printf("curl perform success:%s\n", url_buf);
	}
	curl_easy_cleanup(handht);
	pthread_mutex_unlock(&http_lock);/*解锁*/
}

//查询本地发送http
void making_self_msg_ser(int type, dev_node_t *node)
{
	char str[256] = {0};
	switch(type)
	{
		case T_GET_CAT_DLAMEOUT:/*查询点火熄火信息*/
			snprintf(str, 256, 
					"{\"devicid\":\"%s\",\"msgtype\":\"%d\","
					"\"flameout\":\"%d\"}", node->devid_10,
					type, node->flameout);

			http_send(con_info->http_dlameout, str);
			break;
		default:
			printf("无法识别的消息！\n");
			break;
	}
}

/*上线离线信息上报*/
void online_msg_send(int sockfd, int online)
{
	if(sockfd < 0||(online != 0 && online != 1))
	{
		printf("send online msg failed\n");
		return;
	}
	dev_node_t *node = sockfd_find_node(sockfd);
	if(!node) return;
	char str[256] = {0};
	snprintf(str, 256, 
				"{\"devicid\":\"%s\",\"msgtype\":\"%d\","
				"\"online\":\"%d\"}", node->devid_10,
				T_PUT_DEV_ONLINE_MSG, online);
	http_send(con_info->http_online, str);
}
