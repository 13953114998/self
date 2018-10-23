/*
 * ============================================================================
 *
 *       Filename:  fiberhome_lte.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年02月27日 14时15分24秒
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
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "arg.h"
#include "http.h"
#include "fiberhome_lte.h"
#ifdef SCACHE
#include "cache_file.h"
#endif
#ifdef SAVESQL
#include "save2sqlite.h"
#endif

cJSON *fiber_sta_object = NULL;
cJSON *fiber_sta_array = NULL;
S32 fiber_sta_count = 0;
pthread_mutex_t fiber_sta_lock;

void fiber_sta_lock_init()
{
	pthread_mutex_init(&fiber_sta_lock, NULL);
}
void fiber_sta_json_lock()
{
	pthread_mutex_lock(&fiber_sta_lock);
}
void fiber_sta_json_unlock()
{
	pthread_mutex_unlock(&fiber_sta_lock);
}
cJSON *get_fiber_sta_object()
{
	return fiber_sta_object;
}
S32 get_fiber_sta_count()
{
	return fiber_sta_count;
}
void fiber_sta_count_sum(S32 num)
{
	fiber_sta_count += num;
}

/* 初始化保存baicell 基带板上传的IMSI信息的JSON数组 */
void fiber_sta_json_init()
{
	fiber_sta_object = cJSON_CreateObject();
	if (fiber_sta_object == NULL) exit(0);
	cJSON_AddStringToObject(fiber_sta_object, "topic", my_topic);
	fiber_sta_array = cJSON_CreateArray();
	if (fiber_sta_array == NULL) {
		cJSON_Delete(fiber_sta_object);
		exit(0);
	}
	cJSON_AddItemToObjectCS(fiber_sta_object,"clients", fiber_sta_array);
	fiber_sta_count = 0;
	printf("Station json object Inited!\n");
}
/* 清除保存baicell基带板上传的IMSI信息的JSON数组 */
void fiber_sta_json_delete()
{
	cJSON_Delete(fiber_sta_object);
}

/* 建立 TCPserver 与下端的烽火baseband进行socket网络交互 */
void *thread_tcp_to_fiberbaseband_lte(void *arg)
{
	/* 初始化保存IMSI信息的json */
	fiber_sta_json_init();
	int server_sockfd, client_sockfd;
	struct sockaddr_in server_addr, client_addr;
	server_sockfd = server_socket_init(args_info.fiber_port_arg);
	if (server_sockfd == -1) {
		printf("get tcp socket (fiberhome)error!\n");
		exit(-1);
	}
	printf("Creat Fiber LTE socket SUCCESS!\n");
	int ret = listen(server_sockfd, MAX_HLIST_LENGTH);
	if (ret == -1) {
		printf("listen failed %s(%d)\n", strerror(errno), errno);
		close(server_sockfd);
		exit(-1);
	}
	printf("Listen Fiber LTE Socket...\n");
	socklen_t addrlen = sizeof(struct sockaddr_in);
	while(1) {
		client_sockfd = accept(server_sockfd,
			(struct sockaddr *)&client_addr,
			(socklen_t *)&addrlen);
		if(client_sockfd == -1) {
			sleep(1);
			continue;
		}
		printf("Accept:Fiber LTE fd: %d, ipaddr: %s\n",
			client_sockfd, inet_ntoa(client_addr.sin_addr));
		ret = Epoll_add(client_sockfd, inet_ntoa(client_addr.sin_addr));

		/* 通过ip在链表中进行查找，未找到则新建节点 */
		band_entry_t *entry =
			band_list_entry_search_add(inet_ntoa(client_addr.sin_addr));
		if (entry->sockfd != client_sockfd) {
			if (entry->sockfd != -1) {
				struct cliinfo cli;
				cli.fd = entry->sockfd;
				cli.addr =
					strdup(inet_ntoa(client_addr.sin_addr));
				Epoll_del(&cli);
				free(cli.addr);
			}
			pthread_mutex_lock(&(entry->lock));
			entry->sockfd = client_sockfd;
			entry->work_mode = FIBERHOME_BASE_BAND_WMODE;/* 5 */
			pthread_mutex_unlock(&(entry->lock));
		}
	} /* end while(1) */
	return NULL;
}

/* ====================================================
 * 获取单基带板的基础信息并组合成json数据发送
 * band_entry_t *entry -- 包含基带板信息的结构体指针
 * ====================================================
 * */
void get_fiber_lte_base_band_status(band_entry_t *entry)
{
	if (!entry || !(entry->macaddr)) return;
	cJSON *object = cJSON_CreateObject();
	if (!object)
		return;
	cJSON_AddStringToObject(object, "topic", my_topic);
	cJSON *array = cJSON_CreateArray();
	if (!array) {
		cJSON_Delete(object);
		return;
	}
	S8 *json_str = NULL;
	cJSON_AddItemToObjectCS(object, "stations", array);
	S32 need_send = 0;
	if (entry) {
		cJSON *object_in_arry = cJSON_CreateObject();
		if (!object_in_arry) {
			printf("creat json object in arry ERROR!\n");
			return;
		}
		pthread_mutex_lock(&(entry->lock));
		cJSON_AddStringToObject(object_in_arry,
			"online", (entry->online)?"1":"0");
		cJSON_AddStringToObject(object_in_arry,
			"ip", (entry->ipaddr)?entry->ipaddr:"");
		cJSON_AddStringToObject(object_in_arry,
			"mac", (entry->macaddr)?(entry->macaddr):"");
		cJSON_AddNumberToObject(object_in_arry, "pa_num", 0);
		cJSON_AddStringToObject(object_in_arry,
			"sn", (entry->sn)?(entry->sn):"");
		cJSON_AddStringToObject(object_in_arry, "oper",
			(entry->oper == 0)?"China Mobile":
			((entry->oper == 1)?"China Unicom":"China Net"));
		cJSON_AddNumberToObject(object_in_arry,
			"work_mode", (U32)entry->lte_mode);
		cJSON_AddStringToObject(object_in_arry,
			"software_version",
			(entry->s_version)?(entry->s_version):"");
		cJSON_AddStringToObject(object_in_arry,
			"hardware_version", "N/A");
		cJSON_AddStringToObject(object_in_arry,
			"device_model", "FiberHome");
		cJSON_AddNumberToObject(object_in_arry,
			"power_derease", (U32)entry->powerderease);
		pthread_mutex_unlock(&(entry->lock));
		cJSON_AddStringToObject(object_in_arry,
			"cell_state", (entry->cellstate == 1)?"1":"2");
		cJSON_AddItemToArray(array, object_in_arry);
		need_send = 1;
	}
	json_str = cJSON_PrintUnformatted(object);
	cJSON_Delete(object);
	if (!json_str) {
		printf("get json string error!\n");
		return;
	}
	printf("send entry information:%s\n", json_str);
	if (need_send) {
		pthread_mutex_lock(&http_send_lock);                       
		http_send(args_info.baseband_api_arg, json_str, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
	} else
		printf("well ,needn't send anything(fiberhome LTE)!\n");
	free(json_str);
}
/* ====================================================
 * 获取基带板的基础信息并组合成json
 * 遍历所有节点找到fiberhome基带板组合信息
 * ====================================================
 * */
S8 *get_all_fiber_lte_base_band_status()
{
	if (!band_entry_count)
		return NULL;
	cJSON *object = cJSON_CreateObject();
	if (!object)
		return NULL;
	cJSON_AddStringToObject(object, "topic", my_topic);
	cJSON *array = cJSON_CreateArray();
	if (!array) {
		cJSON_Delete(object);
		return NULL;
	}
	S8 *json_str = NULL;
	cJSON_AddItemToObjectCS(object, "stations", array);
	S32 need_send = 0;
	band_entry_t *tpos, *n;
	pthread_mutex_lock(&band_list_lock);
	list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
		if (tpos->work_mode != FIBERHOME_BASE_BAND_WMODE) {
			/* it is not fiberhome LTE device */
			continue;
		}
		cJSON *object_in_arry = cJSON_CreateObject();
		if (!object_in_arry) {
			printf("creat json object in arry ERROR!\n");
			continue;
		}
		pthread_mutex_lock(&(tpos->lock));
		cJSON_AddStringToObject(object_in_arry,
			"online", (tpos->online)?"1":"0");
		cJSON_AddStringToObject(object_in_arry,
			"ip", (tpos->ipaddr)?tpos->ipaddr:"");
		cJSON_AddStringToObject(object_in_arry,
			"mac", (tpos->macaddr)?(tpos->macaddr):"");
		cJSON_AddNumberToObject(object_in_arry,
			"pa_num", (int)tpos->pa_num);
		cJSON_AddStringToObject(object_in_arry,
			"sn", (tpos->sn)?(tpos->sn):"");
		cJSON_AddNumberToObject(object_in_arry,
			"work_mode", (U32)tpos->lte_mode);
		cJSON_AddStringToObject(object_in_arry,
			"software_version",
			(tpos->s_version)?(tpos->s_version):"");
		cJSON_AddStringToObject(object_in_arry,
			"hardware_version", "N/A");
		cJSON_AddStringToObject(object_in_arry,
			"device_model", "N/A");
		cJSON_AddNumberToObject(object_in_arry,
			"power_derease", (U32)tpos->powerderease);
		cJSON_AddStringToObject(object_in_arry,
			"cell_state", (tpos->cellstate == 1)?"1":"2");
		pthread_mutex_unlock(&(tpos->lock));
		cJSON_AddItemToArray(array, object_in_arry);
		need_send = 1;
	}
	pthread_mutex_unlock(&band_list_lock);
	json_str = cJSON_PrintUnformatted(object);
	cJSON_Delete(object);
	if (!json_str) {
		printf("get json string error!\n");
		return NULL;
	}
	if (need_send) {
		return json_str;
	}
	printf("well ,needn't send anything(fiberhome LTE)!\n");
	free(json_str);
	return NULL;
}

/* ====================================================
 * 烽火基站数据generic信息解析
 * band_entry_t *entry  -- 保存有基带板信息数据的结构体指针
 * U16 msg_type         -- unsigned short int类型的数据类型
 * U8 *buffer           -- 从client端(基带板)接收到的原始数据
 * ====================================================
 * */
void generic_response_handle(band_entry_t *entry, U16 msg_type, U8 *buffer)
{
	_fiber_genneric_set_res pStr;
	memcpy(&pStr, buffer, sizeof(pStr));
	S8 id_tmp[22] = {0};
	S8 result[4] = {0};
	S8 cause[4] = {0};
	S8 msgtype[6] = {0};
	memcpy(id_tmp, pStr.device_id, 21);
	snprintf(msgtype, 4, "%u", msg_type);
	snprintf(result, 4, "%u", ntohs(pStr.result));
	snprintf(cause, 6, "%u", ntohs(pStr.cause));
	printf("设备id:%s\n", id_tmp);
	printf("返回值:%s\n", result);
	printf("错误码:%s\n", cause);
	cJSON *object  = cJSON_CreateObject();
	if (!object) return;
	cJSON_AddStringToObject(object, "result", result);
	cJSON_AddStringToObject(object, "cause", cause);
	cJSON_AddStringToObject(object, "ip", entry->ipaddr);
	cJSON_AddStringToObject(object, "msgtype", msgtype);
	S8 *json_str = cJSON_PrintUnformatted(object);
	cJSON_Delete(object);
	if (json_str) {
		printf("generic config response:%s\n", json_str);
		pthread_mutex_lock(&http_send_lock);
		http_send(NULL, json_str, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
		free(json_str);
	}
}

/* ====================================================================== 
 * 烽火基站数据信息解析
 * U8 *recBuf  -- 读取的字符串信息 
 * band_entry_t *entry  -- 保存有基带板信息数据的结构体指针
 * ======================================================================
 * */
void handle_recev_fiber_package(U8 *recBuf, band_entry_t *entry)
{
	if (!recBuf || !entry) return;
	_fiberhome_header p_header;
	U16 msg_type;
	memcpy(&msg_type, &recBuf[4], 2);
	S8 *send_api = NULL;
	cJSON *object = cJSON_CreateObject();
	if (!object) return;
	S8 char_m_type[8] = {0};
	snprintf(char_m_type, 8, "%u", (U32)ntohs(msg_type));
	cJSON_AddStringToObject(object, "topic", my_topic);
	cJSON_AddStringToObject(object, "ip", entry->ipaddr);
	cJSON_AddStringToObject(object, "msgtype", char_m_type);
	switch (ntohs(msg_type)) {
	case FROMBASE_DEVICE_STATUS_REPORT:
		{//(303)3.7.7设备运行状态上报(OK) 10s/次
			printf("####FiberHome-------device status report!\n");
			_fiber_dev_status_report pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			U8 send_s = 0;
			memcpy(id_tmp, pStr.rep.device_id, 21);
			printf("  dev_id:%s\n", id_tmp);
			printf("  status:%u\n", pStr.status);
			printf("cpu_rate:%u\n", pStr.cpu_rate);
			printf("mem_rate:%u\n", pStr.mem_rate);
			if (entry->cellstate != pStr.status)
				send_s = 1;
			else
				send_s = 0;
			entry->cellstate = pStr.status; /* 1-小区激活 */
			entry->change_count++;
			entry->_auto++;
			/* 此信息为实时上报当前的设备CPU内存的负载状态
			 * 不做存储,直接上传至服务器,由于比较频繁,数量繁多,
			 * 可能造成服务器负载过大,可约定多少次后上传
			 * */
			if (send_s)
				get_fiber_lte_base_band_status(entry);
			/* 发送心跳检测消息请求 10s/次 */
			send_generic_request_to_fiberhome(FROMBASE_HARTBEAT_REPORT, entry);
			/* 如果20s基带板还是status==2,则下发一次开启采集请求 */
			S8 _type[8] = {0};
			if (pStr.status!= 1 && !(entry->_auto %2)) {
				/* set txpower 4 */
				cJSON *object = cJSON_CreateObject();
				snprintf(_type, 8, "%u", TOBASE_TXPOWER_CONFIG_REQUEST);
				if (object) {
					cJSON_AddStringToObject(object, "ip", entry->ipaddr);
					cJSON_AddStringToObject(object, "msgtype", _type);
					cJSON_AddStringToObject(object, "txpower", "4");
					S8 *j_str = cJSON_PrintUnformatted(object);
					cJSON_Delete(object);
					if (j_str) {
						fiberhome_pare_config_json_message(j_str, entry);
					}
				}
				/* switch on probe */
				object = cJSON_CreateObject();
				snprintf(_type, 8, "%u", TOBASE_PROBE_SWITCH_CONFIG_REQUEST);
				if (object) {
					cJSON_AddStringToObject(object, "ip", entry->ipaddr);
					cJSON_AddStringToObject(object, "msgtype", _type);
					cJSON_AddStringToObject(object, "cmd_type", "1");
					S8 *j_str = cJSON_PrintUnformatted(object);
					cJSON_Delete(object);
					if (j_str) {
						fiberhome_pare_config_json_message(j_str, entry);
					}
				}
			}
			break;
		}
	case FROMBASE_DEVICE_STATUS_REPORT_ADDED:
		{//(409)3.7.8 状态上报扩展部分(OK)
			printf("####FiberHome-------"
				"device status added report!\n");
			_fiber_dev_status_add_rep pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.rep.device_id, 21);
			printf("dev ststus added 设备ID:%s\n", id_tmp);
			printf("dev ststus added balltle:%u\n",
				ntohs(pStr.battle));
			printf("dev ststus added sync:%u\n", (U32)pStr._sync);
			pthread_mutex_lock(&(entry->lock));
			if ((U32)pStr._sync == 0) {
				entry->u16SyncState = 2;
			} else if((U32)pStr._sync == 2) {
				entry->u16SyncState = 0;
			} else {
				entry->u16SyncState = 1;
			}
			pthread_mutex_unlock(&(entry->lock));
			break;
		}
	case FROMBASE_IMSI_REPORT:// (304)IMSI信息上报
		{
			printf("####FiberHome-------IMSI information report!\n");
			_fiber_imsi_rep pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			U32 i;
			S8 imsi_tmp[20] = {0};
			time_t rawtime;
			S8 usend_str[128] = {0};
			struct tm * timeinfo;
			rawtime = time(NULL);
			timeinfo = localtime(&rawtime);
			char now_time[20] = {0};
			snprintf(now_time, 20, "%lu", (unsigned long)rawtime);
			for (i = 0; i < ntohl(pStr.num); i++) {
				cJSON *object = cJSON_CreateObject();
				if (!object) continue;
				snprintf(imsi_tmp, 20, "%llx",
					my_htonll_ntohll(pStr.imsi[i]));
				printf("IMSI:%s\n", imsi_tmp);
				cJSON_AddStringToObject(object,
					"timestamp", now_time);
				cJSON_AddStringToObject(object,
					"imsi", imsi_tmp);
				cJSON_AddStringToObject(object,
					"ip", entry->ipaddr);
				cJSON_AddNumberToObject(object, "rssi", 0);
				cJSON_AddStringToObject(object, "imei", " ");
				fiber_sta_json_lock();
				cJSON_AddItemToArray(fiber_sta_array, object);
				fiber_sta_json_unlock();
				fiber_sta_count++;
				/* Add user json and send user information */
				if (fiber_sta_count >=
					args_info.max_stacache_arg) {
					char *send_str =
						cJSON_PrintUnformatted(fiber_sta_object);
					if (!send_str) {
						fiber_sta_json_unlock();
						break;
					}
#ifdef SCACHE
					if (get_netstate() == NETWORK_OK) {
#endif
						pthread_mutex_lock(&http_send_lock);
						http_send(args_info.station_api_arg,
							send_str, NULL, NULL);
						pthread_mutex_unlock(&http_send_lock);
#ifdef SCACHE
					} else {
						write_string_cachefile(send_str);
					}
#endif
					free(send_str);
					fiber_sta_json_delete();
					fiber_sta_json_init();
				}
				fiber_sta_json_unlock();
#ifdef SAVESQL
				/* insert to sqlite */
				_imsi_st s_point;
				memset(&s_point, 0, sizeof(s_point));
				strcpy(s_point.type, "Fiber4G");
				snprintf(s_point.this_time, 20,
					"%04d-%02d-%02d %02d:%02d:%02d",
					timeinfo->tm_year + 1900,
					timeinfo->tm_mon + 1, timeinfo->tm_mday,
					timeinfo->tm_hour, timeinfo->tm_min,
					timeinfo->tm_sec);
				strcpy(s_point.dev_ip, entry->ipaddr);
				strcpy(s_point.imsi, imsi_tmp);
				save2sqlite_insert(s_point);
#endif /* SAVESQL */
				/* send to udp  */
				snprintf(usend_str, 128,
					"BaseIP:%-16s"
					"  Time:%04d-%02d-%02d %02d:%02d:%02d"
					"  IMSI:-->%-16s",
					entry->ipaddr, timeinfo->tm_year + 1900,
					timeinfo->tm_mon + 1,
					timeinfo->tm_mday, timeinfo->tm_hour,
					timeinfo->tm_min, timeinfo->tm_sec,
					imsi_tmp);
				send_string_udp(usend_str, 128);
			} /* end for */
			goto go_return;
		}
	case FROMBASE_GET_PROBE_CONDIF_RESPONSE://(308) 获取采集信息回应(OK)
		{
			printf("####FiberHome-------"
				"get probe config response!\n");
			_fiber_getprobe_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			if (ntohs(pStr.res.result)) {
				if (ntohs(pStr.res.cause) == 101) {
					printf("get probe config failed:"
						"parameter error!\n");
				} else if (ntohs(pStr.res.cause) == 102) {
					printf("get probe config failed:"
						"device ID error!\n");
				} else if (ntohs(pStr.res.cause) == 103) {
					printf("get probe config failed:"
						"device is not ready!\n");
				} else if (ntohs(pStr.res.cause) == 106) {
					printf("get probe config failed:"
						"timeout!\n");
				}
				break;
			}
			printf("get probe config success!\n");
			printf("  命令类型:%u\n", pStr.cmd_type);
			printf("    运营商:%u\n", ntohs(pStr.oper));
			printf("    小区ID:%u\n", ntohl(pStr.preten_cellid));
			printf("  下行频点:%u\n", ntohs(pStr.workfcn));
			printf("      PLMN:%u\n", ntohl(pStr.plmn));
			printf("    小区ID:%u\n", ntohl(pStr.pci));
			printf("  工作频带:%u\n", ntohl(pStr.band));
			printf("区域跟踪码:%u\n", ntohl(pStr.tac));
			printf("  发射功率:%u\n", ntohl(pStr.powerrank));
			printf("  采集周期:%u\n", ntohl(pStr.collectperiod));
			pthread_mutex_lock(&(entry->lock));
			entry->oper = ntohs(pStr.oper);
			entry->CellId = ntohl(pStr.preten_cellid);
			entry->sysDlARFCN = ntohs(pStr.workfcn);
			snprintf((S8 *)entry->PLMN, 7, "%u", ntohl(pStr.plmn));
			entry->PCI = ntohl(pStr.pci);
			entry->sysBand = ntohl(pStr.band);
			entry->TAC = ntohl(pStr.tac);
			/*
			int a2 = 0xff;
			int a1 = a2 - (int)ntohl(pStr.powerrank);
			char buf[4] = {0};
			itoa(a1, buf, 10);
			printf("\n回传的发射功率参数：\na1: %d\nbuf: %s\n", a1, buf);
			sscanf(buf, "%u", &(entry->powerderease));
			*/
			entry->powerderease =255 -(int) ntohl(pStr.powerrank);
			entry->collectperiod= ntohl(pStr.collectperiod);
			pthread_mutex_unlock(&(entry->lock));

			pthread_mutex_lock(&(entry->lock));
			cJSON_AddNumberToObject(object,
				"sysdiarfcn", (int)entry->sysDlARFCN);
			cJSON_AddStringToObject(object,
				"plmn",	(char *)entry->PLMN);
			cJSON_AddNumberToObject(object,
				"sysband", (int)entry->sysBand);
			cJSON_AddNumberToObject(object,
				"pci", (int)entry->PCI);
			cJSON_AddNumberToObject(object,
				"tac", (int)entry->TAC);
			cJSON_AddNumberToObject(object,
				"cellid", (int)entry->CellId);
			cJSON_AddNumberToObject(object,
				"oper", (int)entry->oper);
			cJSON_AddNumberToObject(object,
				"powerderease",	(int)entry->powerderease);
			cJSON_AddNumberToObject(object,
				"collectperiod", ntohl(pStr.collectperiod));
			pthread_mutex_unlock(&(entry->lock));
			//send_api = strdup();
			break;
		}
	case FROMBASE_GET_DEVICE_INFO_RESPONSE://(316)设备信息查询回应
		{
			printf("####FiberHome-------"
				"get device query response!\n");
			_fiber_dev_query_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("      设备ID:%s\n", id_tmp);
			printf("  设备版本号:%s\n", pStr.version);
			printf("设备LTE 模式:%u\n", pStr.lte_mode);
			printf("设备工作频带:%s\n", pStr.bands);
			printf("    设备状态:%u\n", pStr.status);
			printf("     CPU负载:%u\n", pStr.cpu_rate);
			printf("    内存占用:%u\n", pStr.mem_rate);
			printf("     CPU温度:%d\n", ntohl(pStr.cpu_temp));
			pthread_mutex_lock(&(entry->lock));
			entry->lte_mode = (pStr.lte_mode == 1)?TDD:FDD;
			entry->sn = strdup(id_tmp);
			entry->s_version = strdup(pStr.version);
			entry->fiber_bands = strdup(pStr.bands);
			entry->cellstate = (U32)pStr.status;
			pthread_mutex_unlock(&(entry->lock));
			/* 发送单个基带板信息 */
			get_fiber_lte_base_band_status(entry);
			goto go_return;
			break;
		}
	case FROMBASE_VERSION_UPDATE_RATE_REPORT://(320)基站升级进度指示
		{
			printf("####FiberHome-------"
				"device update status report!\n");
			_fiber_update_rate_rep pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.rep.device_id, 21);
			printf("rate:%u\n", pStr.rate);
			break;
		}
	case FROMBASE_GET_DEVICE_SITE_RESPONSE://(322)基站位置查询回应
		{
			printf("####FiberHome-------"
				"get device site response!\n");
			_fiber_get_site_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("get site 设备ID:%s\n", id_tmp);
			printf("get site result:%u\n", ntohs(pStr.res.result));
			printf("get site  cause:%u\n", ntohs(pStr.res.cause));
			pStr.longitude[16] = 0;
			pStr.latitude[16] = 0;
			pStr.height[16] = 0;
			printf("site longitude:%s\n", (pStr.longitude));
			printf("site latitude:%s\n", (pStr.latitude));
			printf("site height:%s\n", (pStr.height));
			break;
		}
	case FROMBASE_GET_IMSI_INFO_RESPONSE:
		{/* (324)3.7.24.2IMSI数据上传响应 */
			printf("####FiberHome-------"
				"Get IMSI information response!\n");
			_fiber_imsi_ftp_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			printf("Get IMSI 返回码: %u\n", ntohs(pStr.res.result));
			printf("Get IMSI 错误码: %u\n", ntohs(pStr.res.cause));
			if (ntohs(pStr.res.result) == 0)
				printf("Get IMSI: %s", pStr.filename);
			break;
		}
	case FROMBASE_DEVICE_INFOMATION_REPORT:
		{// (326)3.7.25 设备信息上报 60s/次
			printf("####FiberHome-------"
				"Device information report!\n");
			_fiber_dev_info_report pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.rep.device_id, 21);
			printf("      设备ID:%s\n", id_tmp);
			printf("  设备版本号:%s\n", pStr.version);
			printf("设备LTE 模式:%u\n", pStr.lte_mode);
			printf("设备工作频带:%s\n", pStr.bands);
			printf("    设备状态:%u\n", pStr.status);
			printf("     CPU负载:%u\n", pStr.cpu_rate);
			printf("    内存占用:%u\n", pStr.mem_rate);
			printf("     CPU温度:%d\n", ntohl(pStr.cpu_temp));
			printf("    内存温度:%d\n", ntohl(pStr.pa_temp));
			pthread_mutex_lock(&(entry->lock));
			if (!strncmp(id_tmp, "10", 2) ||
				!strncmp(id_tmp, "20", 2)) {
				entry->oper = 0; /* C Mobile */
			} else if (!strncmp(id_tmp, "21", 2)) {
				entry->oper = 1; /* C Unicom */
			} else
				entry->oper = 2; /* C Net */

			S8 mac_tmp1[20] = {0};
			S8 mac_tmp2[20] = {0};
			memcpy(mac_tmp1, &(id_tmp[21-12]), 21-(21-12));
			U32 i, j;
			for (i = 0, j = i; i < 21-(21-12); i++, j++ ) {
				if (mac_tmp1[i] <= 'f' && mac_tmp1[i] >= 'a')
					mac_tmp1[i] = mac_tmp1[i] - 32; 

				mac_tmp2[j] = mac_tmp1[i];
				if (i % 2  == 1) {
					mac_tmp2[++j] = ':';
				}
			}
			mac_tmp2[17] = 0;
			if (!(entry->macaddr)){
				entry->macaddr = strdup(mac_tmp2);
			}
			entry->lte_mode = (pStr.lte_mode == 1)?TDD:FDD;
			entry->sn = strdup(id_tmp);
			entry->s_version = strdup(pStr.version);
			entry->fiber_bands = strdup(pStr.bands);
			entry->cellstate = (U32)pStr.status;
			pthread_mutex_unlock(&(entry->lock));
			/* 发送单个基带板信息 */
			get_fiber_lte_base_band_status(entry);
			goto go_return;
			break;
		}
	case FROMBASE_GET_TIME_RESPONSE://(329)基站时间查询回应
		{
			printf("####FiberHome-------"
				"Device time get response!\n");
			_fiber_get_time_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("get dev time 设备ID:%s\n", id_tmp);
			printf("get dev time result:%u\n",
				ntohs(pStr.res.result));
			printf("get dev time  cause:%u\n",
				ntohs(pStr.res.cause));
			pStr._date[16] = 0;
			pStr._time[16] = 0;
			printf("get dev time date:%s\n", pStr._date);
			printf("get dev time time:%s\n", pStr._time);
			cJSON_AddStringToObject(object, "date", pStr._date);
			cJSON_AddStringToObject(object, "time", pStr._time);
			break;
		}
	case FROMBASE_GET_IP_RESPONSE:// (331)基站IP查询回应OK
		{
			printf("####FiberHome-----Device IP get response!\n");
			_fiber_ipaddr_query_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("get dev time 设备ID:%s\n", id_tmp);
			printf("get dev time result:%u\n",
				ntohs(pStr.res.result));
			printf("get dev time  cause:%u\n",
				ntohs(pStr.res.cause));
			printf("get dev time ip_ver:%u\n", pStr.ip_ver);
			printf("get dev time ipaddr:%u.%u.%u.%u\n",
				pStr.ipaddr[0],	pStr.ipaddr[1],
				pStr.ipaddr[2],	pStr.ipaddr[3]);
			printf("get dev time netmask:%u\n", pStr.net_mask);
			printf("get dev time gateway:%u.%u.%u.%u\n",
				pStr.gateway[0], pStr.gateway[1],
				pStr.gateway[2], pStr.gateway[3]);
			printf("get dev time mbapk_ip:%u.%u.%u.%u\n",
				pStr.mbapk_ipaddr[0], pStr.mbapk_ipaddr[1],
				pStr.mbapk_ipaddr[2], pStr.mbapk_ipaddr[3]);
			printf("get devtime mbapk_port:%u\n", pStr.mbapk_port);
			printf("get devtime  ftp_ser_port:%u\n",
				pStr.ftp_ser_port);
			break;
		}		
	case FROMBASE_WARING_REPORT:// (0x0809)/* 告警上报消息 */
		{
			printf("####FiberHome-------Waning report!\n");
			_fiber_warning_report pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.rep.device_id, 21);
			printf("Warning report 设备ID:%s\n", id_tmp);
			printf("Warning report alarm_id:%u\n",
				ntohl(pStr.alarm_id));
			printf("Warning report alarm_restore_flag:%u\n",
				ntohl(pStr.alarm_restore_flag));
			printf("Warning report param:%s\n", pStr.param);
			cJSON_AddNumberToObject(object,"ind_flag",
				(ntohl(pStr.alarm_restore_flag))? 0 : 1);
			cJSON_AddNumberToObject(object,"alarming_type",
				ntohl(pStr.alarm_id));
			cJSON_AddStringToObject(object, "param", pStr.param);
			break;
		}
	case FROMBASE_CELL_ACTIVE_MODE_QUERY_RESPONSE://(404)运行模式查询回应
		{/* OK */
			printf("####FiberHome-------"
				"Cell active mode query response!\n");
			_fiber_cell_act_query_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("Active mode query 设备ID:%s\n",id_tmp);
			printf("Active mode query result:%u\n",
				ntohs(pStr.res.result));
			printf("Active mode query cause:%u\n",
				ntohs(pStr.res.cause));
			printf("Active mode query cmd_t:%u\n", pStr.cmd_type);
			printf("Active mode query trc_t:%u\n", pStr.trans_type);
			printf("Active mode query powersave:%u\n",
				pStr.powersave);
			cJSON_AddNumberToObject(object, "cmd_type", pStr.cmd_type);
			cJSON_AddNumberToObject(object, "trans_type", pStr.trans_type);
			cJSON_AddNumberToObject(object, "powersave", pStr.powersave);
//			send_api = strdup();
			break;
		}
	case FROMBASE_REM_SCAN_QUERY_RESPONSE:
		{// (408) 3.7.6.2 REM扫描查询回应
			printf("####FiberHome-------"
				"REM scan query response!\n");
			_fiber_rem_scan_query_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("REM scan 设备ID:%s\n", id_tmp);
			printf("REM scan result:%u\n", ntohs(pStr.res.result));
			printf("REM scan  cause:%u\n", ntohs(pStr.res.cause));

			printf("REM scan cmd_type:%u\n", pStr.cmd_type);
			printf("REM scan mode:%u\n", pStr.mode);
			printf("REM scan scanfcn:%s\n", pStr.scanfcn);
			printf("REM scan plmn:%s\n", pStr.plmn);
			printf("REM scan thrrsrp:%u\n", ntohl(pStr.thrrsrp));
			cJSON_AddNumberToObject(object, "cmd_type", (U32)pStr.cmd_type);
			cJSON_AddNumberToObject(object, "mode", (U32)pStr.mode);
			cJSON_AddStringToObject(object, "scanfcn", pStr.scanfcn);
			cJSON_AddStringToObject(object, "plmn", pStr.plmn);
			cJSON_AddNumberToObject(object, "thrrsrp", ntohl(pStr.thrrsrp));
//			send_api = strdup();
			break;
		}
	case FROMBASE_IMSI_CONTROL_LIST_CONFIG_RESPONSE:
		{// (411)3.7.11.2 黑白名单配置回应 No more support
			printf("####FiberHome-------"
				"contrl list config response!\n");
			break;
		}
	case FROMBASE_IMSI_CONTROL_LIST_QUERY_RESPONSE:
		{// (413)3.7.12.2 黑白名单查询回应 No more support
			printf("####FiberHome-------"
				"contrl list query response!\n");
			_fiber_control_list_query_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("control list query 设备ID:%s\n", id_tmp);
			printf("control list query num:%u\n", ntohl(pStr.num));
			U32 count;
			U64 imsi;
			for (count = 0; count < ntohl(pStr.num); count++) {
				printf("imsi[%u]:%llx\n", count,
					my_htonll_ntohll(pStr.imsi[count]));
			}
			break;
		}
	case FROMBASE_ADAREA_SCAN_QUERY_RESPONSE:
		{// (415)3.7.15 邻区扫描结果查询回应 No more support
			printf("####FiberHome-------"
				"nb adarea scan query response!\n");
			break;
		}
	case FROMBASE_ADAREA_LIST_QUERY_RESPONSE:
		{// (423) 3.7.29.2邻区表查询回应
			printf("####FiberHome-------"
				"nb adarea list query response!\n");
			_fiber_nbear_list_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("nb adarea list 设备ID:%s\n",id_tmp);
			printf("nb adarea list result:%u\n",
				ntohs(pStr.res.result));
			printf("nb adarea scan list cause:%u\n",
				ntohs(pStr.res.cause));
			cJSON_AddNumberToObject(object, "num", ntohs(pStr.num));
			cJSON *array = cJSON_CreateArray();
			if (!array) break;
			cJSON_AddItemToObjectCS(object, "list", array);
			U16 count = 0;
			for (count = 0; count < ntohs(pStr.num); count++) {
				printf("pStr->ary[%u].nbearfcn:%u", count,
					ntohl(pStr.ary[count].nbearfcn));
				printf("pStr->ary[%u].nbpci:%u", count,
					ntohl(pStr.ary[count].nbpci));
				printf("pStr->ary[%u].nbdev:%u", count,
					ntohs(pStr.ary[count].nbdev));
				printf("pStr->ary[%u].nbrsrp:%u", count,
					ntohl(pStr.ary[count].nbrsrp));
				printf("pStr->ary[%u].nbreselectsib3:%u",
					count, pStr.ary[count].nbreselectsib3);
				cJSON *obj = cJSON_CreateObject();
				if (!obj) continue;
				cJSON_AddNumberToObject(obj, "nbearfcn",
					ntohl(pStr.ary[count].nbearfcn));
				cJSON_AddNumberToObject(obj, "nbpci",
					ntohl(pStr.ary[count].nbpci));
				cJSON_AddNumberToObject(obj, "nbdev",
					(int)ntohs(pStr.ary[count].nbdev));
				cJSON_AddNumberToObject(obj, "nbrsrp",
					ntohl(pStr.ary[count].nbrsrp));
				cJSON_AddNumberToObject(obj, "nbreselectsib3",
					pStr.ary[count].nbreselectsib3);
				cJSON_AddItemToArray(array, obj);
			}
//			send_api = strdup();
			break;
		}
	case FROMBASE_FREQUENCY_LIST_QUERY_RESPONSE:
		{// (425) 3.7.30.2 频率表查询回应
			printf("####FiberHome-------"
				"frequency list query response!\n");
			_fiber_freq_list_res pStr;
			memcpy(&pStr, recBuf, sizeof(pStr));
			S8 id_tmp[22] = {0};
			memcpy(id_tmp, pStr.res.device_id, 21);
			printf("frequency list 设备ID:%s\n",id_tmp);
			printf("frequency list result:%u\n",
				ntohs(pStr.res.result));
			printf("frequency list cause:%u\n",
				ntohs(pStr.res.cause));
			cJSON_AddNumberToObject(object, "num", ntohs(pStr.num));
			cJSON *array = cJSON_CreateArray();
			if (!array) break;
			cJSON_AddItemToObjectCS(object, "list", array);
			U16 count = 0;
			for (count = 0; count < ntohs(pStr.num); count++) {
				printf("pStr->ary[%u].nbearfcn:%u", count,
					ntohl(pStr.ary[count].nbearfcn));
				printf("pStr->ary[%u].nbreselectsib5:%u",
					count, pStr.ary[count].nbreselectsib5);
				cJSON *obj = cJSON_CreateObject();
				if (!obj) continue;
				cJSON_AddNumberToObject(obj, "nbearfcn",
					ntohl(pStr.ary[count].nbearfcn));
				cJSON_AddNumberToObject(obj, "nbreselectsib5",
					pStr.ary[count].nbreselectsib5);
				cJSON_AddItemToArray(array, obj);
			}
//			send_api = strdup();
			break;
		}
	case FROMBASE_PROBE_CONFIG_RESPONSE:
		{/* (302)3.7.1.2 采集配置回应 generic */
			printf("####FiberHome-------probe config response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_CELL_ACTIVE_MODE_CONFIG_RESPONSE:
		{/* (402)3.7.2.2 运行模式配置回应 generic */
			printf("####FiberHome-------"
				"Cell active mode config response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_REM_SCAN_CONFIG_RESPONSE:// 
		{/* (406)3.7.5.2 REM扫描配置回应 generic */
			printf("####FiberHome-------"
				"REM scan config response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_TXPOWER_CONFIG_RESPONSE:
		{/* (306)3.7.13.2 功率档位设置回应(OK) generic */
			printf("####FiberHome-------"
				"TXpower config response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_DEVICE_IP_CONFIG_RESPONSE:
		{/* (309)3.7.16.2设置基站IP回应 generic */
			printf("####FiberHome-------IPaddr config response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_RESET_DEVICE_RESPONSE:
		{/* (312)3.7.18.2 复位基站回应 generic */
			printf("####FiberHome-------"
				"reset device response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_TIME_CONFIG_RESPONSE:// 
		{/* (314)3.7.19.2 设置基站时间回应 generic */
			printf("####FiberHome-------"
				"site device time  response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_VERSION_UPDATE_RESPONSE:
		{/* (318)3.7.22.2 基站升级操作回应 generic */
			printf("####FiberHome-------"
				"device update response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_VERSION_UPDATE_FINISH_REPORT:// 
		{/* (319)3.7.22.3 基站升级操作结束指示 generic */
			printf("####FiberHome-------"
				"device update finished report!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_IMSI_UP_FINISH_REPORT:
		{/* (325) 3.7.24.3 IMSI数据上传结束指示 generic */
			printf("####FiberHome-------IMSI up finish report!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_PROBE_SWITCH_CONFIG_REQUEST:
		{/* (417)3.7.26 采集开关回应 generic */
			printf("####FiberHome-------"
				"Probe switch config response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_TIMING_REBOOT_CONFIG_REQUEST:
		{/* (419)3.7.27.2 定时重启回应 generic */
			printf("####FiberHome-------"
				"timing switch config response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_DELETE_ADAREA_SCAN_LIST_RESPONSE:
		{/* (421)3.7.28.2一键删除邻区列表回应 generic */
			printf("####FiberHome-------"
				" delete adarea scan list response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	case FROMBASE_ACTIVE_WARING_RESPONSE:
		{/* (0x080B)4.6.3 活跃告警激活响应 generic */
			printf("####FiberHome-------"
				"Active warning response!\n");
			generic_response_handle(entry, ntohs(msg_type), recBuf);
			break;
		}
	default:
		{
			printf("####FiberHome-------msgt[%u] not supported\n",
				ntohs(msg_type));
			break;
		}
	}/* switch (ntohs(msg_type)) */
	if (send_api) {
		S8 *send_str = cJSON_PrintUnformatted(object);
		if (send_str) {
			printf("send_str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
	//		http_send(send_api, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
		}
		free(send_api);
	}
go_return:
	cJSON_Delete(object);
}

/* ==================================================== 
 * 烽火基站发送通用指令
 * U16 msg_type        -- 协议类型 
 * band_entry_t *entry -- 保存有基带板信息数据的结构体指针
 * ====================================================
 * */
void send_generic_request_to_fiberhome(U16 msg_type, band_entry_t *entry)
{
	if (!entry || !(entry->sn)) return;
	srand(time(NULL));
	_fiberhome_header header;
	header.transid = htons(rand() % 1000 + 1);
	header.msgtype = htons(msg_type);
	header.version = htons(22);
	_fiber_genneric_set_req sStr;
	memcpy(sStr.device_id, entry->sn, 21);
	U32 send_length = sizeof(sStr);
	header.length = htonl(send_length);
	sStr.header = header;
	U8 send_request[256] = {0};
	memcpy(send_request, &sStr, sizeof(sStr));
	if (send_length > 0) {
		int ret = send(entry->sockfd, send_request, send_length, 0);
		if (ret != send_length) {
			printf("####----Send to FibserHome device failed!\n");
		}
	}
}
/* ====================================================
 * 烽火基站mosquito指令信息解析
 * S8 *mqtt_msg        -- 后台下发的用于配置的指令字符串(JSON 格式数据) 
 * band_entry_t *entry -- 保存有基带板信息数据的结构体指针
 * ====================================================
 * */
void fiberhome_pare_config_json_message(S8 *mqtt_msg, band_entry_t *entry)
{
	cJSON *pa_object = cJSON_Parse(mqtt_msg);
	if (!pa_object) {
		printf("in function:%s,Pare json object failed!\n" , __func__);
		return;
	}
	/* get config msg type */
	cJSON *msgtype = cJSON_GetObjectItem(pa_object, "msgtype");
	if (!msgtype)
		goto go_return;
	U32 msgt = 0;
	sscanf(msgtype->valuestring, "%u", &msgt);

	U8 send_request[MAXDATASIZE] = {0}; /* to base band device */
	U32 send_length = 0;

	srand(time(NULL));
	_fiberhome_header header;
	header.transid = htons(rand() % 1000 + 1);
	header.msgtype = htons(msgt);
	header.version = htons(22);

	switch(msgt) {
	case TOBASE_CELL_ACTIVE_MODE_CONFIG_REQUEST:
		{//(401)3.7.2.1 小区运行模式配置请求 (OK)
			printf("####----Command to Fiberhome device:"
				"cell active mode config request!\n");
			cJSON *cmdtype
				= cJSON_GetObjectItem(pa_object, "cmdtype");
			cJSON *transtype
				= cJSON_GetObjectItem(pa_object, "transtype");
			cJSON *reboot
				= cJSON_GetObjectItem(pa_object, "reboot");
			cJSON *powersave
				= cJSON_GetObjectItem(pa_object, "powersave");
			if (!cmdtype || !transtype || !reboot || !powersave) {
				goto go_return;
			}
			_fiber_cell_act_cfg_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.cmd_type = atoi(cmdtype->valuestring);
			sStr.trans_type= atoi(transtype->valuestring);
			sStr.reboot = atoi(reboot->valuestring);
			sStr.powersave = atoi(powersave->valuestring);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_PROBE_CONFIG_REQUEST:
		{// (301)3.7.4 (手动)建站配置请求(OK)自动模式下只要配置tac和collectperiod
			printf("####----Command to Fiberhome device:"
				"probe config request!\n");
			cJSON *cmd_type = cJSON_GetObjectItem(pa_object,
				"cmd_type");
			cJSON *oper = cJSON_GetObjectItem(pa_object, "oper");
			cJSON *preten_cellid =
				cJSON_GetObjectItem(pa_object, "preten_cellid");
			cJSON *workfcn =
				cJSON_GetObjectItem(pa_object, "workfcn");
			cJSON *plmn = cJSON_GetObjectItem(pa_object, "plmn");
			cJSON *pci = cJSON_GetObjectItem(pa_object, "pci");
			cJSON *band = cJSON_GetObjectItem(pa_object, "band");
			cJSON *tac = cJSON_GetObjectItem(pa_object, "tac");
			cJSON *powerrank =
				cJSON_GetObjectItem(pa_object, "powerrank");
			cJSON *collectperiod =
				cJSON_GetObjectItem(pa_object, "collectperiod");

			if (!cmd_type | !oper | !preten_cellid | !workfcn |
				!plmn | !pci | !band | !tac | !powerrank |
				!collectperiod) {
				goto go_return;
			} 
			_fiber_probe_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.cmd_type = cmd_type->valuestring[0];
			sscanf(powerrank->valuestring, "%u", &sStr.powerrank);
			sscanf(workfcn->valuestring, "%u", &sStr.workfcn);
			sscanf(oper->valuestring, "%u", &sStr.oper);
			sscanf(plmn->valuestring, "%u", &sStr.plmn);
			sscanf(band->valuestring, "%u", &sStr.band);
			sscanf(pci->valuestring, "%u", &sStr.pci);
			sscanf(tac->valuestring, "%u", &sStr.tac);
			sscanf(preten_cellid->valuestring, "%u",
				&sStr.preten_cellid);
			sscanf(collectperiod->valuestring, "%u",
				&sStr.collectperiod);

			sStr.oper = htons(sStr.oper);
			sStr.preten_cellid = htonl(sStr.preten_cellid);
			sStr.collectperiod = htonl(sStr.collectperiod);
			sStr.powerrank = htonl(sStr.powerrank);
			sStr.workfcn = htons(sStr.workfcn);
			sStr.plmn = htonl(sStr.plmn);
			sStr.band = htonl(sStr.band);
			sStr.pci = htonl(sStr.pci);
			sStr.tac = htonl(sStr.tac);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_REM_SCAN_CONFIG_REQUEST:
		{//(405)3.7.5.1 REM扫描配置(OK)
			printf("####----Command to Fiberhome device:"
				"REM scan config request!\n");
			cJSON *cmdtype
				= cJSON_GetObjectItem(pa_object, "cmdtype");
			cJSON *mode
				= cJSON_GetObjectItem(pa_object, "mode");
			cJSON *scanfcn
				= cJSON_GetObjectItem(pa_object, "scanfcn");
			cJSON *plmn
				= cJSON_GetObjectItem(pa_object, "plmn");
			cJSON *thrsrp
				= cJSON_GetObjectItem(pa_object, "thrsrp");
			if (!cmdtype ||!mode ||!scanfcn ||!plmn ||!thrsrp) {
				goto go_return;
			}
			_fiber_rem_scan_cfg_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.cmd_type = atoi(cmdtype->valuestring);
			sStr.mode = atoi(mode->valuestring);
			sStr.thrsrp = htonl(atoi(thrsrp->valuestring));
			strcpy(sStr.scanfcn, scanfcn->valuestring);
			strcpy(sStr.plmn, plmn->valuestring);
			memcpy(send_request, &sStr, sizeof(sStr));

			break;
		}
	case TOBASE_IMSI_CONTROL_LIST_CONFIG_REQUEST:
		{//(410)3.7.11 IMSI黑白名单配置 No more support
			printf("####----Command to Fiberhome device:"
				"Control list config request!\n");
			cJSON *num = cJSON_GetObjectItem(pa_object, "num");
			cJSON *type = cJSON_GetObjectItem(pa_object, "type");
			cJSON *imsi_ary
				= cJSON_GetObjectItem(pa_object, "list");
			if(!num || !type || !imsi_ary) {
				goto go_return;
			}
			U32 imsi_l, imsi_h;
			_fiber_control_list_cfg_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.num = htonl(atoi(num->valuestring));
			sStr.type = atoi(type->valuestring);
			U8 count = cJSON_GetArraySize(imsi_ary);
			U8 number;
			for (number = 0; number < count; number++) {
				cJSON *imsi = cJSON_GetArrayItem(imsi_ary,
					(int)number);
				sscanf(imsi->valuestring, "%llu",
					&(sStr.imsi[number]));
				imsi_l = sStr.imsi[number] & 0xFFFFFFFF;
				imsi_h = ((sStr.imsi[number])>>32) & 0xFFFFFFFF;
				sStr.imsi[number] = htonl(imsi_l);
				sStr.imsi[number] = (sStr.imsi[number]<<32) |
					htonl(imsi_h);
			}
			for (; number < 32; number++) {
				sStr.imsi[number] = 0;
			}
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_TXPOWER_CONFIG_REQUEST:
		{//(305)3.7.13.1 功率档位设置请求 (OK)
			printf("####----Command to Fiberhome device:"
				"Txpower config request!\n");
			cJSON *txpower =
				cJSON_GetObjectItem(pa_object,"txpower");
			if (!txpower)
				break;
			_fiber_txpower_set_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sscanf(txpower->valuestring, "%u", &(sStr.powerrank));
			sStr.powerrank = htonl(sStr.powerrank);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_DEVICE_IP_CONFIG_REQUEST:
		{//(309)3.7.16 设置基站IP请求 No more support
			printf("####----Command to Fiberhome device:"
				"device IP config request!\n");
			break;
		}
	case TOBASE_TIME_CONFIG_REQUEST:
		{//(313)3.7.19.1 设置基站时间请求(OK)
			printf("####----Command to Fiberhome device:"
				"device time config request!\n");
			/* date:YYYY-MM-DD */
			/* time:HH:MM:SS */
			cJSON *_date = cJSON_GetObjectItem(pa_object, "date");
			cJSON *_time = cJSON_GetObjectItem(pa_object, "time");
			if (!_date || !_time) {
				goto go_return;
			}
			_fiber_time_cfg_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			memcpy(sStr._date, _date->valuestring, strlen(_date->valuestring));
			memcpy(sStr._time, _time->valuestring, strlen(_time->valuestring));
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_VERSION_UPDATE_REQUEST:
		{//(317)3.7.22.1 基站升级操作请求(OK)
			printf("####----Command to Fiberhome device:"
				"device version update request!\n");
			cJSON *u_ip =
				cJSON_GetObjectItem(pa_object, "server_ip");
			cJSON *f_user =
				cJSON_GetObjectItem(pa_object, "ftp_user");
			cJSON *f_password =
				cJSON_GetObjectItem(pa_object, "ftp_password");
			cJSON *u_filename =
				cJSON_GetObjectItem(pa_object, "u_filename");
			if (!u_ip || !f_user || !f_password || !u_filename) {
				goto go_return;
			}
			_fiber_update_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.ip_ver = 0;
			sscanf(u_ip->valuestring, "%u.%u.%u.%u",
				(U32 *)&(sStr.ftp_server_ip[0]),
				(U32 *)&(sStr.ftp_server_ip[1]),
				(U32 *)&(sStr.ftp_server_ip[2]),
				(U32 *)&(sStr.ftp_server_ip[3]));
			snprintf(sStr.ftp_user, 40, "%s",
				f_user->valuestring);
			snprintf(sStr.ftp_password, 40, "%s",
				f_password->valuestring);
			snprintf(sStr.filename, 40, "%s",
				u_filename->valuestring);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_GET_IMSI_INFO_REQUEST:
		{//(323)3.7.24.1 IMSI 数据上传请求(OK)
			printf("####----Command to Fiberhome device:"
				"get IMSI(cache in device) request!\n");
			cJSON *f_ip =
				cJSON_GetObjectItem(pa_object, "server_ip");
			cJSON *f_user =
				cJSON_GetObjectItem(pa_object, "ftp_user");
			cJSON *f_password =
				cJSON_GetObjectItem(pa_object, "ftp_password");
			if (!f_ip || !f_user || !f_password) {
				goto go_return;
			}
			_fiber_imsi_ftp_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.ip_ver = 0;
			sscanf(f_ip->valuestring, "%u.%u.%u.%u",
				(U32 *)&(sStr.ftp_server_ip[0]),
				(U32 *)&(sStr.ftp_server_ip[1]),
				(U32 *)&(sStr.ftp_server_ip[2]),
				(U32 *)&(sStr.ftp_server_ip[3]));
			snprintf(sStr.ftp_user, 40, "%s", f_user->valuestring);
			snprintf(sStr.ftp_password, 40, "%s",
				f_password->valuestring);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_PROBE_SWITCH_CONFIG_REQUEST:
		{// (416)3.7.26 采集开关(OK)
			printf("####----Command to Fiberhome device:"
				"probe switch config request!\n");
			cJSON *type = cJSON_GetObjectItem(pa_object,"cmd_type");
			if (!type) goto go_return;
			_fiber_probe_switch_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.cmd_type = atoi(type->valuestring);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_TIMING_REBOOT_CONFIG_REQUEST:
		{// (418)3.7.27 定时重启(OK)
			printf("####----Command to Fiberhome device:"
				"timing reboot request!\n");
			cJSON *time = cJSON_GetObjectItem(pa_object, "time");
			if (!time) goto go_return;
			_fiber_timing_reboot_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			strcpy(sStr.time, time->valuestring);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_DELETE_ADAREA_SCAN_LIST_REQUEST:
		{// (420)3.7.28 一键删除邻区列表(OK)
			printf("####FiberHome-------"
				"delete adarea scan list request!\n");
			cJSON *delete
				= cJSON_GetObjectItem(pa_object, "delete");
			if (!delete) goto go_return;
			_fiber_del_adarea_scan_req sStr;
			if (!(entry->sn)) break;
			memcpy(sStr.req.device_id, entry->sn, 21);
			send_length = sizeof(sStr);
			header.length = htonl(send_length);
			sStr.req.header = header;
			sStr.delete_enable = atoi(delete->valuestring);
			memcpy(send_request, &sStr, sizeof(sStr));
			break;
		}
	case TOBASE_CELL_ACTIVE_MODE_QUERY_REQUEST:
		{//(403)3.7.3.1 小区运行模式查询OK
			printf("####----Command to Fiberhome device:"
				"cell active mode query request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_REM_SCAN_QUERY_REQUEST:
		{//(407)3.7.6.1REM扫描配置查询请求OK
			printf("####----Command to Fiberhome device:"
				"REM scan config query request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_IMSI_CONTROL_LIST_QUERY_REQUEST:
		{// (412)3.7.12.1 IMSI黑白名单查询 No more support!
			printf("####----Command to Fiberhome device:"
				"IMSI control list query request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_GET_PROBE_CONDIF_REQUEST:
		{//(307)3.7.14.1获取基站采集参数请求OK
			printf("####----Command to Fiberhome device:"
				"get device status request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_GET_IP_REQUEST:
		{//(330)3.7.17.1基站IP查询请求OK
			printf("####----Command to Fiberhome device:"
				"get device IP request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_RESET_DEVICE_REQUEST:
		{//(311)3.7.18.1复位基站请求(OK)
			printf("####----Command to Fiberhome device:"
				"reset device request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_GET_TIME_REQUEST:
		{//(328)3.7.20.1 基站时间查询请求OK
			printf("####----Command to Fiberhome device:"
				"get device time request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_GET_DEVICE_INFO_REQUEST:
		{//(315)3.7.21.1 设备信息查询请求(OK)
			printf("####----Command to Fiberhome device:"
				"get device information request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_GET_DEVICE_SITE_REQUEST:
		{//(321)3.7.23.1基站位置查询请求 (OK) No more support
			printf("####----Command to Fiberhome device:"
				"get device site request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_ADAREA_LIST_QUERY_REQUEST:
		{// (422)3.7.29.1邻区表查询请求
			printf("####----Command to Fiberhome device:"
				"adarea list query request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_FREQUENCY_LIST_QUERY_REQUEST:
		{// (424) 3.7.30.1频率表查询
			printf("####----Command to Fiberhome device:"
				"frequency list query request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_ACTIVE_WARING_REUEST:
		{//(345)告警上报消息请求
			printf("####----Command to Fiberhome device:"
				"get active warning request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	case TOBASE_ADAREA_SCAN_QUERY_REQUEST:
		{// (414)邻区扫描结果查询
			printf("####----Command to Fiberhome device:"
				"Adarea scan query request!\n");
			send_generic_request_to_fiberhome(msgt, entry);
			break;
		}
	default:
		break;
	} /* end switch(msgt) */
	if (send_length > 0) {
		int ret = send(entry->sockfd, send_request, send_length, 0);
		if (ret != send_length) {
			printf("####----Send to FibserHome device failed!\n");
		}
	}
go_return:
	cJSON_Delete(pa_object);
}
/* 每隔args_info.sendsta_itv_arg 时间，检查用户信息json数组中有没有用户信息,
 * 如果有,加锁并将json转换成字符串并使用http协议上至服务器, 
 * 然后删除json中的所有信息,重新初始化json数组
 * */
void *thread_read_send_fiberhome_json_object(void *arg)
{
	char *send_str = NULL;
	while (1) {
		sleep(args_info.sendsta_itv_arg);
		fiber_sta_json_lock();
		if (!fiber_sta_count) {
			fiber_sta_json_unlock();
			continue;
		}
		send_str = cJSON_PrintUnformatted(fiber_sta_object);
		if (!send_str) {
			fiber_sta_json_unlock();
			continue;
		}
		printf("Timeout，send Fiber LTE info str:%s\n", send_str);
#ifdef SCACHE
		if (get_netstate() == NETWORK_OK) {
#endif
			pthread_mutex_lock(&http_send_lock);
			http_send_client(args_info.station_api_arg, send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
#ifdef SCACHE
		} else {
			write_string_cachefile(send_str);
		}
#endif
		free(send_str);
		fiber_sta_json_delete();
		fiber_sta_json_init();
		fiber_sta_json_unlock();
	} /* end while(... */
	return NULL;
}
