/*
 * ============================================================================
 *
 *       Filename:  mqtt.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2018 02:05:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>

#include "typeda.h"



/*解析处理后台接收到的数据*/
void analysis_after_data(char *mqt_buf)
{
	if(!mqt_buf)
	{
		printf("recev data failure\n");
		return;
	}

	cJSON *ser_obj = cJSON_Parse(mqt_buf);
	if(!ser_obj)
	{
		printf("cJSON_Parse failure\n");
		if(mqt_buf) free(mqt_buf);
		return;
	}
	cJSON *devid = cJSON_GetObjectItem(ser_obj, "devicid");
	if(!devid){
		printf("devicid failed\n");
		return;
	}
	cJSON *typestr = cJSON_GetObjectItem(ser_obj, "msgtype");
	if(!typestr){
		printf("msgtype failed\n");
		return;
	}

	dev_node_t *node = devid_10_find_node(devid->valuestring);
	if(!node)
	{
		printf("recv server msg failed(devid:%s)\n", devid->valuestring);
		if(mqt_buf) free(mqt_buf);
		return;
	}
	printf("get server sockfd: %d\n", node->sockfd);

	int type = 0;
	sscanf(typestr->valuestring, "%x", &type);

	making_dev_message(type, node);
	if(mqt_buf) free(mqt_buf);
}

void making_dev_message(int type, dev_node_t *node)
{
	if(!node)
	{
		printf("making_message node is null\n");
		return;
	}
	unsigned char *msg = (unsigned char *)calloc(1, 1024);
	if(!msg)
	{
		printf("calloc failed\n");
		exit(-1);
	}
	int len = 0;
	msg[len] = 0x28;
	++len;
	memcpy(&msg[len], node->devid_16, 6);
	len+=6;

	switch(type)
	{
		case T_GET_GPS_POSITION_YES: /*查询GPS信息*/

				msg[len] = 0x20;
				++len;
				msg[len] = 0x04;
				++len;
				msg[len] = 0x00;
				++len;
				msg[len] = 0x01;/*长度*/
				++len;
				msg[len] = 0x00;/*查询*/
				++len;
				break;
		case T_GET_CAT_AVERAGE_OIL: /*查询汽车的平均油耗*/
				pthread_mutex_lock(&oil_lock);
				oil_message = node->sockfd;
				pthread_mutex_unlock(&oil_lock);

				msg[len] = 0x30;
				++len;
				msg[len] = 0x0B;
				++len;
				msg[len] = 0x00;
				++len;
				msg[len] = 0x01;
				++len;
				msg[len] = 0x00;
				++len;
				break;
		case T_GET_CAT_DLAMEOUT: /*查询点火熄火信息*/
				making_self_msg_ser(type, node);	
				goto go_return;
		case T_GET_CAT_SPEED:	/*查询速度信息*/
				pthread_mutex_lock(&speed_lock);
				speed_message = node->sockfd;
				pthread_mutex_unlock(&speed_lock);

				msg[len] = 0x20;
				++len;
				msg[len] = 0x04;
				++len;
				msg[len] = 0x00;
				++len;
				msg[len] = 0x01;
				++len;
				msg[len] = 0x00;
				++len;
				break;
		default:
				break;
	}
	unsigned char tmp  = msg[1];
	for(int i=2; i<len; ++i)
	{
		tmp = tmp^msg[i];
	}
	msg[len] = tmp;
	++len;
	msg[len] = 0x29;
	++len;

	send_msg_to_dev(node->sockfd, msg, len);
go_return:
	if(msg) free(msg);
}

void mosquitto_set(struct mosquitto *mosq)
{
	if(con_info->name && con_info->possword)
	{
		if(mosquitto_username_pw_set(mosq, con_info->name, con_info->possword))
		{
			printf("set mqtt possword failure!\n");
			mosquitto_lib_cleanup();
			exit(-1);
		}
	}
}

void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	if(!result)
	{
		if(con_info->sub_msg)
			mosquitto_subscribe(mosq, NULL, con_info->sub_msg, 2);
		else
			exit(-1);
	}else
	{
		printf("msquitto_connack_string failure\n");
		exit(-1);
	}
}

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen)
	{
		int meslen = message->payloadlen;
		char *msg = (char *)calloc(1, meslen+1);
		if(!msg)
		{
			printf("calloc msg failure\n");
			exit(-1);
		}
		memcpy(msg, message->payload, meslen);
		msg[meslen] = '\0';
		printf("get mqtt msg is %s\n", msg);
		analysis_after_data(msg);
	}else
	{
		printf("recv message:%s (NULL)\n", message->topic);
	}
	fflush(stdout);
}
