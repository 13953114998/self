/*
 * ============================================================================
 *
 *       Filename:  udp_gsm.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年08月30日 09时13分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  weichao lu (Chao), lwc_yx@163.com
 *   Organization:  
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hash_list_active.h"
#include "cJSON.h"
#include "config.h"
#ifdef SCACHE
#include "cache_file.h"
#endif
#include "http.h"
#include "cmdline.h"
#include "arg.h"
#include "epoll_active.h"
#include "mosquit_sub.h"
#ifdef SAVESQL
#include "save2sqlite.h"
#endif
#include "fiberhome_lte.h"
#include "cli_action.h"
#include "baicell_net.h"
#include "udp_gsm.h"

band_entry_t *entry;
int udp_sockfd ,send_sockfd;
socklen_t addrlen = sizeof(struct sockaddr);
int gsm_msg_num;

static int cap_time;
struct sockaddr_in send_addr;

cJSON *gsm_info_object; // 存储发送的用户信息的json
cJSON *gsm_info_array;  // 存储发送的用户列表的json列表
int gsm_info_count = 0; // 当前array中的元素个数
pthread_mutex_t gsm_js_object_lock; // 锁，拿到锁才能对-|gsm_info_object 进行操作
//                                                  	|gsm_info_array

#define GSM_SIG_ONE (0) //GSM载波1
#define GSM_SIG_TWO (1) //GSM载波2
void gsm_cli_object_lock_init()
{
	pthread_mutex_init(&gsm_js_object_lock, NULL); // 初始化GSM用户信息json锁
}
void gsm_cli_object_lock()
{
	pthread_mutex_lock(&gsm_js_object_lock);
}
void gsm_cli_object_unlock()
{
	pthread_mutex_unlock(&gsm_js_object_lock);
}

void gsm_info_object_init()
{
	gsm_info_object = cJSON_CreateObject();
	if (gsm_info_object == NULL) exit(0);
	gsm_info_array = cJSON_CreateArray();
	if (gsm_info_array == NULL) {
		cJSON_Delete(gsm_info_object);
		exit(0);
	}
	cJSON_AddStringToObject(gsm_info_object, "topic", my_topic);
	cJSON_AddItemToObjectCS(gsm_info_object,"clients", gsm_info_array);
	gsm_info_count = 0;
	printf("GSM Station json object Inited!\n");
}

void gsm_info_object_delete()
{
	cJSON_Delete(gsm_info_object);
}



cJSON *gsm_scan_info_object; // 存储发送的用户信息的json
cJSON *gsm_scan_info_array;  // 存储发送的用户列表的json列表
int gsm_scan_info_count = 0; // 当前array中的元素个数
pthread_mutex_t gsm_scan_js_object_lock; // 锁，拿到锁才能对-|gsm_scan_info_object 进行操作
					 //   		     |gsm_scan_info_array

#define MAX_GSM_INFO_COUNT (200) //用户信息JSON最大元素个数(大于等于这个值，需要发送)
#define MAX_GSM_SCAN_INFO_COUNT (10) //扫描信息JSON最大元素个数(大于等于这个值，需要发送)
#define MAX_GSM_SCAN_INFO_SLEEP_TIME (30)  //检查扫描信息JSON结构间隔时间

static void _gsm_json_arry_init(cJSON **array)
{
	*array = cJSON_CreateArray();
	if (!(*array)) {
		printf("Init json array error!\n");
		exit(1);
	}
}

/*JSON初始化*/
static void _gsm_json_object_init(cJSON **object)
{
	*object = cJSON_CreateObject();
	if (!(*object)) {
		printf("in json init creat json object error!\n");
		exit(1);
	}

	cJSON_AddStringToObject(*object, "topic", my_topic);
	if(entry)
		cJSON_AddStringToObject(*object, "ip", entry->ipaddr);
	else
		cJSON_AddStringToObject(*object, "ip", GSM_IP_ADDR);
	if(*object == gsm_info_object) {
		_gsm_json_arry_init(&gsm_info_array);
		cJSON_AddItemToObjectCS(*object,"clients", gsm_info_array);
	} else {
		_gsm_json_arry_init(&gsm_scan_info_array);
		cJSON_AddItemToObjectCS(*object,"clients", gsm_scan_info_array);
	}
}

#if 0
/*初始化GSM用户信息JSON*/
static void gsm_json_init()
{
	_gsm_json_object_init(&gsm_info_object);
	cJSON_AddStringToObject(gsm_info_object, "topic", my_topic);
	if(entry)
		cJSON_AddStringToObject(gsm_info_object, "ip", entry->ipaddr);
	else
		cJSON_AddStringToObject(gsm_info_object, "ip", GSM_IP_ADDR);
	_gsm_json_arry_init(&gsm_info_array);
	cJSON_AddItemToObjectCS(gsm_info_object,"clients", gsm_info_array);
}

/*初始化GSM扫描信息JSON*/
static void gsm_scan_json_init()
{
	_gsm_json_object_init(&gsm_scan_info_object);
	cJSON_AddStringToObject(gsm_scan_info_object, "topic", my_topic);
	if(entry)
		cJSON_AddStringToObject(gsm_scan_info_object, "ip", entry->ipaddr);
	else
		cJSON_AddStringToObject(gsm_scan_info_object, "ip", GSM_IP_ADDR);
	_gsm_json_arry_init(&gsm_scan_info_array);
	cJSON_AddItemToObjectCS(gsm_scan_info_object,"clients", gsm_scan_info_array);
}
#endif

/*处理服务器下发的其他请求（开启/关闭射频  扫描）*/
void handle_send_to_gsm(WrtogsmHeader_t *WrtogsmHeader, U32 carrier_t)
{
	U8 send_request[MAXDATASIZE] = {0};
	S32 length = WrtogsmHeader->msg_len;
#ifdef BIG_END
	my_btol_ltob(&(WrtogsmHeader->msg_len), 4);
#endif
	memcpy(send_request, WrtogsmHeader, length);
	if (carrier_t & GSM_CARRIER_TYPE_MOBILE) {
		send_request[6] = GSM_SIG_ONE; /* 操作载波 1 */
		sendto(send_sockfd, &send_request, length, 0,
			(struct sockaddr *)&send_addr, addrlen);
	}
	if (carrier_t & GSM_CARRIER_TYPE_UNICOM) {
		send_request[6] = GSM_SIG_TWO; /* 操作载波 2 */
		sendto(send_sockfd, &send_request, length, 0,
			(struct sockaddr *)&send_addr, addrlen);
	}
}

/*处理服务器下发的查询/设置请求，然后通过UDP发送给GSM*/
void handle_gsm_req(wrtogsmfreqcfg_t *pStr) 
{
	printf("----------%s----------\n", __FUNCTION__);
	U8 send_request[MAXDATASIZE] = {0};
	
	if(gsm_msg_num < 0xFE)
		gsm_msg_num++;
	else 
		gsm_msg_num = 0x02;
	pStr->WrtogsmHeaderInfo.msg_num = gsm_msg_num;
	S32 length = pStr->WrtogsmHeaderInfo.msg_len;
#ifdef BIG_END
	my_btol_ltob(&(pStr->bcc_type), 2);
	my_btol_ltob(&(pStr->mcc_type), 2);
	my_btol_ltob(&(pStr->mnc_type), 2);
	my_btol_ltob(&(pStr->lac_type), 2);
	my_btol_ltob(&(pStr->lowatt_type), 2);
	my_btol_ltob(&(pStr->upatt_type), 2);
	my_btol_ltob(&(pStr->cnum_type), 2);
	my_btol_ltob(&(pStr->cfgmode_type), 2);
	my_btol_ltob(&(pStr->workmode_type), 2);
	my_btol_ltob(&(pStr->startfreq_1_type), 2);
	my_btol_ltob(&(pStr->startfreq_1), 2);
	my_btol_ltob(&(pStr->endfreq_1_type), 2);
	my_btol_ltob(&(pStr->endfreq_1), 2);
	my_btol_ltob(&(pStr->startfreq_2_type), 2);
	my_btol_ltob(&(pStr->startfreq_2), 2);
	my_btol_ltob(&(pStr->endfreq_2_type), 2);
	my_btol_ltob(&(pStr->endfreq_2), 2);
	my_btol_ltob(&(pStr->freqoffset_type), 2);
	my_btol_ltob(&(pStr->freqoffset), 2);
	my_btol_ltob(&(pStr->WrtogsmHeaderInfo.msg_len), 4);
#endif
	memcpy(send_request, pStr, SEND_TO_GSM_LEN);
	sendto(send_sockfd, &send_request, length, 0,
		(struct sockaddr *)&send_addr, addrlen);
}

/*处理GSM发送来的查询应答消息并处理后上报给服务器*/
void handle_gsm_query_set_ack(char *recv_buf, int msg_len, U8 gsm_carrier_type)
{
	printf("----------%s----------\n", __FUNCTION__);
	S32 msg_num = 0;
	cJSON *object = cJSON_CreateObject();
	if (object == NULL) return;

	while((msg_num + 8) < (msg_len + 8)) {
		gsm_msg_t msg;
		memcpy(&msg, &recv_buf[msg_num], 3);
		msg.gsm_msg = malloc(msg.gsm_msg_len - 3);
		if(msg.gsm_msg == NULL) {
			/* stop memory lost! Add by Adams! */
			cJSON_Delete(object);
			return;
		}
		memcpy(msg.gsm_msg, &recv_buf[msg_num + 3], msg.gsm_msg_len - 3);
#ifdef BIG_END
		my_btol_ltob(&(msg.gsm_msg_type), 2);
#endif
		switch(msg.gsm_msg_type) {
			case O_GSM_CARRIER_FRE:
			{
				// printf("----------->O_GSM_CARRIER_FRE:%s\n", msg.gsm_msg);
				cJSON_AddStringToObject(object, "topic", my_topic);
				cJSON_AddStringToObject(object, "ip", entry->ipaddr);
				cJSON_AddStringToObject(object, "bcc", (S8 *)(msg.gsm_msg));
				break;
			}
			case O_GSM_MCC:
			{
				// printf("----------->O_GSM_MCC:%s\n", msg.gsm_msg);
				cJSON_AddStringToObject(object, "mcc", (S8 *)(msg.gsm_msg));
				break;
			}
			case O_GSM_MNC:
			{
				// printf("----------->O_GSM_MNC:%s\n", msg.gsm_msg);
				cJSON_AddStringToObject(object, "mnc", (S8 *)(msg.gsm_msg));
				break;
			}
			case O_GSM_LAC:
			{
				// printf("----------->O_GSM_LAC:%s\n", msg.gsm_msg);
				cJSON_AddStringToObject(object, "lac", (S8 *)(msg.gsm_msg));
				break;
			}
			case O_GSM_DOWN_ATTEN:
			{
				// printf("----------->O_GSM_DOWN_ATTEN:%s\n", msg.gsm_msg);
				cJSON_AddStringToObject(object, "lowatt", (S8 *)(msg.gsm_msg));
				break;
			}
			case O_GSM_UP_ATTEN:
			{
				// printf("----------->O_GSM_UP_ATTEN:%s\n", msg.gsm_msg);
				cJSON_AddStringToObject(object, "upatt", (S8 *)(msg.gsm_msg));
				break;
			}
			case O_GSM_CI:
			{
				// printf("----------->O_GSM_CI:%s\n", msg.gsm_msg);
				// U8 msg_t = msg.gsm_msg[0];
				// cJSON_AddNumberToObject(object, "cnum", msg_t);
				if(msg.gsm_msg)
					cJSON_AddStringToObject(object, "cnum", (S8 *)(msg.gsm_msg));
				else
					cJSON_AddStringToObject(object, "cnum", "0");
				break;
			}
			case O_GSM_CONFIG_MODE:
			{
				// printf("----------->O_GSM_CONFIG_MODE:%s\n", msg.gsm_msg);
				U8 msg_t = msg.gsm_msg[0];
				cJSON_AddNumberToObject(object, "cfgmode", msg_t);
				break;
			}
			case O_GSM_WORK_MODE:
			{
				// printf("----------->O_GSM_WORK_MODE:%s\n", msg.gsm_msg);
				U8 msg_t = msg.gsm_msg[0];
				cJSON_AddNumberToObject(object, "workmode", msg_t);
				break;
			}
			case O_GSM_START_FRE_ONE:
			{
				// printf("----------->O_GSM_START_FRE_ONE:%s\n", msg.gsm_msg);
				S16 msg_t = msg.gsm_msg[0] | msg.gsm_msg[1]<<8;
				cJSON_AddNumberToObject(object, "startfreq_1", msg_t);
				break;
			}
			case O_GSM_STOP_FRE_ONE:
			{
				// printf("----------->O_GSM_STOP_FRE_ONE:%s\n", msg.gsm_msg);
				S16 msg_t = msg.gsm_msg[0] | msg.gsm_msg[1]<<8;
				cJSON_AddNumberToObject(object, "endfreq_1", msg_t);
				break;
			}
			case O_GSM_START_FRE_TWO:
			{
				// printf("----------->O_GSM_START_FRE_TWO:%s\n", msg.gsm_msg);
				S16 msg_t = msg.gsm_msg[0] | msg.gsm_msg[1]<<8;
				cJSON_AddNumberToObject(object, "startfreq_2", msg_t);
				break;
			}
			case O_GSM_STOP_FRE_TWO:
			{
				// printf("----------->O_GSM_STOP_FRE_TWO:%s\n", msg.gsm_msg);
				S16 msg_t = msg.gsm_msg[0] | msg.gsm_msg[1]<<8;
				cJSON_AddNumberToObject(object, "endfreq_2", msg_t);
				break;
			}
			case O_GSM_FRE_OFFSET:
			{
				// printf("----------->O_GSM_FRE_OFFSET:%s\n", msg.gsm_msg);
				S16 msg_t = msg.gsm_msg[0] | msg.gsm_msg[1]<<8;
				cJSON_AddNumberToObject(object, "freqoffset", msg_t);

				char *send_str = cJSON_PrintUnformatted(object);
				if (!send_str) {
					printf("GSM json to string error!\n");
					break;
				}
				printf("GSM report str:%s\n", cJSON_Print(object));
				// cJSON_Delete(object);
				pthread_mutex_lock(&http_send_lock);
				if(gsm_carrier_type == 0)
					http_send(args_info.gsm_fre_one_api_arg, send_str, NULL, NULL);
				else if(gsm_carrier_type == 1)
					http_send(args_info.gsm_fre_two_api_arg, send_str, NULL, NULL);
				pthread_mutex_unlock(&http_send_lock);
				free(send_str);
				break;
			}
			default:
			{
				printf("GSM response info error!");
				if(msg.gsm_msg)
					free(msg.gsm_msg);
				/* stop memory lost, Add by Adams */
				if (object)
					cJSON_Delete(object);
				return;
			}
		}
		if(msg.gsm_msg)
			free(msg.gsm_msg);
		msg_num =  msg_num + recv_buf[msg_num];
	}
	struct cli_req_if cli_info;
	snprintf((S8 *)(cli_info.ipaddr), 20, "%s", entry->ipaddr);
	cli_info.msgtype = O_SEND_TOGSM_GETCFGONE + 1;
	cli_info.sockfd = -1;
	cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
	if(cli_entry == NULL) {
		cli_info.msgtype = O_SEND_TOGSM_GETCFGTWO + 1;
		cli_entry = entry_to_clireq_list_search(cli_info);
	}
	if (cli_entry) {
		if (entry) {
			cJSON_AddStringToObject(object,
				"online", (entry->online)?"1":"0");
			cJSON_AddStringToObject(object,
				"software_version",
				(entry->s_version)?(entry->s_version):"");
		}
		char *json_str = cJSON_PrintUnformatted(object);
		if (json_str) {
			write(cli_entry->info.sockfd,
				json_str, strlen(json_str) + 1);
			free(json_str);
		} else {
			char cli_resq[8] = "nothing";
			write(cli_entry->info.sockfd,
				cli_resq, sizeof(cli_resq));
		}
		close(cli_entry->info.sockfd);
		del_entry_to_clireq_list(cli_entry);
	} else {
		cli_info.msgtype = O_SEND_TOGSM_SETCFGONE + 1;
		cli_entry = entry_to_clireq_list_search(cli_info);
		if(cli_entry == NULL) {
			cli_info.msgtype = O_SEND_TOGSM_SETCFGTWO + 1;
			cli_entry = entry_to_clireq_list_search(cli_info);
		}
		if(cli_entry) {
			char cli_set_buf[32] = {0};
			snprintf(cli_set_buf, 32, "set arfcn success");
			write(cli_entry->info.sockfd, cli_set_buf, strlen(cli_set_buf) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	}
	if(object)
		cJSON_Delete(object);
}

void handle_from_gsm_ind(char *recv_buf)   //心跳信息处理
{
	U16 ind_type = recv_buf[1] | recv_buf[2]<<8;
	if(ind_type == O_GSM_RUN_STATE) {
		printf("CDMA ind\n");
	} else {
		printf("GSM ind\n");
#if 0		
		U16 com_one_fre = recv_buf[0] | recv_buf[1]<<8;   //小区1的频点
		U16 com_two_fre = recv_buf[2] | recv_buf[3]<<8;   //小区2的频点
#endif
		entry->change_count += 5;
		entry->online = 1;

		char json_str[256] = {0};
		snprintf(json_str, 256,
			"{\"topic\":\"%s\",\"ip\":\"%s\","
			"\"work_admin_state\":\"%d\"}",
			my_topic, entry->ipaddr, entry->cellstate);
		printf("gsm status str:%s\n", json_str);
		pthread_mutex_lock(&http_send_lock);
		http_send(args_info.cellstat_api_arg,
			json_str, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
	}
}

/*
处理GSM发送来的用户信息消息并上报给服务器
GSM信息结构 0x0200,0x0103，0x0211,0x0212，0x0213，0x0215
CDMA信息结构 0x0200,0x0150，0x0110，0x0211,0x0214，0x0215(只有GSM)*/
void handle_from_gsm_msg_rep(char *recv_buf, int msg_len)
{
	printf("----------%s----------\n", __FUNCTION__);
	gsm_msg_t from_gsm_msg;
	S32 buf_len = 0;
	gsm_rep_msg_t gsm_send_msg;
	memset(&gsm_send_msg,'\0', sizeof(gsm_rep_msg_t));

	while(buf_len < msg_len) {
		from_gsm_msg.gsm_msg_len = recv_buf[buf_len];
		from_gsm_msg.gsm_msg = malloc(from_gsm_msg.gsm_msg_len - 3);
		if(from_gsm_msg.gsm_msg == NULL) {
			printf("malloc error!\n");
			return;
		}
		from_gsm_msg.gsm_msg_type = recv_buf[buf_len + 1] | recv_buf[buf_len + 2]<<8;
		memcpy(from_gsm_msg.gsm_msg, &recv_buf[buf_len + 3], from_gsm_msg.gsm_msg_len - 3);

		if(from_gsm_msg.gsm_msg_type == O_GSM_REP_OPE) {
			memcpy(&gsm_send_msg.perator, from_gsm_msg.gsm_msg, 4);
#ifdef BIG_END
			my_btol_ltob(&(gsm_send_msg.perator), 4);
#endif
		}				
		else if(from_gsm_msg.gsm_msg_type == O_GSM_LAC)	{
			memcpy(gsm_send_msg.lac, from_gsm_msg.gsm_msg, 8);
		} else if(from_gsm_msg.gsm_msg_type == O_GSM_REP_IMSI) {
			memcpy(gsm_send_msg.imsi, from_gsm_msg.gsm_msg, 20);
		} else if(from_gsm_msg.gsm_msg_type == O_GSM_REP_IMEI) {
			memcpy(gsm_send_msg.imei, from_gsm_msg.gsm_msg, 20);
		} else if(from_gsm_msg.gsm_msg_type == O_GSM_REP_TMSI) {
			memcpy(gsm_send_msg.tmsi, from_gsm_msg.gsm_msg, 20);
		} else if(from_gsm_msg.gsm_msg_type == O_GSM_SIGNAL_INT) {
			memcpy(&gsm_send_msg.signal, from_gsm_msg.gsm_msg, 2);
#ifdef BIG_END
			my_btol_ltob(&(gsm_send_msg.signal), 2);
#endif
		}
		if(from_gsm_msg.gsm_msg)
			free(from_gsm_msg.gsm_msg);
		buf_len = buf_len + from_gsm_msg.gsm_msg_len;
	}/* end while(buf_len < msg_len) */
	time_t rawtime;
	struct tm *timeinfo;
	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);
	char now_time[20] = {0};
	printf("<----GSM-IMSI %s,\n", gsm_send_msg.imsi);
	printf("<----GSM-IMEI %s,\n", gsm_send_msg.imei);
	snprintf(now_time, 20, "%lu", rawtime);

	cJSON *udp_object = cJSON_CreateObject();
	if (udp_object) {
		cJSON_AddStringToObject(udp_object, "timestamp", now_time);
		cJSON_AddStringToObject(udp_object,
			"imsi", (S8 *)gsm_send_msg.imsi);
		cJSON_AddStringToObject(udp_object, "ip", entry->ipaddr);
		cJSON_AddStringToObject(udp_object,
			"imei", (S8 *)gsm_send_msg.imei);
		cJSON_AddNumberToObject(udp_object,
			"rssi", gsm_send_msg.signal);
		S8 *usend_str = cJSON_PrintUnformatted(udp_object);
		if (usend_str) {
			/* send to udp  */
			send_string_udp(usend_str, strlen(usend_str));
			free(usend_str);
		}
		cJSON_Delete(udp_object);
	}

	cJSON *local_object = cJSON_CreateObject();
	if (!local_object)
		goto end_send_json;
	cJSON_AddNumberToObject(local_object, "perator", gsm_send_msg.perator);
	cJSON_AddStringToObject(local_object, "imsi", (S8 *)gsm_send_msg.imsi);
	cJSON_AddStringToObject(local_object, "imei", (S8 *)gsm_send_msg.imei);
	cJSON_AddNumberToObject(local_object, "signal", gsm_send_msg.signal);
	cJSON_AddStringToObject(local_object, "timestamp", now_time);
	//cJSON_AddStringToObject(local_object, "lac", gsm_send_msg.lac);
	//cJSON_AddStringToObject(local_object, "tmsi", gsm_send_msg.tmsi);

	gsm_cli_object_lock();
	cJSON_AddItemToArray(gsm_info_array, local_object);
	gsm_info_count++;
	if (gsm_info_count >= args_info.max_stacache_arg) {
		char *send_str = cJSON_PrintUnformatted(gsm_info_object);
		if (!send_str) {
			printf("GSM user info report json to string error!\n");
			gsm_cli_object_unlock();
			/* not return ,need insert to db */
			/* change By Adams */
			goto end_send_json;
		}
		printf("GSM user info report str:%s\n", send_str);
#ifdef SCACHE
		if (get_netstate() == NETWORK_OK) {
#endif
			pthread_mutex_lock(&http_send_lock);
			http_send_client(args_info.gsm_cdma_rep_api_arg,
				send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
#ifdef SCACHE
		} else {
			write_string_gsmcachefile(send_str);
		}
#endif
		free(send_str);
		cJSON_DeleteItemFromObject(gsm_info_object, "clients");
		cJSON_Delete(gsm_info_object);
		_gsm_json_object_init(&gsm_info_object);
		gsm_info_count = 0;
	} /* end if(gsm_info_count >= args_info.max_stacache_arg).. */
	gsm_cli_object_unlock();
#ifdef SAVESQL
	_imsi_st s_point;
#endif
end_send_json:
#ifdef SAVESQL
	/* insert to db */
	snprintf(now_time, 20, "%lu", (unsigned long)rawtime);
	snprintf(s_point.this_time, 20, "%04d-%02d-%02d %02d:%02d:%02d",
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
		timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min,
		timeinfo->tm_sec);
	snprintf(s_point.imsi, 16, "%s", gsm_send_msg.imsi);
	snprintf(s_point.imei, 16, "%s", gsm_send_msg.imei);
	strcpy(s_point.dev_ip, entry->ipaddr);
	strcpy(s_point.type, "2G");
	save2sqlite_insert(s_point);
#endif
	return;
}

/*处理扫描信息并上报给服务器*/
void handle_from_gsm_scan_rep(char *recv_buf, int msg_len)   //扫描结果上报信息处理
{
	printf("----------%s----------\n", __FUNCTION__);
	nb_cell_info_item_t cell_info;
	U8 plmn_tmp[6] = {0};
	memcpy(&cell_info, recv_buf, msg_len);
	cJSON *local_object = cJSON_CreateObject();
	if (local_object == NULL) return;

	sprintf((S8 *)plmn_tmp, "%d%d%d%d%d", cell_info.bPLMNId[0], cell_info.bPLMNId[1], cell_info.bPLMNId[2], cell_info.bPLMNId[3], cell_info.bPLMNId[4]);
	//printf("plmn:%s\n", plmn_tmp);
	if(strncmp((S8 *)plmn_tmp, "46000", 5) && strncmp((S8 *)plmn_tmp, "46001", 5)) {
		cJSON_Delete(local_object);
		return;
	}
#ifdef BIG_END
	my_btol_ltob(&cell_info.bGlobalCellId, 4);
	my_btol_ltob(&cell_info.wLAC, 2);
	my_btol_ltob(&cell_info.wBSIC, 2);
	my_btol_ltob(&cell_info.wARFCN, 2);
#endif
	cJSON_AddNumberToObject(local_object, "bglobal_cell_id", cell_info.bGlobalCellId);
	cJSON_AddStringToObject(local_object, "plmn_id", (S8 *)plmn_tmp);
	cJSON_AddNumberToObject(local_object, "rssi", cell_info.cRSSI);
	cJSON_AddNumberToObject(local_object, "lac", cell_info.wLAC);
	cJSON_AddNumberToObject(local_object, "bsic", cell_info.wBSIC);
	cJSON_AddNumberToObject(local_object, "arfcn", cell_info.wARFCN);
	cJSON_AddNumberToObject(local_object, "cc1", cell_info.cC1);
	cJSON_AddNumberToObject(local_object, "nb_cell_num", cell_info.bNbCellNum);
	cJSON_AddNumberToObject(local_object, "bc2", cell_info.bC2);
	cJSON *nbcell_array = cJSON_CreateArray();
	if (!nbcell_array) {
		cJSON_Delete(local_object);
		return;
	}
	cJSON_AddItemToObjectCS(local_object, "nb_cell_array", nbcell_array);

	char scanfile_buf[256] = {0};
	sprintf(scanfile_buf, "[arfcn:%d;plmn_id:%s;bglobal_cell_id:%d;"
		"rssi:%d;lac:%d;bsic:%d;cc1:%d;"
		"nb_cell_num:%d;bc2:%d]\n",
		cell_info.wARFCN, plmn_tmp, cell_info.bGlobalCellId,
		cell_info.cRSSI, cell_info.wLAC, cell_info.wBSIC,
		cell_info.cC1, cell_info.bNbCellNum, cell_info.bC2);
	FILE *gsmscan_fp = fopen(SCAN_GSM_FILE, "a");
	if(gsmscan_fp != NULL)
	{
		fputs(scanfile_buf, gsmscan_fp);
		fclose(gsmscan_fp);
	}
	pthread_mutex_lock(&gsm_scan_js_object_lock);
	cJSON_AddItemToArray(gsm_scan_info_array, local_object);
	gsm_scan_info_count++;
	if (gsm_scan_info_count >= MAX_GSM_SCAN_INFO_COUNT) {
		char *send_str = cJSON_PrintUnformatted(gsm_scan_info_object);
		if (!send_str) {
			printf("scan info report json to string error!\n");
			pthread_mutex_unlock(&gsm_scan_js_object_lock);
			return;
		}
		printf("scan info report str:%s\n", cJSON_Print(gsm_scan_info_object));	
		pthread_mutex_lock(&http_send_lock);
		http_send(args_info.gsm_scan_api_arg, send_str, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
		if (send_str) free(send_str);
		cJSON_Delete(gsm_scan_info_object); // 删除gsm_info_object
		// gsm_scan_json_init(); // 初始化json数组
		_gsm_json_object_init(&gsm_scan_info_object);
		gsm_scan_info_count = 0;
	}
	pthread_mutex_unlock(&gsm_scan_js_object_lock);
}

/*处理GSM查询基本信息的应答消息并上报给服务器*/
void handle_gsm_station(char *recv_buf)
{
	S32 all_leng = 0;
	memcpy(&all_leng, recv_buf, sizeof(S32));
#ifdef BIG_END
	my_btol_ltob(&all_leng, sizeof(S32));
#endif
	S32 offset = GSM_HEADER_LEN;
	U8 msg_len = 0, *msg_value = NULL;
	U16 msg_type = 0;
	entry->change_count++;
	entry->online = 1;
	while (offset < all_leng) {
		msg_len = recv_buf[offset++] - GSM_MSG_HEADER_LEN;
		msg_type = recv_buf[offset++] | (recv_buf[offset++] << 8);
		msg_value = (U8 *)calloc(1, msg_len + 1);
		if (!msg_value) {
			offset += msg_len;
			continue;
		}
		memcpy(msg_value, &recv_buf[offset], msg_len);
		offset += msg_len;
		switch(msg_type) {
		case O_GSM_VERSION: /* 软件版本 */
			{

				printf("version query response\n");
				if (entry->s_version == NULL)
					entry->s_version = strdup((S8 *)msg_value);
				break;
			}
		case O_GSM_WORK_MODE: /* 工作模式 */
			{
				printf("workmode query response\n");
				/* 0与1已被4g设备所占用，2g模式对应2与3 */
				if(msg_value[0] == 0)
					entry->work_mode = 2;
				else if(msg_value[0] == 1)
					entry->work_mode = 3;
				break;
			}
		case O_GSM_RUN_STATE: /* 运行状态 */
			{
#ifdef BIG_END
				my_btol_ltob(msg_value, msg_len);
#endif
				printf("run status response!\n");
				entry->u16RemMode = 0;
				entry->u16SyncState = (msg_value[0] & 1 << 1)?1:2;
				entry->cellstate = (msg_value[0] & (1 << 4))?1:0;
			}
		default:
			break;
		}
		free(msg_value);
	}
}

void handle_recv_gsm(char *recvfrom_buf)
{
	printf("----------%s----------\n", __FUNCTION__);
	if(recvfrom_buf == NULL)
		return;
	U8 msgType, msgNum;
	S32 recvfrom_buf_len = recvfrom_buf[0] | recvfrom_buf[1] << 8 |
		recvfrom_buf[2] << 16 | recvfrom_buf[3] << 24;
	S32 msg_len = recvfrom_buf_len - GSM_HEADER_LEN;
	U8 gsm_carrier_type = recvfrom_buf[6];

	msgNum = recvfrom_buf[4];
	if(msgNum == 0xFF) {
		handle_gsm_station(recvfrom_buf);
		return;
	}
	char *recv_buf = malloc(msg_len);
	if(recv_buf == NULL)
		return;
	msgType = recvfrom_buf[5];
	if(msgType == 0) {
		free(recv_buf);
		return;
	}
	switch(msgType) {
		case O_FROM_GSM_QUERY_ACK:
		{
			memcpy(recv_buf, &recvfrom_buf[GSM_HEADER_LEN], msg_len);
			//printf("查询应答消息体buf\n");
			handle_gsm_query_set_ack(recv_buf, msg_len, gsm_carrier_type);
			break;
		}
		case O_FROM_GSM_SET_ACK:
		{
	
			memcpy(recv_buf, &recvfrom_buf[GSM_HEADER_LEN], msg_len);
			//printf("设置应答消息体buf\n");
			handle_gsm_query_set_ack(recv_buf, msg_len, gsm_carrier_type);
			break;
		}		
		case O_FROM_GSM_MSG_REP:
		{
			memcpy(recv_buf, &recvfrom_buf[GSM_HEADER_LEN], msg_len);
			//printf("用户信息上报消息体buf\n");
			handle_from_gsm_msg_rep(recv_buf, msg_len);
			break;
		}
		case O_FROM_GSM_SCAN_REP:
		{
			
			memcpy(recv_buf, &recvfrom_buf[GSM_HEADER_LEN], msg_len);
			//printf("GSM扫描结果上报消息体buf\n");
			handle_from_gsm_scan_rep(recv_buf, msg_len);
			break;
		}
		case O_FROM_GSM_IND:
		{
			memcpy(recv_buf, &recvfrom_buf[GSM_HEADER_LEN], msg_len);
			//printf("心跳消息体buf\n");
			handle_from_gsm_ind(recv_buf);
			break;
		}

		case O_FROM_GSM_NETSCAN_ACK:
		{
			printf("scan response\n");
			safe_pthread_create(read_andsend_gsm_scan_json_object, NULL); //开启循环检测扫描JSON的线程
			break;
		}
		case O_FROM_GSM_ON_RF_ACK:
		{
			printf("start RF response\n");
			break;
		}
		case O_FROM_GSM_OFF_RF_ACK:
		{
			printf("stop RF response\n");
			int ii = 0;
			for (ii = 0; ii < recvfrom_buf_len; ii++) {
				printf("%02x ", recvfrom_buf[ii]);
			}
			printf("\n");
			break;
		}
		case O_FROM_GSM_RES_SYS_ACK:
		{
			printf("restart system response\n");
			char no_info_str[32] = {0};
			snprintf(no_info_str, 32, "restart success");
			struct cli_req_if cli_info;
			snprintf((S8 *)(cli_info.ipaddr), 20, "%s", GSM_IP_ADDR);
			cli_info.msgtype = O_FL_LMT_TO_ENB_REBOOT_CFG + 1;
			cli_info.sockfd = -1;
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					no_info_str,
					strlen(no_info_str) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
			break;
		}
		case O_FROM_GSM_CARRIER_RES_ACK:
		{
			printf("Carrier reset response\n");
			break;
		}
		default:
		{
			printf("GSM recvfrom info error!");
			break;
		}
	}
	free(recv_buf);
}

void *thread_send_gsm(void *arg)   //循环查询GSM的基础信息(软件版本与工作模式)
{
	printf("----------%s----------\n", __FUNCTION__);
	U8 send_request[64];
	S32 offset = 0;

	send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(send_sockfd < 0) {
		perror("get send_sockfd err!\n");
		exit(-1);
	}
	printf("create send_sockfd success!\n");

	bzero(&send_addr, addrlen);
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(args_info.udp_send_gsm_port_arg);
	send_addr.sin_addr.s_addr = inet_addr(args_info.udp_gsm_ip_arg);

	WrtogsmHeader_t send_head;
	send_head.msg_num = 0xFF;        /* 查询软件版本与工作模式使用0xFF */
	send_head.carrier_type = 0x00; /* 载波指示:0即可 */
	send_head.msg_type = O_SEND_TO_GSM_QUERY; /* 查询为:0x01 */

//查询命令的消息总长度(信息总头8个字节) */
	send_head.msg_len = GSM_HEADER_LEN + /* 消息头长度 8 */
		(GSM_MSG_HEADER_LEN + GSM_VERSION_LEN) + /* 查询版本信息长度 */
		(GSM_MSG_HEADER_LEN + GSM_WORKMODE_LEN)+ /* 查询工作模式长度 */
		(GSM_MSG_HEADER_LEN + GSM_RUN_STATUS_LEN); /* 查询工作状态长度 */
#ifdef BIG_END
	my_btol_ltob(&(send_head.msg_len), 4);
#endif
	memcpy(send_request, &send_head, GSM_HEADER_LEN);
	offset += GSM_HEADER_LEN;

	gsm_msg_t send_gsm_msg;
	/* 组合查询版本信息的信息体 */
	send_gsm_msg.gsm_msg = (U8 *)calloc(1, GSM_VERSION_LEN);
	if(send_gsm_msg.gsm_msg == NULL)
		return NULL;
	memset(send_gsm_msg.gsm_msg, 0, GSM_VERSION_LEN);
	send_gsm_msg.gsm_msg_type = O_GSM_VERSION;		//软件版本
	send_gsm_msg.gsm_msg_len = GSM_MSG_HEADER_LEN + GSM_VERSION_LEN;  //信息单元头3个字节
#ifdef BIG_END
	my_btol_ltob(&(send_gsm_msg.gsm_msg_type), 2);
#endif
	memcpy(&send_request[offset], &send_gsm_msg, GSM_MSG_HEADER_LEN);
	offset += GSM_MSG_HEADER_LEN;
	memcpy(&send_request[offset], send_gsm_msg.gsm_msg, GSM_VERSION_LEN);
	offset += GSM_VERSION_LEN;
	/* 组合查询工作模式的信息体 */
	if(send_gsm_msg.gsm_msg) {
		free(send_gsm_msg.gsm_msg);
		send_gsm_msg.gsm_msg = NULL;
	}
	U8 send_gsm_workmode = 0x00;
	send_gsm_msg.gsm_msg_len = GSM_MSG_HEADER_LEN + GSM_WORKMODE_LEN;
	send_gsm_msg.gsm_msg_type = O_GSM_WORK_MODE; //工作模式
#ifdef BIG_END
	my_btol_ltob(&(send_gsm_msg.gsm_msg_type), 2);
#endif
	memcpy(&send_request[offset], &send_gsm_msg, GSM_MSG_HEADER_LEN);
	offset += GSM_MSG_HEADER_LEN;
	memcpy(&send_request[offset], &send_gsm_workmode, GSM_WORKMODE_LEN);
	offset += GSM_WORKMODE_LEN;

	/* 组合查询设备状态的信息体 */
	if(send_gsm_msg.gsm_msg) {
		free(send_gsm_msg.gsm_msg);
		send_gsm_msg.gsm_msg = NULL;
	}
	send_gsm_msg.gsm_msg_len = GSM_MSG_HEADER_LEN + GSM_RUN_STATUS_LEN; /* 长度 */
	send_gsm_msg.gsm_msg_type = O_GSM_RUN_STATE;   /* 运行状态 */
	send_gsm_msg.gsm_msg = (U8 *)calloc(1, GSM_RUN_STATUS_LEN);
#ifdef BIG_END
	my_btol_ltob(&(send_gsm_msg.gsm_msg_type), 2);
#endif
	memcpy(&send_request[offset], &send_gsm_msg, GSM_MSG_HEADER_LEN);
	offset += GSM_MSG_HEADER_LEN;
	memcpy(&send_request[offset], send_gsm_msg.gsm_msg, GSM_RUN_STATUS_LEN);
	offset += GSM_RUN_STATUS_LEN;
	while(1) {
		sendto(send_sockfd, send_request, offset, 0,
			(struct sockaddr *)&send_addr, addrlen);
		sleep(args_info.bandonline_itv_arg / 2);
	}
	return NULL;
}

void *thread_recv_gsm(void *arg)
{
	handle_recv_gsm((char *)arg);
	return NULL;
}

/*主控板与GSM交互的线程*/
void *thread_udp_to_gsm(void *arg)
{		
	struct sockaddr_in server_addr, client_addr;
	char recvfrom_buf[MAX_RECEV_LENGTH] = {0};
	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp_sockfd < 0) {
		perror("get udp socket err!\n");
		exit(-1);
	}
	bzero(&server_addr, addrlen);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(args_info.udp_recv_gsm_port_arg);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(udp_sockfd, (struct sockaddr *)&server_addr, addrlen) < 0) {
		perror("bind err");
		close(udp_sockfd);
		exit(-1);
	}
	//gsm_info_object_init();
	printf("create udp socket success!\n");

	gsm_cli_object_lock_init();
	pthread_mutex_init(&gsm_scan_js_object_lock, NULL); // 初始化GSM扫描信息json锁
	// gsm_json_init();
	_gsm_json_object_init(&gsm_info_object);
	// gsm_scan_json_init();
	_gsm_json_object_init(&gsm_scan_info_object);
	safe_pthread_create(thread_send_gsm, NULL); //开启线程，循环向GSM发送获取基本信息的请求
	safe_pthread_create(thread_gsm_cap_time, NULL); //开启线程，每隔一段时间，更新gsm的LAC
	while(1)  //循环等待接收GSM通过UDP发送来的消息
	{
		printf("recv GSM device Msg...\n");
		recvfrom(udp_sockfd, recvfrom_buf, MAX_RECEV_LENGTH, 0,
			(struct sockaddr *)&client_addr, &addrlen);
		if(entry == NULL)
			entry = band_list_entry_search_add(inet_ntoa(client_addr.sin_addr));  //根据连接的ip新建链表节点
		safe_pthread_create(thread_recv_gsm, (void *)recvfrom_buf); //开启线程处理GSM发送来的消息
	}
	return NULL;
}

int get_gsm_cap_time()
{
	return cap_time;
}

void set_gsm_cap_time(int cap)
{
	cap_time = cap;
}

void *thread_gsm_cap_time(void *arg)
{
	printf("----------%s----------\n", __FUNCTION__);
	U8 send_request[64];
	cap_time = 10;
	U8 gsm_lac[9];
	memset(gsm_lac, 0, 9);
	int gsm_lac_num = 1;
	// strcpy((S8 *)gsm_lac, "00000001");
	// int i = 7;
	while(1)
	{
		sleep(cap_time * 60);
		WrtogsmHeader_t send_head;
		send_head.msg_num = 0x05;
		send_head.carrier_type = 0x00;
		send_head.msg_type = O_SEND_TO_GSM_SET;
		send_head.msg_action = 0x00;
		send_head.msg_len = GSM_HEADER_LEN + 3 + 8;

		memcpy(send_request, &send_head, GSM_HEADER_LEN);
		send_request[GSM_HEADER_LEN] = 0x0B;
		send_request[GSM_HEADER_LEN + 1] = 0x03;
		send_request[GSM_HEADER_LEN + 2] = 0x01;
		/*for(i = 7; i >= 0; i--)
		{
			if(gsm_lac[i] != '9')
			{
				gsm_lac[i]++;
				break;
			}
			else
			{
				gsm_lac[i] = '0';
				continue;
			}
		}*/
		sprintf((char *)gsm_lac, "%d", gsm_lac_num);
		memcpy(&send_request[GSM_HEADER_LEN + 3], gsm_lac, 8);
		printf("set gsm lac:%d\n", gsm_lac_num);
		if(gsm_lac_num < 9999)
			gsm_lac_num++;
		else
			gsm_lac_num = 1;
		/*int n;
		for(n = 0; n < send_head.msg_len; n++)
		{
			printf("--------send[%d]:%x\n", n, send_request[n]);
		}*/
		if(send_sockfd < 0)
			continue;
		sendto(send_sockfd, send_request, send_head.msg_len, 0, (struct sockaddr *)&send_addr, addrlen);
		send_head.carrier_type = 0x01;
		memcpy(send_request, &send_head, GSM_HEADER_LEN);
		sendto(send_sockfd, send_request, send_head.msg_len, 0, (struct sockaddr *)&send_addr, addrlen);
	}
}

/* 每隔args_info.sendsta_itv_arg 时间，检查用户信息json数组中有没有用户信息,
 * 如果有,加锁并将json转换成字符串并使用http协议上至服务器, 
 * 然后删除json中的所有信息,重新初始化json数组
 * */
void *read_send_gsm_json_object(void *arg)
{
	char *json_str = NULL;
	while (1) {
		sleep(args_info.sendsta_itv_arg);
		gsm_cli_object_lock();
		if (!gsm_info_count) {
			gsm_cli_object_unlock();
			continue;
		}
		json_str = cJSON_PrintUnformatted(gsm_info_object);
		if (!json_str) {
			gsm_cli_object_unlock();
			continue;
		}
		printf("Timeout，send user info str:%s\n", json_str);
#ifdef SCACHE
		if(get_netstate() == NETWORK_OK) {
#endif
			pthread_mutex_lock(&http_send_lock);
			http_send_client(args_info.gsm_cdma_rep_api_arg,
				json_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
#ifdef SCACHE
		} else {
			write_string_gsmcachefile(json_str);
		}
#endif
		free(json_str);
		cJSON_Delete(gsm_info_object); // 删除object
		_gsm_json_object_init(&gsm_info_object);
		gsm_info_count = 0;
		gsm_cli_object_unlock();
	}
	return NULL;
}

/* 每隔MAX_GSM_SCAN_INFO_SLEEP_TIME时间，检查扫描信息json数组中有没有用户信息,
 * 如果有,加锁并将json转换成字符串并使用http协议上至服务器, 
 * 然后删除json中的所有信息,重新初始化json数组
 * 循环检查20遍后结束线程，收到扫描应答后重新开启线程
 * */
void *read_andsend_gsm_scan_json_object(void *arg)
{
	char *json_str = NULL;
	int i;
	for(i = 0; i< 20; i++) { //开启扫描后开启此线程，循环检测20次后结束线程
		sleep(MAX_GSM_SCAN_INFO_SLEEP_TIME);
		if (!gsm_scan_info_count)
			continue;
		if (!gsm_scan_info_object)
			continue;
		pthread_mutex_lock(&gsm_scan_js_object_lock);
		json_str = cJSON_PrintUnformatted(gsm_scan_info_object);
		printf("Timeout，send scan GSM info str:%s\n",
			cJSON_Print(gsm_scan_info_object));
		if (json_str) {
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.gsm_scan_api_arg, json_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(json_str);
			cJSON_Delete(gsm_scan_info_object); // 删除object
			_gsm_json_object_init(&gsm_scan_info_object);
			gsm_scan_info_count = 0;
		}
		pthread_mutex_unlock(&gsm_scan_js_object_lock);
	}
	return NULL;
}
