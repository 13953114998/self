/*
 * ============================================================================
 *
 *       Filename:  baicell_net.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年05月30日 15时10分08秒
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

#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "arg.h"
#include "config.h"
#include "cJSON.h"
#include "cli_action.h"
#include "http.h"
#include "my_log.h"
#include "imsi_list_acton.h"
#include "baicell_net.h"
#include "cache_file.h"
#include "scan_fre.h"

cJSON *bc_sta_object = NULL;
cJSON *bc_sta_array = NULL;
S32 bc_sta_count = 0;
pthread_mutex_t bc_sta_obj_lock; // 锁，拿到锁才能对-|bc_sta_object 进行操作
/* baicell socket about */
#define BAICELL_MAX_MSG_LENGTH (40960) /* max length of baicell's massage  */
static S32 bc_server_fd = -1;
static S32 bc_client_fd = -1; /* tcp 状态下接收保存client的fd, UDP下为发送fd */
static struct sockaddr_in bc_server_addr;
static struct sockaddr_in bc_client_addr; 
socklen_t bc_addrlen = sizeof(struct sockaddr);

#pragma pack(1)
typedef struct baicell_complicated_package {
	S8 *ip;
	U8 *pkg;
#ifdef CLI
	struct cli_req_if cli_info;
#endif
} bc_complicated_pkg;
#pragma pack()

/* baicell sta JSON save about */
static void bc_sta_obj_lock_init()
{
	pthread_mutex_init(&bc_sta_obj_lock, NULL);
}
void bc_sta_json_obj_lock()
{
	pthread_mutex_lock(&bc_sta_obj_lock);
}
void bc_sta_json_obj_unlock()
{
	pthread_mutex_unlock(&bc_sta_obj_lock);
}

cJSON *get_bc_sta_object()
{
	return bc_sta_object;
}
cJSON *get_bc_sta_array()
{
	return bc_sta_array;
}
void bc_sta_array_sum(cJSON *array)
{
	S32 count = cJSON_GetArraySize(array);
	S32 i;
	for (i = 0; i < count; i++) {
		cJSON *object = cJSON_GetArrayItem(array, i);
		if (object) {
			cJSON_AddItemReferenceToArray(bc_sta_array, object);
		}
	}

}
S32 get_bc_sta_count()
{
	return bc_sta_count;
}
void bc_sta_count_sum(S32 num)
{
	bc_sta_count += num;
}

/* 初始化保存baicell 基带板上传的IMSI信息的JSON数组 */
void bc_sta_json_init()
{
	bc_sta_object = cJSON_CreateObject();
	if (bc_sta_object == NULL) exit(0);
	cJSON_AddStringToObject(bc_sta_object, "topic", my_topic);
	bc_sta_array = cJSON_CreateArray();
	if (bc_sta_array == NULL) {
		cJSON_Delete(bc_sta_object);
		exit(0);
	}
	cJSON_AddItemToObjectCS(bc_sta_object,"clients", bc_sta_array);
	bc_sta_count = 0;
	printf("Station json object Inited!\n");
}
/* 清除保存baicell基带板上传的IMSI信息的JSON数组 */
void bc_sta_json_delete()
{
	cJSON_Delete(bc_sta_object);
}

/* 创建并初始化与Baicell基带板交互的socket
 * success return 1
 * failed return 0
 * */
S32 init_baicell_socket()
{
	if (args_info.bc_proto_arg == BAICELL_PROTOCOL_TCP) {
		bc_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	} else { /* BAICELL_PROTOCOL_UDP */
		bc_server_fd = socket(AF_INET, SOCK_DGRAM, 0);
		bc_client_fd = socket(AF_INET, SOCK_DGRAM, 0);
	}
	if (bc_server_fd == -1) {
		printf("Baicell socket creat failed!%s(%d)\n",
			strerror(errno), errno);
		return 0;
	}
	S32 ret = 0;
	memset(&bc_server_addr, 0, sizeof(struct sockaddr_in));
	bc_server_addr.sin_family = AF_INET;
	bc_server_addr.sin_addr.s_addr = INADDR_ANY;
	bc_server_addr.sin_port = htons(args_info.port_arg);
	ret = bind(bc_server_fd,
		(struct sockaddr *)&bc_server_addr, bc_addrlen);
	if (ret < 0) {
		printf("Baicell server bind failed!%s(%d)\n",
			strerror(errno), errno);
		close(bc_server_fd);
		return 0;
	}
	if (args_info.bc_proto_arg == BAICELL_PROTOCOL_TCP) {
		/* set TCP option */
		int flag = 1;
		ret = setsockopt(bc_server_fd, SOL_SOCKET, SO_REUSEADDR,
			&flag, sizeof(flag));
		if(ret < 0) {
			printf("Set socket failed: %s(%d)\n",
				strerror(errno), errno);
			close(bc_server_fd);
			return 0;
		}
		struct timeval timeout = {10, 0};
		ret = setsockopt(bc_server_fd, SOL_SOCKET, SO_RCVTIMEO,
			&timeout, sizeof(timeout));
		if(ret < 0) {
			printf("Set socket recv time out failed: %s(%d)\n",
				strerror(errno), errno);
			close(bc_server_fd);
			return 0;
		}
		struct timeval s_timeout = {2, 0};
		ret = setsockopt(bc_server_fd, SOL_SOCKET, SO_SNDTIMEO,
			&s_timeout, sizeof(s_timeout));
		if(ret < 0) {
			printf("Set socket send time out failed: %s(%d)\n",
				strerror(errno), errno);
			close(bc_server_fd);
			return 0;
		}
		ret = listen(bc_server_fd, MAX_HLIST_LENGTH);
		if (ret == -1) {
			printf("listen failed %s(%d)\n",
				strerror(errno), errno);
			close(bc_server_fd);
			return 0;
		}
	} /* end if(args_info.bc_proto_arg == BAICELL_PROTOCOL_TCP) */

	printf("create Baicell Server success!\n");
	/* init baicell probe IMSI info JSON object and array */
	bc_sta_json_init();
	/* init bacicell JSON object action lock */
	bc_sta_obj_lock_init();
	/* init save Baicell IMSI struct list */
	if (init_imsi_list_heads() == -1) return 0;
	return 1;
}
/* 向Baicell 基带板发送请求(TCP/UDP) */
S32 baicell_send_msg_to_band(U8 *send_buffer, U32 send_len, S8 *ipaddr)
{
	band_entry_t *entry = band_list_entry_search(ipaddr);
	if (!entry) return -1;
	printf("send pakage (len:%u)to band:%s\n",send_len, ipaddr);
	if (args_info.bc_proto_arg == BAICELL_PROTOCOL_TCP) {
		return send(entry->sockfd, send_buffer, send_len, 0);
	} else { /* args_info.bc_proto_arg == BAICELL_PROTOCOL_UDP */
		struct sockaddr_in send_addr;
		send_addr.sin_family = AF_INET;
		send_addr.sin_addr.s_addr = inet_addr(ipaddr); 
		send_addr.sin_port = htons(args_info.port_arg);

		socklen_t addr_len = sizeof(struct sockaddr);
		return sendto(bc_client_fd, send_buffer, send_len, 0,
			(struct sockaddr *)&send_addr, addr_len);
	}
}
/* 向基代板发送基础请求 */
void send_generic_request_to_baseband(S8 *ip, U16 msgtype)
{
	wrMsgHeader_t WrmsgHeader;
	WrmsgHeader.u16MsgType = msgtype;
	WrmsgHeader.u32FrameHeader = MSG_FRAMEHEAD_DEF;
	WrmsgHeader.u16frame = SYS_WORK_MODE_TDD;
	WrmsgHeader.u16SubSysCode = SYS_CODE_DEFAULT;
	WrmsgHeader.u16MsgLength = sizeof(WrmsgHeader);
#ifdef BIG_END
	WrmsgHeader.u16MsgType = my_htons_ntohs( WrmsgHeader.u16MsgType);
	WrmsgHeader.u32FrameHeader = my_htonl_ntohl(WrmsgHeader.u32FrameHeader);
	WrmsgHeader.u16frame = my_htons_ntohs(WrmsgHeader.u16frame);
	WrmsgHeader.u16SubSysCode = my_htons_ntohs(WrmsgHeader.u16SubSysCode);
	WrmsgHeader.u16MsgLength = my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
	U8 send_msg[MAXDATASIZE] = {0};
	switch(msgtype) {
	case O_FL_LMT_TO_ENB_BASE_INFO_QUERY: /* (0xF02B)基带板基础信息查询 */
		{
			U32 _id = 7;
			wrFLLmtToEnbBaseInfoQuery_t pStr;
			WrmsgHeader.u16MsgLength = sizeof(pStr);
#ifdef BIG_END
			WrmsgHeader.u16MsgLength = my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			while(_id) {
				pStr.u32EnbBaseInfoType = _id - 1;
#ifdef BIG_END
				pStr.u32EnbBaseInfoType = my_htonl_ntohl(pStr.u32EnbBaseInfoType);
#endif
				memcpy(send_msg, &pStr, sizeof(pStr));
				sleep(1);
				baicell_send_msg_to_band(send_msg , sizeof(pStr), ip);
				_id--;
			}
			break;
		}
	case O_FL_LMT_TO_ENB_REBOOT_CFG:      //(0xF00B) 3. 8 重启指示
	case O_FL_LMT_TO_ENB_GET_ENB_STATE:   //(0xF01A) 3.20 获取小区最后一次操作执行的状态请求
	case O_FL_LMT_TO_ENB_GET_ARFCN:       //(0xF027) 3.34 小区频点配置相关参数查询
	case O_FL_LMT_TO_ENB_SYNC_INFO_QUERY: //(0xF02D) 3.42 基站同步信息查询
	case O_FL_LMT_TO_ENB_CELL_STATE_INFO_QUERY:       //(0xF02F) 3.44 小区状态信息查询
	case O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY:  //(0xF031) 3.46 接收增益和发射功率查询
	case O_FL_LMT_TO_ENB_ENB_IP_QUERY:                //(0xF033) 3.48 IP地址查询
	case O_FL_LMT_TO_ENB_QRXLEVMIN_VALUE_QUERY:       //(0xF035) 3.50 小区选择 QRxLevMin 查询
	case O_FL_LMT_TO_ENB_REM_CFG_QUERY:               //(0xF037) 3.52 扫频参数配置查询
	case O_FL_LMT_TO_ENB_MEAS_UE_CFG_QUERY:           //(0xF03D) 4. 3 UE 测量配置查询
	case O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG_QUERY:     //(0xF03F) 3.54 重定向参数配置查询
	case O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON_QUERY://(0xF041) 3.56 上电小区自激活配置查询
	case O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY:          //(0xF05c) 3.73 Gps 经纬高度查询
	case O_FL_LMT_TO_ENB_CONTROL_LIST_QUERY: 	  //(0xF043) 4. 7 管控名单查询(黑白名单)
	case O_FL_LMT_TO_ENB_SYS_INIT_SUCC_RSP: 	  //(0xF011) 心跳回应)
		{
			memcpy(send_msg, &WrmsgHeader, sizeof(WrmsgHeader));
			baicell_send_msg_to_band(send_msg, sizeof(WrmsgHeader), ip);
			break;
		}
	default:
		printf("%X Not simple request! \n", WrmsgHeader.u16MsgType);
		break;
	}
}
/* 向Baicell发送基础查询信息请求 */
static void *thread_send_generic_request_to_bc(void *arg)
{
	S8 *ip = (S8 *)arg;
	/* 3.34. 小区频点配置相关参数查询 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_GET_ARFCN);
	/* 3.42 发送获取基带基设备础信息的请求 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_BASE_INFO_QUERY);
	/* 3.44. 基站同步信息查询 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_SYNC_INFO_QUERY);
	/* 3.46 发送获取小区状态信息的请求 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_CELL_STATE_INFO_QUERY);
	/* 3.54. 扫频参数配置查询 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_REM_CFG_QUERY);
	/* 3.58  上电自建小区配置查询 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON_QUERY);
	/* 3.48. 接收增益和发射功率查询 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY);
	/* 3.73  Gps 经纬高度查询 */
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY);
	return NULL;
}
void send_reboot_request_to_baicell(S8 *ip)
{
	send_generic_request_to_baseband(ip, O_FL_LMT_TO_ENB_REBOOT_CFG);
}
/* 解析baicell基带板发来的复杂的包(需要大量解析和组合json数组等工作的包) */
static void *thread_pare_bc_complicated_package(void *arg)
{
	bc_complicated_pkg *psk = (bc_complicated_pkg *)arg;
	S8 *client_ip = psk->ip;
	band_entry_t *entry = band_list_entry_search(client_ip);
	if (!entry) {
		goto free_return;
	}
#ifdef CLI
	struct cli_req_if cli_info = psk->cli_info;
#endif
	U16 msg_type = psk->pkg[4] | (psk->pkg[5] << 8);
	switch(msg_type) {
	case O_FL_ENB_TO_LMT_ARFCN_IND: //小区频点信息查询回复
		{
			wrFLLmtToEnbSysArfcnCfg_T *pStr =
				(wrFLLmtToEnbSysArfcnCfg_T *)psk->pkg;
			printf("<------cell Arfcn query response\n");
#ifdef BIG_END
			pStr->sysUlARFCN = my_htonl_ntohl(pStr->sysUlARFCN);
			pStr->sysDlARFCN = my_htonl_ntohl(pStr->sysDlARFCN);
			pStr->sysBand = my_htonl_ntohl(pStr->sysBand);
			pStr->PCI = my_htons_ntohs(pStr->PCI);
			pStr->TAC = my_htons_ntohs(pStr->TAC);
			pStr->CellId = my_htonl_ntohl(pStr->CellId);
			pStr->UePMax = my_htons_ntohs(pStr->UePMax);
			pStr->EnodeBPMax = my_htons_ntohs(pStr->EnodeBPMax);
#endif /* BIG_END */
			pthread_mutex_lock(&entry->lock);
			entry->sysUlARFCN = pStr->sysUlARFCN;
			entry->sysDlARFCN = pStr->sysDlARFCN;
			memcpy(entry->PLMN, pStr->PLMN, 7);
			entry->sysBandwidth = pStr->sysBandwidth;
			entry->sysBand = pStr->sysBand;
			entry->PCI = pStr->PCI;
			entry->TAC = pStr->TAC;
			entry->CellId = pStr->CellId;
			entry->UePMax = pStr->UePMax;
			entry->EnodeBPMax = pStr->EnodeBPMax;
			pthread_mutex_unlock(&entry->lock);

			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddNumberToObject(object, "sysuiarfcn",
				(int)pStr->sysUlARFCN);
			cJSON_AddNumberToObject(object, "sysdiarfcn",
				(int)pStr->sysDlARFCN);
			cJSON_AddStringToObject(object, "plmn",
				(char *)pStr->PLMN);
			cJSON_AddNumberToObject(object, "sysbandwidth",
				(int)pStr->sysBandwidth);
			cJSON_AddNumberToObject(object, "sysband",
				(int)pStr->sysBand);
			cJSON_AddNumberToObject(object, "pci", (int)pStr->PCI);
			cJSON_AddNumberToObject(object, "tac", (int)pStr->TAC);
			cJSON_AddNumberToObject(object, "cellid",
				(int)pStr->CellId);
			cJSON_AddNumberToObject(object, "uepmax",
				(int)pStr->UePMax);
			cJSON_AddNumberToObject(object, "enodebpmax",
				(int)pStr->EnodeBPMax);
			S8 *send_str = cJSON_PrintUnformatted(object);
			write_action_log(client_ip,
				"get cell Arfcn query response:");
			write_action_log(client_ip, send_str);
			cJSON_Delete(object);
			if (!send_str) break;
			printf("Arfcn point str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.brand_api_arg,send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
#ifndef BIG_END
			/* 将频点信息写入信息文件中 */
			char file_name[128] = {0};
			strcat(file_name, args_info.bsavepath_arg);
			if (file_name[strlen(file_name) - 1] != '/') {
				strcat(file_name, "/");
			}
			strcat(file_name, client_ip);
			printf("file name is %s\n", file_name);
			FILE *fp = fopen(file_name, "w+");
			if (fp) {
				fprintf(fp,
					"  sysUlARFCN : %u\n"
					"  sysDlARFCN : %u\n"
					"        PLMN : %s\n"
					"sysBandwidth : %u\n"
					"     sysBand : %u\n"
					"         PCI : %u\n"
					"         TAC : %u\n"
					"      CellId : %u\n"
					"      UePMax : %u\n"
					"  EnodeBPMax : %u\n",
					pStr->sysUlARFCN,
					pStr->sysDlARFCN,
					(char *)pStr->PLMN,
					pStr->sysBandwidth,
					pStr->sysBand,
					pStr->PCI,
					pStr->TAC,
					pStr->CellId,
					pStr->UePMax,
					pStr->EnodeBPMax);
				fclose(fp);
			}
#endif
#ifdef CLI
			char cli_resq[256] = {0};
			snprintf(cli_resq, 256,
				"IP:%s,Arfcn information:\n"
				"  sysUlARFCN : %u\n"
				"  sysDlARFCN : %u\n"
				"        PLMN : %s\n"
				"sysBandwidth : %u\n"
				"     sysBand : %u\n"
				"         PCI : %u\n"
				"         TAC : %u\n"
				"      CellId : %u\n"
				"      UePMax : %u\n"
				"  EnodeBPMax : %u\n",
				client_ip,
				pStr->sysUlARFCN,
				pStr->sysDlARFCN,
				(char *)pStr->PLMN,
				pStr->sysBandwidth,
				pStr->sysBand,
				pStr->PCI,
				pStr->TAC,
				pStr->CellId,
				pStr->UePMax,
				pStr->EnodeBPMax);
			cli_req_t *cli_entry =
				entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd, cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_TDD_SUBFRAME_ASSIGNMENT_QUERY_ACK:/* TDD子帧查询应答 */
		{
			wrFLLmtOrEnbSendYDDSubframeSetAndGet_t *pStr =
				(wrFLLmtOrEnbSendYDDSubframeSetAndGet_t *)psk->pkg;
			char tddsf[10] = {0}, tdd_[10] = {0};
			snprintf(tddsf, 10, "%u", pStr->u8TddSfAssignment);
			snprintf(tdd_, 10, "%u", pStr->u8TddSpecialSfPatterns);
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddStringToObject(object, "tddsf_assignment", tddsf);
			cJSON_AddStringToObject(object, "tdd_special_patterns", tdd_);
			S8 *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) break;
			printf("TDD subframe info report str:%s\n", send_str);
			write_action_log(client_ip,
				"get TDD subframe info response!");
			write_action_log("response", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.tddsubframe_api_arg, send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_BASE_INFO_QUERY_ACK: //获取基带板信息并完善链表
		{
			wrFLEnbToLmtBaseInfoQueryAck_t *pStr =
				(wrFLEnbToLmtBaseInfoQueryAck_t *)psk->pkg;
#ifdef BIG_END
			pStr->u32EnbBaseInfoType
				= my_htonl_ntohl(pStr->u32EnbBaseInfoType);
			pStr->WrmsgHeaderInfo.u16frame
				= my_htons_ntohs(pStr->WrmsgHeaderInfo.u16frame);
#endif
			pthread_mutex_lock(&(entry->lock));
			printf("IP:%sType[0x%x], info: %s\n",entry->ipaddr,
				pStr->u32EnbBaseInfoType, pStr->u8EnbbaseInfo);
#if 1
			//write log to buf in order to record more info, nyb,2018.03.27
			char logStr[200];
			sprintf(logStr, "u32EnbBaseInfoType[0x%x], info: %s\n",
				pStr->u32EnbBaseInfoType, pStr->u8EnbbaseInfo);
			write_action_log(client_ip,logStr);
#endif
			switch (pStr->u32EnbBaseInfoType) {
			case 0:
				if (strlen((char *)pStr->u8EnbbaseInfo)) {
					if (entry->dev_model)
						free(entry->dev_model);
					entry->dev_model =
						strdup((char *)pStr->u8EnbbaseInfo);
					entry->is_have |= IS_HAVE_BANDDEV_DEV_MODEL;
				}
				break;
			case 1:
				if (strlen((char *)pStr->u8EnbbaseInfo)) {
					if (entry->h_version)
						free(entry->h_version);
					entry->h_version =
						strdup((char *)pStr->u8EnbbaseInfo);
					entry->is_have |= IS_HAVE_BANDDEV_HVERSION;
				}
				break;
			case 2:
				if (strlen((char *)pStr->u8EnbbaseInfo)) {
					if (entry->s_version) {
						free(entry->s_version);
					}
					entry->s_version =
						strdup((char *)pStr->u8EnbbaseInfo);
					int xlen = strlen(entry->s_version);
					(entry->s_version)[xlen - 1] = '\0';
					entry->is_have |= IS_HAVE_BANDDEV_SVERSION;
				}
				break;
			case 3:
				if (strlen((char *)pStr->u8EnbbaseInfo)) {
					if (entry->sn) {
						free(entry->sn);
					}
					entry->sn =
						strdup((char *)pStr->u8EnbbaseInfo);
					entry->is_have |= IS_HAVE_BANDDEV_SN;
				}
				break;
			case 4:
				if (strlen((char *)pStr->u8EnbbaseInfo)) {
					if (entry->macaddr) {
						free(entry->macaddr);
					}
					entry->macaddr =
						strdup((char *)pStr->u8EnbbaseInfo);
					entry->is_have |= IS_HAVE_BANDDEV_MAC;
				}
			//	/* 如果信息完成,线程更新基带板信息 */
			//	if (entry->is_have & BASE_BAND_GENERIC_FULL) {
			//		safe_pthread_create(update_base_band_db,
			//			(void *)entry);
			//	}
				break;
			case 5: //UBOOT version
				if (strlen((char *)pStr->u8EnbbaseInfo)) {
					if (entry->ubt_version)
						free(entry->ubt_version);
					entry->ubt_version =
						strdup((char *)pStr->u8EnbbaseInfo);
					entry->is_have |= IS_HAVE_BANDDEV_UBOOTVER;
				}
				break;
			case 6: //board temp
				if (strlen((char *)pStr->u8EnbbaseInfo)) {
					if (entry->board_temp)
						free(entry->board_temp);
					entry->board_temp =
						strdup((char *)pStr->u8EnbbaseInfo);
					entry->is_have |= IS_HAVE_BANDDEV_BOARD_TEMP;
				}
				break;
			default:
				/* unknown */
				break;
			}
			/* 工作模式0 TDD,1 FDD */
			if (pStr->WrmsgHeaderInfo.u16frame
				== SYS_WORK_MODE_TDD) {
				entry->work_mode = TDD;
			} else if(pStr->WrmsgHeaderInfo.u16frame
				== SYS_WORK_MODE_FDD) {
				entry->work_mode = FDD;
			}
			pthread_mutex_unlock(&(entry->lock));
			break;
		}
	case O_FL_ENB_TO_LMT_SYNC_INFO_QUERY_ACK://3.43. 基站同步信息查询应答
		{
			printf("<---- device sync info query response\n");
			wrFLEnbToLmtSyncInfoQueryAck_t *pStr =
				(wrFLEnbToLmtSyncInfoQueryAck_t *)psk->pkg;
			if(!(pStr->u16RemMode)) {
				printf("empty sync type");
			} else {
				printf("GPS sync type");
			}
			/*
			 * 0:GPS 同步成功;
			 * 1:空口同步成功;
			 * 2:未同步;
			 * */
			write_action_log(client_ip,
				"get base station sync info response!");
#ifdef BIG_END
			pStr->u16RemMode = my_htons_ntohs(pStr->u16RemMode);
			pStr->u16SyncState = my_htons_ntohs(pStr->u16SyncState);
#endif
			switch(pStr->u16SyncState) {
			case GPS_SYNC_OK:
				printf("<--- GPS sync success\n");
				write_action_log("response:",
					"GPS sync success!");
				send_generic_request_to_baseband(client_ip,
					O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY);
				break;
			case SNIFF_SYNC_OK:
				printf("<--- empty sync success\n");
				write_action_log("response",
					"empty sync success!");
				break;
			case SYNC_FAIL:
				printf("<--- not sync\n");
				write_action_log("response",
					"not sync success!");
				break;
			default:
				printf("\n");
				break;
			}
			pthread_mutex_lock(&(entry->lock));
			entry->u16RemMode = pStr->u16RemMode;
			entry->u16SyncState = pStr->u16SyncState;
			pthread_mutex_unlock(&(entry->lock));

			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddNumberToObject(object, "remmode", pStr->u16RemMode);
			cJSON_AddNumberToObject(object, "sync_state", pStr->u16SyncState);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				printf("Assemble sync info query str error!\n");
				break;
			}
			printf("sync info query send str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.sync_api_arg, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_CELL_STATE_INFO_QUERY_ACK://3.45. 小区状态信息查询应答
		{
			printf("<---- 3.45. cell status info query response\n");
			write_action_log(client_ip,
				"get current status response!");
			wrFLEnbToLmtCellStateInfoQureyAck_t *pStr =
				(wrFLEnbToLmtCellStateInfoQureyAck_t *)psk->pkg;
			char state[2] = {0};
			state[0] = '0';
#ifdef BIG_END
			pStr->u32CellState = my_htonl_ntohl(pStr->u32CellState);
#endif
			switch(pStr->u32CellState) {
			case IDLE_STATE:
				printf("<----current status: Idle \n");
				write_action_log("current status:", "Idle!");
				break;
			case REM_STATE:
				printf("<----current status:REM running\n");
				write_action_log("current status:", "running!");
				break;
			case CELL_READY_ACTIVE:
				printf("<----current status:cell activating...\n");
				write_action_log("current status:", "activating!");
				break;
			case CELL_ACTIVE_YET:
				printf("<----current status:cell activated\n");
				write_action_log("current status:", "activated!");
				state[0] = '1';
				break;
			case CELL_READY_DEACTIVE:
				printf("<----current status:cell deactivating...\n");
				write_action_log("current status:","deactivating!");
				break;
			default:
				printf("<----current status:nonsupport\n");
				write_action_log("current status:","nonsupport!");
				break;
			}
			pthread_mutex_lock(&(entry->lock));
			entry->cellstate = pStr->u32CellState;
			pthread_mutex_unlock(&(entry->lock));
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddStringToObject(object, "work_admin_state", state);
			S8 *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) break;
			printf("cell status query str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.cellstat_api_arg, send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_RXGAIN_POWER_DEREASE_QUERY_ACK:
		{
			printf("<----gain and transmit power query response\n");
			wrFLEnbToLmtEnbRxGainPowerDereaseQueryAck_t *pStr =
				(wrFLEnbToLmtEnbRxGainPowerDereaseQueryAck_t *)psk->pkg;
			pthread_mutex_lock(&(entry->lock));
			entry->regain = pStr->u8RxGainValueFromMib;
			entry->powerderease = pStr->u8PowerDereaseValueFromMib;
			pthread_mutex_unlock(&(entry->lock));

			char rx_power_str[256] = {0};
			snprintf(rx_power_str, 256, 
				"{\"topic\":\"%s\",\"ip\":\"%s\","
				"\"rxgainvaluefromreg\":\"%u\","/* in reg */
				"\"rxgainvaluefrommib\":\"%u\","/* in db */
				"\"powerdereasevaluefromreg\":\"%u\","/*in reg*/
				"\"powerdereasevaluefrommib\":\"%u\","/*in db*/
				"\"agcflag\":\"%x\"}",/*AGC AC(0:close,1:open)*/
				my_topic, client_ip,
				pStr->u8RxGainValueFromReg,
				pStr->u8RxGainValueFromMib,
				pStr->u8PowerDereaseValueFromReg,
				pStr->u8PowerDereaseValueFromMib,
				pStr->u8AgcFlag);
#ifdef CLI
			char cli_resq[256] = {0};
			snprintf(cli_resq, 256, 
				"TX-PowerDerease in reg:%u\n"
				"TX-PowerDerease in  db:%u\n"
				" RX-Gain in reg:%u\n"
				" RX-Gain in  db:%u\n"
				"       ACG-flag:%s\n",
				pStr->u8PowerDereaseValueFromReg,
				pStr->u8PowerDereaseValueFromMib,
				pStr->u8RxGainValueFromReg,
				pStr->u8RxGainValueFromMib,
				(pStr->u8AgcFlag == 0)?"Close":"Open");
			cli_req_t *cli_entry =
				entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd, cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			printf("gain and transmit power query str: %s\n",
				rx_power_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.rx_power_api_arg,
				rx_power_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			break;
		}
	case O_FL_ENB_TO_LMT_ENB_IP_QUERY_ACK://3.49. 基站 IP 查询应答
		{
			printf("<--- (0xF034) base station IP query response");
			wrFLEnbToLmtEnbIPQueryAck_t *pStr =
				(wrFLEnbToLmtEnbIPQueryAck_t *)psk->pkg;
			printf("base station IP: %d.%d.%d.%d\n",
				pStr->u8EnbIp[0], pStr->u8EnbIp[1],
				pStr->u8EnbIp[2], pStr->u8EnbIp[3]);
			break;
		}
	case O_FL_ENB_TO_LMT_REM_CFG_QUERY_ACK:/* 3.55扫频参数配置查询应答 */
		{
			printf("<---scan frequency parameter query response\n");
			wrFLEnbToLmtRemCfgQueryAck_t *pStr =
				(wrFLEnbToLmtRemCfgQueryAck_t *)psk->pkg;
			int i = 0;
			cJSON *object = cJSON_CreateObject();
			if (!object) {
				printf("creat cjson object error!\n");
				break;
			}
			cJSON *array = cJSON_CreateArray();
			if (!array) {
				printf("creat cjson array error!\n");
				cJSON_Delete(object);
				break;
			}
#ifdef BIG_END
			pStr->wholeBandRem = my_htonl_ntohl(pStr->wholeBandRem);
			pStr->sysArfcnNum = my_htonl_ntohl(pStr->sysArfcnNum);
#endif
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddNumberToObject(object,	"band_rem",
				pStr->wholeBandRem);
			cJSON_AddNumberToObject(object, "arfcn_num",
				pStr->sysArfcnNum);
			cJSON_AddItemToObjectCS(object, "sys_arfcn", array);
			for(i = 0; i < pStr->sysArfcnNum; i++) {
#ifdef BIG_END
				pStr->sysArfcn[i] =
					my_htonl_ntohl(pStr->sysArfcn[i]);
#endif
				cJSON *number =
					cJSON_CreateNumber(pStr->sysArfcn[i]);
				if (!number) continue;
				cJSON_AddItemToArray(array, number);
			}
			S8 *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				printf("sysarfcn json to string error!\n");
				break;
			}
			printf("scan frequency parameter config query str:%s\n",
				send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.sysarfcn_api_arg,
				send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_SELFCFG_ARFCN_QUERY_ACK: //自配置频点列表查询回复
		{
			printf("<---- 3.88. Arfcn list query report\n");
			FLEnbToLmtSelfcfgArfcnQueryAck_t *pStr =
				(FLEnbToLmtSelfcfgArfcnQueryAck_t *)psk->pkg;
			U32 count;
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON *array = cJSON_CreateArray();
			if (!array) {
				cJSON_Delete(object);
				break;
			}
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddItemToObjectCS(object, "list", array);
#ifdef BIG_END
			pStr->DefaultArfcnNum =
				my_htonl_ntohl(pStr->DefaultArfcnNum);
#endif
			char value[4] = {0};
			if (pStr->DefaultArfcnNum > 0) {
				for (count = 0; count < pStr->DefaultArfcnNum; count++) {
#ifdef BIG_END
					pStr->ArfcnValue[count] =
						my_htonl_ntohl(pStr->ArfcnValue[count]);
#endif
					snprintf(value, 4, "%u", pStr->ArfcnValue[count]);
					cJSON *t = cJSON_CreateString(value);
					if (t) {
						cJSON_AddItemToArray(array, t);
					}
				}
			}
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) break;
			write_action_log(client_ip,
				"Arfcn list query config response!");
			write_action_log("response", send_str);
			printf("Arfcn list config report str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.selfcfg_api_arg, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_CONTROL_LIST_QUERY_ACK://4.8. 管控名单查询应答
		{
			printf("<---- 4.8. get Control list response\n");
			wrFLEnbToLmtControlListQueryAck_t *pStr =
				(wrFLEnbToLmtControlListQueryAck_t *)psk->pkg;
			if(pStr->ControlListProperty == 0) {
				printf("blacklist mode\n");
			} else if(pStr->ControlListProperty == 1) {
				printf("white list mode\n");
			} else {
				printf("none Control list\n");
				break;
			}
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON *array = cJSON_CreateArray();
			if (!array) {
				printf("in control list init array failed!\n");
				cJSON_Delete(object);
				break;
			}
			cJSON_AddStringToObject(object, "topic", my_topic);
			U8 i;
			for(i = 0; i < pStr->ControlListUENum; i++) {
				printf("list[%d]:%s\n",
					i, pStr->ControlListUEId[i]);
				cJSON *t = cJSON_CreateObject();
				if (!t) continue;
				cJSON_AddStringToObject(t, "ip", client_ip);
				cJSON_AddStringToObject(t, "imsi",
					(char *)pStr->ControlListUEId[i]);
				cJSON_AddItemToArray(array, t);
			}
			cJSON_AddItemToObjectCS(object, "clients", array);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				printf("control list json to string failed!\n");
				break;
			}
			write_action_log(client_ip,
				"get control list response!");
			write_action_log("response", send_str);
			printf("Control list query str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.black_list_api_arg,	send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_REM_INFO_RPT: //扫频/同步结果上报
		{
			printf("<----scan frequency/synchronous info report\n");
			wrFLEnbToLmtScanCellInfoRpt_t *pStr =
				calloc(1, sizeof(wrFLEnbToLmtScanCellInfoRpt_t));
			if (!pStr) break;
			int num = 12 + 4; /* sizeof header + 2 * U16 */
			memcpy(pStr, psk->pkg, num);
			int i, i2, i3;
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON *array = cJSON_CreateArray();
			if (!array) {
				cJSON_Delete(object);
				break;
			}
			cJSON_AddStringToObject(object,	"ip", client_ip);
			cJSON_AddNumberToObject(object,	"collectioncellnum",
				pStr->collectionCellNum);
			cJSON_AddNumberToObject(object,	"collectiontypeflag",
				pStr->collectionEndFlag);
			cJSON_AddItemToObjectCS(object, "stcollcellinfo", array);
			for (i = 0; i < pStr->collectionCellNum; i++) {
				memcpy(&(pStr->stCollCellInfo[i].stCellInfo),
					&psk->pkg[num], sizeof(wrFLCellInfo_t));
				num = num + sizeof(wrFLCellInfo_t);
				memcpy(&(pStr->stCollCellInfo[i].NeighNum),
					&psk->pkg[num], 4);
				num = num + 4;
				cJSON *stcollcellinfo_obj = cJSON_CreateObject();
				if (!stcollcellinfo_obj) {
					continue;
				}
				cJSON_AddItemToArray(array, stcollcellinfo_obj);
				cJSON *stcellinfo_obj = cJSON_CreateObject();
				if (!stcellinfo_obj) {
					continue;
				}
				cJSON_AddNumberToObject(stcellinfo_obj,	"arfcn",
					(int)(pStr->stCollCellInfo[i].stCellInfo.Arfcn));
				cJSON_AddNumberToObject(stcellinfo_obj,	"pci",
					(int)(pStr->stCollCellInfo[i].stCellInfo.pci));
				cJSON_AddNumberToObject(stcellinfo_obj,	"tac",
					(int)(pStr->stCollCellInfo[i].stCellInfo.Tac));
				cJSON_AddNumberToObject(stcellinfo_obj,	"rssi",
					(int)(pStr->stCollCellInfo[i].stCellInfo.Rssi));
				cJSON_AddNumberToObject(stcellinfo_obj,
					"sfassign",
					(int)(pStr->stCollCellInfo[i].stCellInfo.SFassign));
				cJSON_AddNumberToObject(stcellinfo_obj,
					"cellid",
					(int)(pStr->stCollCellInfo[i].stCellInfo.cellid));
				cJSON_AddNumberToObject(stcellinfo_obj,
					"priority",
					(int)(pStr->stCollCellInfo[i].stCellInfo.Priority));
				cJSON_AddNumberToObject(stcellinfo_obj,	"rsrp",
					(int)(pStr->stCollCellInfo[i].stCellInfo.RSRP));
				cJSON_AddNumberToObject(stcellinfo_obj,
					"rsrq",
					(int)(pStr->stCollCellInfo[i].stCellInfo.RSRQ));
				cJSON_AddNumberToObject(stcellinfo_obj,
					"bandwidth",
					(int)(pStr->stCollCellInfo[i].stCellInfo.Bandwidth));
				cJSON_AddNumberToObject(stcellinfo_obj,
					"specsfassign",
					(int)(pStr->stCollCellInfo[i].stCellInfo.SpecSFassign));
				cJSON_AddItemToObjectCS(stcollcellinfo_obj,
					"stcellinfo", stcellinfo_obj);
				cJSON_AddNumberToObject(stcollcellinfo_obj,
					"neighnum", pStr->stCollCellInfo[i].NeighNum);
				memcpy(&(pStr->stCollCellInfo[i].stNeighCellInfo[0]),
					&psk->pkg[num],
					sizeof(wrFLNeighCellInfo_t) * pStr->stCollCellInfo[i].NeighNum);
				num = num + sizeof(wrFLNeighCellInfo_t) * pStr->stCollCellInfo[i].NeighNum;
				memcpy(&(pStr->stCollCellInfo[i].NumOfInterFreq), &psk->pkg[num], 4);
				num = num + 4;
				cJSON *stneighcell_array = cJSON_CreateArray();
				if (!stneighcell_array) {
					continue;
				}
				cJSON_AddItemToObjectCS(stcollcellinfo_obj,
					"wrflneighcellinfo", stneighcell_array);
				for (i2 = 0; i2 < pStr->stCollCellInfo[i].NeighNum; i2++) {
					cJSON *stneighcell_obj = cJSON_CreateObject();
					if (!stneighcell_obj) continue;
					cJSON_AddItemToArray(stneighcell_array,stneighcell_obj);
					cJSON_AddNumberToObject(stneighcell_obj,
						"arfcn",
						pStr->stCollCellInfo[i].stNeighCellInfo[i2].Arfcn);
					cJSON_AddNumberToObject(stneighcell_obj,
						"pci",
						pStr->stCollCellInfo[i].stNeighCellInfo[i2].pci);
					cJSON_AddNumberToObject(stneighcell_obj,
						"qoffsetcell",
						pStr->stCollCellInfo[i].stNeighCellInfo[i2].QoffsetCell);
				}
				cJSON_AddNumberToObject(stcollcellinfo_obj,
					"numofinterfreq", pStr->stCollCellInfo[i].NumOfInterFreq);
				cJSON * stinterfreqlst_array = cJSON_CreateArray();
				if (!stinterfreqlst_array) {
					continue;
				}
				cJSON_AddItemToObjectCS(stcollcellinfo_obj,
					"stfllteintrefreqlist", stinterfreqlst_array);
				for (i2 = 0; i2 < pStr->stCollCellInfo[i].NumOfInterFreq; i2++) {
					memcpy(&(pStr->stCollCellInfo[i].stInterFreqLstInfo[i2]),
						&psk->pkg[num], 12);
					num = num + 12;
					cJSON *stinterfreqlst_obj = cJSON_CreateObject();
					if (!stinterfreqlst_obj) continue;
					cJSON_AddItemToArray(stinterfreqlst_array,
						stinterfreqlst_obj);
					cJSON_AddNumberToObject(stinterfreqlst_obj,
						"dlarfcn",
						pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].dlARFCN);
					cJSON_AddNumberToObject(stinterfreqlst_obj,
						"cellreselectpriotry",
						pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].cellReselectPriotry);
					cJSON_AddNumberToObject(stinterfreqlst_obj,
						"q_offsetfreq",
						pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].Q_offsetFreq);
					cJSON_AddNumberToObject(stinterfreqlst_obj,
						"measBandWidth",
						pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].measBandWidth);
					cJSON_AddNumberToObject(stinterfreqlst_obj,
						"interfreqnghnum",
						pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].interFreqNghNum);
					memcpy(&(pStr->stCollCellInfo[i].stInterFreqLstInfo[i2]),
						&psk->pkg[num],
						sizeof(wrFLInterNeighCellInfo_t) * pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].interFreqNghNum);
					num = num + sizeof(wrFLInterNeighCellInfo_t) * pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].interFreqNghNum;
					cJSON *stinterfreq_array = cJSON_CreateArray();
					if (!stinterfreq_array) {
						continue;
					}
					for (i3 = 0; i3 < pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].interFreqNghNum; i3++) {
						cJSON *stinterfreq_obj = cJSON_CreateObject();
						if (!stinterfreq_obj) continue;
						cJSON_AddItemToArray(stinterfreq_array,
							stinterfreq_obj);
						cJSON_AddNumberToObject(stinterfreq_obj,
							"pci",
							pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].stInterFreq[i3].pci);
						cJSON_AddNumberToObject(stinterfreq_obj,
							"qoffsetcell",
							pStr->stCollCellInfo[i].stInterFreqLstInfo[i2].stInterFreq[i3].QoffsetCell);
					} /* for (i3 = 0... */
				} /* for (i2 = 0;... */
			} /* for (i = 0; i < pStr->collectionCellNum; i++) */
			S8 *send_str = cJSON_PrintUnformatted(object);
			if (!send_str) break;
			printf("scan frequency/synchronous info str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.rem_result_api_arg,	send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);

			if(strncmp(client_ip, BAND_38_IPADDR, 12) != 0) {
				cJSON_Delete(object);
				free(send_str);
				break;
			}
#if 0
			FILE *scan_fp;
			scan_fp = fopen(SCAN_LTE_FILE, "a+");
			char now_time[32] = {0};
			time_t rawtime;
			struct tm * timeinfo;
			rawtime = time(NULL);
			timeinfo = localtime(&rawtime);
			snprintf(now_time, 20,
				"%04d-%02d-%02d %02d:%02d:%02d",
				timeinfo->tm_year + 1900,
				timeinfo->tm_mon +1, timeinfo->tm_mday,
				timeinfo->tm_hour, timeinfo->tm_min,
				timeinfo->tm_sec);
			if(scan_fp == NULL) {
				printf("open lte scan file err!\n");
			} else {
				fputs(now_time, scan_fp);
				fputc('\n', scan_fp);
				fputs(cJSON_Print(object), scan_fp);
				fputc('\n', scan_fp);
				fputc('\n', scan_fp);
				fputc('\n', scan_fp);
				fclose(scan_fp);
			}
#endif
			pthread_mutex_lock(&entry->lock);
			entry->scan_frequency_flag = 2;
			pthread_mutex_unlock(&entry->lock);
		//	scan_fre_com_config(entry, 0);
			cJSON_Delete(object);
			write_action_log(client_ip,
				"3.7 get scan frequency/synchronous info response");
			write_action_log(client_ip, send_str);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_MEAS_UE_CFG_QUERY_ACK://4.4. F03E UE测量查询应答
		{
			wrFLLmtToEnbMeasUecfg_t *pStr =
				(wrFLLmtToEnbMeasUecfg_t *)psk->pkg;
#ifdef BIG_END
			pStr->u16CapturePeriod = my_htons_ntohs(pStr->u16CapturePeriod);
#endif
			printf("<---- 4.4 UE config query response\n");
			char send_str[320] = {0};
			write_action_log(client_ip,
				"get UE config query response!");
			snprintf(send_str, 320,
				"{\"topic\":\"%s\",\"ip\":\"%s\","
				"\"work_mode\":\"%u\","
				"\"sub_mode\":\"%u\","
				"\"capture_period\":\"%u\","
				"\"control_submode\":\"%u\","
				"\"dst_imsi\":\"%s\","
				"\"report_itv\":\"%u\","
				"\"max_txflag\":\"%u\","
				"\"ue_max_txpower\":\"%u\","
				"\"posi_flag\":\"%u\","
				"\"cap_flag\":\"%u\","
				"\"control_submode\":\"%u\"}",
				my_topic, client_ip,
				pStr->u8WorkMode,
				pStr->u8SubMode,
				pStr->u16CapturePeriod,
				pStr->u8ControlSubMode,
				pStr->IMSI,
				pStr->u8MeasReportPeriod,
				pStr->SchdUeMaxPowerTxFlag,
				pStr->SchdUeMaxPowerValue,
				pStr->SchdUeUlFixedPrbSwitch,
				pStr->CampOnAllowedFlag,
				pStr->u8ControlSubMode);

			write_action_log("UE config status:", send_str);
			printf("UE config query str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.uesend_api_arg, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			break;
		}
	case O_FL_ENB_TO_LMT_MEAS_INFO_RPT://UE测量值上报
		{
			wrFLEnbToLmtMeasInfoRpt_t *pStr =
				(wrFLEnbToLmtMeasInfoRpt_t *)psk->pkg;
			U8 ts8[2] = {0};
			memcpy(ts8, &(pStr->TA), 2);
#ifdef BIG_END
			pStr->TA = my_htons_ntohs(pStr->TA);
#endif
			S8 uerep_str[128] = {0};
			snprintf(uerep_str, 128,
				"{\"IMSI\":\"%s\","
				" \"UE\":\"%u\","
				"\"TA\":\"%u\","
				"\"prover\":\"%s\"}",
				pStr->IMSI, pStr->UeMeasValue,
				pStr->TA, (!ts8[1])?"R8":"R10");
			printf("UE:report\n%s\n", uerep_str);
			send_ue_string_udp(uerep_str, strlen(uerep_str));
			break;
		}
	case O_FL_ENB_TO_LMT_REDIRECT_INFO_CFG_QUERY_ACK://3.55 重定向查询应答
		{
			printf("<---- 3.55 redirect config query response\n");
			write_action_log(client_ip,
				"get redirect config query response");
			wrFLEnbToLmtRedirectInfoCfgQueryAck_t *pStr =
				(wrFLEnbToLmtRedirectInfoCfgQueryAck_t *)psk->pkg;
#ifdef BIG_END
			pStr->OnOff = my_htonl_ntohl(pStr->OnOff); 
			pStr->EARFCN = my_htonl_ntohl(pStr->EARFCN); 
			pStr->RedirectType
				= my_htonl_ntohl(pStr->RedirectType);
#endif
			// OnOff  是否开启重定向功能开关:0:打开,1:关闭
			// EARFCN  重定向频点
			// RedirectType 重定向类型 0:4G;1:3G;2:2G
			char redir_str[128] = {0};
			snprintf(redir_str, 128,
				"{\"topic\":\"%s\",\"ip\":\"%s\","
				"\"onoff\":\"%u\","
				"\"earfcn\":\"%u\","
				"\"redirect_type\":\"%u\"}",
				my_topic, client_ip,
				pStr->OnOff,
				pStr->EARFCN,
				pStr->RedirectType);
			write_action_log("redirect config status:", redir_str);
			printf("redirect config query str:%s\n", redir_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.redirect_api_arg, redir_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			break;
		}
	case O_FL_ENB_TO_LMT_GSP1PPS_QUERY_ACK: //3.84. GPS 同步偏移量查询应答
		{
			printf("<----GPS sync offset query response\n");
			FLEnbToLmtGps1ppsQueryAck_t *pStr =
				(FLEnbToLmtGps1ppsQueryAck_t *)psk->pkg;
#ifdef BIG_END
			pStr->Gpsppsls = my_htonl_ntohl(pStr->Gpsppsls);
#endif
			printf("band(IP:%s),sync offset:%d\n",
				client_ip, pStr->Gpsppsls);
			cJSON *object = cJSON_CreateObject();
			if (object == NULL) {
				break;
			}
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddNumberToObject(object, "gpsppsls", pStr->Gpsppsls);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				break;
			}
			write_action_log(client_ip,
				"get GPS sync offset query response");
			write_action_log("GPS sync offset:", send_str);
			printf("GPS sync offset info str: %s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.gpsppsls_api_arg, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_ALARMING_TYPE_IND:
		{
			printf("<---- 3.72 band warning ind\n");
			wrFLEnbToLmtAlarmTypeInd_t *pStr =
				(wrFLEnbToLmtAlarmTypeInd_t *)psk->pkg;
			if (pStr->IndFlag == 1) {
				printf("band(IP:%s)generate", client_ip);
			} else {
				printf("band(IP:%s)revoke", client_ip);
			}
#ifdef BIG_END
			pStr->AlarmingType = my_htonl_ntohl(pStr->AlarmingType);
			pStr->IndFlag = my_htonl_ntohl(pStr->IndFlag);
#endif
			switch(pStr->AlarmingType) {
			case 0:
				printf("temperature too high warning\n");
				write_action_log(client_ip,
					"WARNING:temperature too high");
				break;
			case 1:
				printf("out-of-step warning\n");
				write_action_log(client_ip,
					"WARNING:out-of-step");
				break;
			case 2:
				printf("base station cell not applicable warning\n");
				write_action_log(client_ip,
					"WARNING:base station cell not applicable");
				break;
			case 3:
				printf("clock source synchronous warning\n");
				write_action_log(client_ip,
					"WARNING:clock source synchronous");
				break;
			case 4:
				printf("standing-wave ratio too high warning\n");
				write_action_log(client_ip,
					"WARNING:standing-wave ratio too high");
				break;
			case 5:
				printf("temperature too low warning\n");
				write_action_log(client_ip,
					"WARNING:temperature too low ");
				break;
			}
			cJSON *object = cJSON_CreateObject();
			if (object == NULL) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddNumberToObject(object, "ind_flag", pStr->IndFlag);
			cJSON_AddNumberToObject(object, "alarming_type", pStr->AlarmingType);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				printf("warning info!json to string failed!\n");
				break;
			}
			printf("warning info report str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.warning_api_arg, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_GPS_LOCATION_QUERY_ACK:// 3.72 GPS经纬高度查询应答
		{
			printf("<---- GPS site query response\n");
			int xx = 0;
			for (xx = 0; xx < sizeof(wrFLEnbToLmtGPSLocationQueryAck_t); xx++) {
				printf("%x ", psk->pkg[xx]);
			}
			printf("\n");
			wrFLEnbToLmtGPSLocationQueryAck_t *pStr =
				(wrFLEnbToLmtGPSLocationQueryAck_t *)psk->pkg;
#ifdef BIG_END
			my_btol_ltob((void *)&(pStr->Longitude), 8);
			my_btol_ltob((void *)&(pStr->Latitude), 8); 
			my_btol_ltob((void *)&(pStr->Altitude), 8); 
			pStr->RateOfPro = my_htonl_ntohl(pStr->RateOfPro);
#endif
#ifdef CLI
			S8 cli_resq[256] = {0};
			snprintf(cli_resq, 256,
				"Latitude  is %02.10lf\n"
				"Longitude is %02.10lf\n"
				"Altitude  is %02.2lf\n"
				"RateOfPro is %u%%",
				pStr->Longitude, pStr->Latitude,
				pStr->Altitude,	pStr->RateOfPro);
			cli_req_t *cli_entry
				= entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd, cli_resq,
					strlen((S8 *)cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			S8 send_str[256] = {0};
			snprintf(send_str, 256,
				"{\"topic\":\"%s\",\"ip\":\"%s\","
				"\"longitude\":\"%02.10lf\","
				"\"latitude\":\"%02.10lf\","
				"\"altitude\":\"%02.2lf\","
				"\"rate_of_pro\":\"%u\"}",
				my_topic, client_ip,
				pStr->Longitude, pStr->Latitude,
				pStr->Altitude,	pStr->RateOfPro
				);
			if (pStr->Longitude > 0 && pStr->Latitude > 0) {
				my_longitude = pStr->Longitude;
				my_latitude = pStr->Latitude;
				my_altitude = pStr->Altitude;
			}
			printf(" GPS info str: %s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.post_gps_api_arg, send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			break;
		}
	case O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_QUERY_ACK:
		{
			/* 3.76 位置更新拒绝原因值配置查询应答 */
			FLLmtToEnbTauAttachRejectCauseQueryAck_t *pStr =
				(FLLmtToEnbTauAttachRejectCauseQueryAck_t *)psk->pkg;
			printf("Location Update refusal cause query response\n");
#ifdef BIG_END
			pStr->U32RejectCause = my_htonl_ntohl(pStr->U32RejectCause);
#endif
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddNumberToObject(object, "reject_cause",
				pStr->U32RejectCause);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				printf("json to string error!\n");
				break;
			}
			printf("Location Update refusal cause send str:%s\n",
				send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.rejectcause_api_arg, send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			char cli_resq[128] = {0};
			switch (pStr->U32RejectCause) {
			case 0:
				snprintf(cli_resq, 128,
					"Location Update refusal No.15,"
					"Access is not allowed in tracking area");
				break;
			case 1:
				snprintf(cli_resq, 128,
					"Location Update refusal No.12,"
					"There is no suitable area in tracking area");
				break;
			case 2:
				snprintf(cli_resq, 128,
					"Location Update refusal No.3,"
					"Invalid terminal");
				break;
			case 3:
				snprintf(cli_resq, 128,
					"Location Update refusal No.13");
				break;
			case 4:
				snprintf(cli_resq, 128,
					"Location Update refusal No.22");
				break;
			default:
				snprintf(cli_resq, 128,
					"Location Update refusal No.-1"
					"unknow !");
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_SECONDARY_PLMNS_QUERY_ACK:// 7.10 辅PLMN 列表查询上报
		{
			printf("<---- PLMN list query report\n");
			wrEnbToLmtSecondaryPlmnsQueryAck_t *pStr =
				(wrEnbToLmtSecondaryPlmnsQueryAck_t *)psk->pkg;
			char plmn_num[2] = {0};
			snprintf(plmn_num, 2, "%d", (int)pStr->u8SecPLMNNum);

			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON *array = cJSON_CreateArray();
			if (!array) {
				cJSON_Delete(object);
				break;
			}
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddStringToObject(object, "plmn_num", plmn_num);
			cJSON_AddItemToObjectCS(object, "list", array);
			U8 count = 0, offset = 0;
			char cli_resq[128] = {0};
			offset += sprintf(&cli_resq[offset],
				"(IP:%s) has %d assist plmn, is:",
				client_ip, (int)pStr->u8SecPLMNNum);
			if (pStr->u8SecPLMNNum > 0) {
				for (count = 0; count < pStr->u8SecPLMNNum; count++) {
					cJSON *p = cJSON_CreateString((char *)pStr->u8SecPLMNList[count]);
					if (p) cJSON_AddItemToArray(array, p);
					offset += sprintf(&(cli_resq[offset]),
						"\"%s\"",
						(char *)pStr->u8SecPLMNList[count]);
				}
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry =
				entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd, cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) break;

			printf("Assist PLMN info report str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.plmninfo_api_arg, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case O_FL_ENB_TO_LMT_RA_ACCESS_QUERY_ACK://7.2 随机接入成功率上报
		{
			printf("<---- 7.2 random access success rate report\n");
			wrFLEnbToLmtRaAccessQueryAck_t *pStr =
				(wrFLEnbToLmtRaAccessQueryAck_t *)psk->pkg;
#ifdef BIG_END
			pStr->RrcConnReqNum = my_htonl_ntohl(pStr->RrcConnReqNum);
			pStr->RrcConnCmpNum = my_htonl_ntohl(pStr->RrcConnCmpNum);
#endif
			char cli_resq[256] = {0};
			snprintf(cli_resq, 256,
				"band(IP:%s), random access success rate info:\n"
				"RrcConnectRequestNum: %u\n"
				"RrcConnetCompleteNum: %u\n"
				"Proportion of random access: %.2f%%"
				, client_ip,
				pStr->RrcConnReqNum, pStr->RrcConnCmpNum,
				(pStr->RrcConnReqNum == 0)?
				0:(float)pStr->RrcConnCmpNum / (float)pStr->RrcConnReqNum * 100);
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", client_ip);
			cJSON_AddNumberToObject(object,"rrc_conn_reqnum",
				pStr->RrcConnReqNum);
			cJSON_AddNumberToObject(object,"rrc_conn_cmpnum",
				pStr->RrcConnCmpNum);
			cJSON_AddNumberToObject(object,"spare1", pStr->Spare1);
			cJSON_AddNumberToObject(object,"spare2", pStr->Spare2);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				break;
			}
			printf("random access success rate str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.rrcconnum_api_arg, send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	default:
		printf("---->MsgType[%x] was not supported\n",msg_type);
	}
free_return:
	free(psk->pkg);
	free(psk->ip);
	free(psk);
	return NULL;
}
/* 解析从baicell基带板发来的数据包 */
void baicell_pare_recv_value(U8 *recBuf, band_entry_t *entry)
{
	U16 msg_type = recBuf[4] | (recBuf[5] << 8);
	U16 msg_len = recBuf[6] | (recBuf[7] << 8);
	if (msg_type <= 0) return;
	S8 *client_ip = entry->ipaddr;

	/*  init entry that talk with cli program */
	struct cli_req_if cli_info;
	snprintf((S8 *)(cli_info.ipaddr), 20, "%s", client_ip);
	cli_info.msgtype = msg_type;
	cli_info.sockfd = -1;
	printf("-------------msgtype:%X\n", msg_type);
	switch (msg_type) {
	case O_FL_ENB_TO_LMT_UE_INFO_RPT:
		{/* IMSI up */
			imsi_list_t *point = calloc(1, sizeof(imsi_list_t));
			if (!point) return;
			point->rtime = time(NULL);
			point->ip = strdup(client_ip);
			memcpy(&(point->ptr), recBuf,
				sizeof(wrFLEnbToLmtUeInfoRpt_t));
			safe_pthread_create(thread_add_point_to_imsi_list,
				(void *)point);
			break;
		} 
	case O_FL_ENB_TO_LMT_SYS_INIT_SUCC_IND:
		{/* Heartbeat */
			wrFLEnbToLmtSysInitInformInd_t *ptr =
				(wrFLEnbToLmtSysInitInformInd_t *)recBuf;
			ptr->WrmsgHeaderInfo.u16MsgType =
				O_FL_LMT_TO_ENB_SYS_INIT_SUCC_RSP;
			send_generic_request_to_baseband(client_ip,
				O_FL_LMT_TO_ENB_SYS_INIT_SUCC_RSP);
			entry->change_count++;
			entry->_auto++;
			if (entry->_auto == 4) {
				printf("send request ALL\n");
				safe_pthread_create(thread_send_generic_request_to_bc,
					(void *)(client_ip));
			} else if (entry->_auto >= 3 && !(entry->_auto % 20)) {
				printf("send request any\n");
				send_generic_request_to_baseband(client_ip,
					O_FL_LMT_TO_ENB_BASE_INFO_QUERY);
				send_generic_request_to_baseband(client_ip,
					O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY);
			}
			break;
		}
	case O_FL_ENB_TO_LMT_SYS_MODE_ACK:
		{
			printf("<---- system work mode config response\n");
			wrFLEnbToLmtSysModeAck_t *pStr
				= (wrFLEnbToLmtSysModeAck_t *)recBuf;
			if(pStr->CfgResult == 0) {
				printf("Sys mode cfg Success!\n");
				write_action_log(client_ip,
					"system work mode config success!");
			} else {
				printf("Sys mode cfg failed with 0x%x\n",
					pStr->CfgResult);
				write_action_log(client_ip,
					"system work mode config failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_SYS_ARFCN_ACK: /* 频点配置应答 */
		{
			printf("<----Arfcn config response\n");
			wrFLEnbToLmtSysArfcnAck_t *pStr =
				(wrFLEnbToLmtSysArfcnAck_t *)recBuf;
			S8 cli_resq[128] = {0};
			if(pStr->CfgResult == 0) {
				snprintf(cli_resq, 128,
					"set baseBand(IP:%s) Arfcn Success!",
					client_ip);
				write_action_log(client_ip,
					"arfcn config success!");
				/* 频点配置成功后要重启基带板 */
				send_generic_request_to_baseband(client_ip,
					O_FL_LMT_TO_ENB_REBOOT_CFG);
			} else {
				snprintf(cli_resq, 128,
					"set baseBand(IP:%s) Arfcn Failed!",
					client_ip);
				write_action_log(client_ip,
					"arfcn config failed!");
#if 0
				if(entry->scan_frequency_flag == 1) {
					scan_fre_com_config(entry, 0);
					cell_up_down_config(entry, 1);
					pthread_mutex_lock(&entry->lock);
					entry->scan_frequency_flag = 0;
					pthread_mutex_unlock(&entry->lock);
				}
#endif
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry =
				entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd, cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_SELF_ACTIVE_CFG_PWR_ON_ACK://3.39.上电自动建小区配置应答
		{
			printf("<---- automatic build cell config response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (pStr->CfgResult == 0) {
				printf("band(IP:%s)automatic build cell"
					" config success!\n",
					client_ip);
				write_action_log(client_ip,
					"automatic build cell config success");
			} else {
				printf("band(IP:%s)automatic build cell"
					" config failed!"
					"CfgResult:%u\n",
					client_ip, pStr->CfgResult);
				write_action_log(client_ip,
					"automatic build cell config failed");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_MEAS_UE_ACK://4.2. 测量 UE 配置应答
		{
			printf("<----4.2. UE config response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if(!(pStr->CfgResult)) {
				printf("UE config success!\n");
				write_action_log(client_ip,
					"UE config success!");
			} else {
				printf("UE config error!cfgResult:%d\n",
					pStr->CfgResult);
				write_action_log(client_ip,
					"UE config failed!");
			}
			break;
		}

	case O_FL_ENB_TO_LMT_UPDATE_SOFT_VERSION_CFG_ACK:/* 基带板升级回复 */
		{
			printf("<---- 3.80 base station update response\n");
			wrFLEnbToLmtUpdateSoftVersionCfgAck_t *pStr =
				(wrFLEnbToLmtUpdateSoftVersionCfgAck_t *)recBuf;
			if (!(pStr->CfgResult)) {
				printf("band(IP:%s)version update success!\n",
					client_ip);
				write_action_log(client_ip,
					"base dev version update success!");
			} else {
				printf("band(IP:%s)version update error!%s\n",
					client_ip, (char *)(pStr->failCause));
				write_action_log(client_ip,
					"base dev version update failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_REBOOT_ACK:/* 基带板重启请求应答 */
		{
			printf("<---- restart response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			char cli_resq[128] = {0};
			if(!(pStr->CfgResult)) {
				snprintf(cli_resq, 128,
					"band(IP:%s)restart success!\n",
					client_ip);
				write_action_log(client_ip, "restart success!");
				/* restart success! band dev will reboot !
				 * so set it offline */
				entry->online = 0;
				entry->change_count = -10;
			} else {
				snprintf(cli_resq, 128,
					"band(IP:%s)restart failed!\n",
					client_ip);
				write_action_log(client_ip, "restart failed!");
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry =
				entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd, cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_SET_ADMIN_STATE_ACK: /* 小区激活/去激活回复 */
		{
			printf("<----3.11 set cell [de]active response!\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			char cli_resq[128] = {0};
			if(!(pStr->CfgResult)) {
				snprintf(cli_resq, 128,
					"set band(IP:%s) Activation Success!",
					client_ip);
				write_action_log(client_ip,
					"cell activation success!");
				if(entry->scan_frequency_flag == 2) {
					pthread_mutex_lock(&entry->lock);
					entry->scan_frequency_flag = 0;
					pthread_mutex_unlock(&entry->lock);
				}
			} else {
				snprintf(cli_resq, 128,
					"set band(IP:%s) Activation Failed!",
					client_ip);
				write_action_log(client_ip,
					"cell activation Failed!");
				if(entry->scan_frequency_flag == 1 ||
					entry->scan_frequency_flag == 2){
					pthread_mutex_lock(&entry->lock);
					entry->scan_frequency_flag = 0;
					pthread_mutex_unlock(&entry->lock);
				}
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_SYS_LOG_LEVL_SET_ACK:
		{
			printf("<---- 3.61. set log print level response!\n");
			wrFLSetSysLogLevelInfoAck_t *pStr =
				(wrFLSetSysLogLevelInfoAck_t *)recBuf;
			printf("band(IP:%s)", client_ip);
			write_action_log(client_ip,
				"set log print level response!");
			if (!(pStr->isSetStkLogLevOk)) {
				printf("Set stk log success!\n");
				write_action_log("response:",
					"Set stk log success!");
			} else {
				printf("Set stk log failed!\n");
				write_action_log("response:",
					"Set stk log failed!");
			}
			if (!(pStr->isSetDbgLogLevOk)) {
				printf("Set dbg log success!\n");
				write_action_log("response:",
					"Set dbg log success!");
			} else {
				printf("Set dbg log failed!\n");
				write_action_log("response:",
					"Set dbg log failed!");
			}
			if (!(pStr->isSetOamLogLevOk)) {
				printf("Set oma log success!\n");
				write_action_log("response:",
					"Set oma Log success!");
			} else {
				printf("Set oam log failed!\n");
				write_action_log("response:",
					"Set oma log failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_SYS_LOG_LEVL_QUERY_ACK:
		{
			printf("<---- 3.63. get Log print level response\n");
			wrFLSysLogLevelInfoAck_t *pStr =
				(wrFLSysLogLevelInfoAck_t *)recBuf; 
			write_action_log(client_ip,
				"get log print level response!");
			if (pStr->queryResult == 1) {
				char tmp_b[128] = {0};
				snprintf(tmp_b, 128,
					"get Log print level success!"
					"stk level:%x, dbg level:%x,"
					" oam level:%x",
					pStr->oamLogLev, pStr->stkLogLevel,
					pStr->dbgLogLevel);
				printf("%s\n", tmp_b);
				write_action_log("level:", tmp_b);
			} else {
				printf("get log print level failed!\n");
				write_action_log("level",
					"get log print level failed");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_TDD_SUBFRAME_ASSIGNMENT_SET_ACK:
		{
			printf("<---- 3.65 set TDD subframe config response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (pStr->CfgResult) {
				printf("band(IP:%s)config failed!\n",
					client_ip);
				write_action_log(client_ip,
					"3.65 TDD subframe config failed!");
				break;
			}
			printf("band(IP:%s)config success!"
				"send restart request!\n", client_ip);
			write_action_log(client_ip,
				"3.65 TDD subframe config success!");
			/* TDD 子帧配置成功后要重启基带板 */
			send_generic_request_to_baseband(client_ip,
				O_FL_LMT_TO_ENB_REBOOT_CFG);
			break;
		}

	case O_FL_ENB_TO_LMT_FREQ_OFFSET_CFG_ACK://初始频偏配置应答
		{
			printf("<---- 3.71. initial frequency offset config ACK!\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if(!(pStr->CfgResult)) {
				printf("dev(IP:%s)initial frequency offset config success!\n",
					client_ip);
				write_action_log(client_ip,
					"set initial frequency offset config success!");
			} else {
				printf("dev(IP:%s)initial frequency offset config failed!\n",
					client_ip);
				write_action_log(client_ip,
					"set initial frequency offset config failed!");
			}
			break;
		}

	case O_FL_LMT_TO_ENB_SYNCARFCN_CFG_ACK:
		{
			printf("<---- 3.74 diff-frequency frequency point and band config response\n");
			FLLmtToEnbSyncarfcnCfgAck_t *pStr =
				(FLLmtToEnbSyncarfcnCfgAck_t *)recBuf;
			if(!(pStr->CfgResult)) {
				printf("band(IP:%s)diff-frequency frequency point and band config success!\n",
					client_ip);
				write_action_log(client_ip,
					"3.74 diff-frequency frequency point and band config success!");
			} else {
				printf("band(IP:%s)diff-frequency frequency point and band config failed!\n",
					client_ip);
				write_action_log(client_ip,
					"3.74 diff-frequency frequency point and band config failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_SYS_RxGAIN_ACK:/* 接收增益配置应答 */
		{
			printf("<---- 3.15. Receive gain config ACK!\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			char cli_resq[128] = {0};
			if(!(pStr->CfgResult)) {
				snprintf(cli_resq, 128,
					"set base band(IP:%s)Rxgain Success!",
					client_ip);
				write_action_log(client_ip,
					"3.15 Rxgain config success!");
			} else {
				snprintf(cli_resq, 128,
					"set base band(IP:%s)Rxgain Failed!",
					client_ip);
				write_action_log(client_ip,
					"3.15 Rxgain config failed!");
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry =
				entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			send_generic_request_to_baseband(client_ip,
				O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY);
			break;
		}
	case O_FL_ENB_TO_LMT_SYS_PWR1_DEREASE_ACK:/* 发射功率配置应答 */
		{
			printf("<---- 3.17. TXPower config ACK!\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			char cli_resq[128] = {0};
			if(!(pStr->CfgResult)) {
				snprintf(cli_resq, 128,
					"set dev(IP:%s)Txpower Success!",
					client_ip);
				write_action_log(client_ip,
					"3.17 txpower config success!");
			} else {
				snprintf(cli_resq, 128,
					"set dev(IP:%s)Txpower Failed!",
					client_ip);
				write_action_log(client_ip,
					"3.17 txpower config failed!");
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			send_generic_request_to_baseband(client_ip,
				O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY);
			break;
		}
	case O_FL_ENB_TO_LMT_REDIRECT_INFO_ACK://重定向配置应答
		{
			printf("<----3.19. redirection config ACK!\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if(!(pStr->CfgResult)) {
				printf("set dev(IP:%s)redirection config Success!\n",
					client_ip);
				write_action_log(client_ip,
					"3.19 redirection config success!");
			} else {
				printf("set dev(IP:%s)redirection config Failed!"
					"Error Number[%d]\n",
					client_ip, pStr->CfgResult);
				write_action_log(client_ip,
					"3.19 redirection config failed!");
			}
			/* 在接收重定向配置应答后
			 * 发送查询接口获得当前的重定向信息状态查询请求
			 * */
			send_generic_request_to_baseband(client_ip,
				O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG_QUERY);
			break;
		}
	case O_FL_ENB_TO_LMT_GPS_PP1S_ACK://GPS同步偏移量配置应答
		{
			printf("<----(0xF02A) GPS Sync Offset ACK!");
			wrFLgpsPp1sCfgAck_t *pStr = (wrFLgpsPp1sCfgAck_t *)recBuf;
			if (pStr->CfgResult != 0) {
				printf("base band(IP:%s)GPS sync offset config failed!\n",
					client_ip);
				write_action_log(client_ip,
					"GPS sync offset config failed!");
			} else {
				printf("base band(IP:%s)GPS sync offset config Success!\n",
					client_ip);
			}
			write_action_log(client_ip,
				"GPS sync offset config success!");
			break;
		}
	case O_FL_ENB_TO_LMT_ENB_STATE_IND:
		{
			printf("<---- 3.21 base station info real-time report\n");
			wrFLEnbToLmtEnbStateInd_t *pStr =
				(wrFLEnbToLmtEnbStateInd_t *)recBuf;
#ifdef BIG_END
			pStr->u32FlEnbStateInd
				= my_htonl_ntohl(pStr->u32FlEnbStateInd);
#endif
			switch(pStr->u32FlEnbStateInd) {
			case WR_FL_ENB_STATE_AIR_SYNC_SUCC:
				printf("<----empty sync success\n");
				pthread_mutex_lock(&(entry->lock));
				entry->u16RemMode = 0;
				entry->u16SyncState = 1;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_AIR_SYNC_FAIL:
				printf("<----empty sync error\n");
				pthread_mutex_lock(&(entry->lock));
				entry->u16RemMode = 0;
				entry->u16SyncState = 2;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_GPS_SYNC_SUCC:
				printf("<----GPS sync success\n");
				send_generic_request_to_baseband(client_ip,
					O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY);
				pthread_mutex_lock(&(entry->lock));
				entry->u16RemMode = 1;
				entry->u16SyncState = 0;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_GPS_SYNC_FAIL:
				printf("<----GPS sync error\n");
				pthread_mutex_lock(&(entry->lock));
				entry->u16RemMode = 1;
				entry->u16SyncState = 2;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_SCAN_SUCC:
				printf("<----scan frequency success\n");
				break;
			case WR_FL_ENB_STATE_SCAN_FAIL:
				printf("<----scan frequency error\n");
				break;
			case WR_FL_ENB_STATE_CELL_SETUP_SUCC:
				printf("<----cell build success\n");
				pthread_mutex_lock(&(entry->lock));
				entry->cellstate = 3;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_CELL_SETUP_FAIL:
				printf("<----cell build error\n");
				pthread_mutex_lock(&(entry->lock));
				entry->cellstate = 4;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_CELL_INACTIVE:
				printf("<----cell deactivation\n");
				pthread_mutex_lock(&(entry->lock));
				entry->cellstate = 4;
				pthread_mutex_unlock(&(entry->lock));

//				if(!strncmp(client_ip, BAND_38_IPADDR, 12)) {
//					if(entry->scan_frequency_flag == 1)
//						scan_fre_com_config(entry, 1);
//				}
				break;
			case WR_FL_ENB_STATE_AIR_SYNC_ON_GOING:
				printf("<----empty sync ...\n");
				pthread_mutex_lock(&(entry->lock));
				entry->u16RemMode = 0;
				entry->u16SyncState = 2;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_GPS_SYNC_ON_GOING:
				printf("<----GPS sync...\n");
				pthread_mutex_lock(&(entry->lock));
				entry->u16RemMode = 1;
				entry->u16SyncState = 2;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_SCAN_ON_GOING:
				printf("<----scan frequency...\n");
				break;
			case WR_FL_ENB_STATE_CELL_SETUP_ON_GOING:
				printf("<----build cell...\n");
				pthread_mutex_lock(&(entry->lock));
				entry->cellstate = 2;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_INACTIVE_ON_GOING:
				printf("<----cell activation...\n");
				pthread_mutex_lock(&(entry->lock));
				entry->cellstate = 2;
				pthread_mutex_unlock(&(entry->lock));
				break;
			case WR_FL_ENB_STATE_INVALID:
				printf("<----Invalid State\n");
				break;
			}
			break;
		}
	case O_FL_ENB_TO_LMT_IP_CFG_ACK:
		{
			//3.23. IP 配置应答
			printf("<---- 3.23. IP config response\n");
			wrFLEnbToLmtIpCfgAck_t *pStr
				= (wrFLEnbToLmtIpCfgAck_t *)recBuf;

			if(!pStr->CfgResult) {
				printf("<----band(IP:%s)config success!\n",
					client_ip);
				write_action_log(client_ip,
					"3.23 IP config response:success!");
				pthread_mutex_lock(&(entry->lock));
				if (entry->ipaddr && entry->changed_ip) {
					free(entry->ipaddr);
					entry->ipaddr = entry->changed_ip;
					entry->changed_ip = NULL;
				}
				pthread_mutex_unlock(&(entry->lock));
				send_generic_request_to_baseband(client_ip,
					O_FL_LMT_TO_ENB_REBOOT_CFG);
			} else {
				printf("<----band(IP:%s)config failed!"
					"CfgResult:%d\n",
					client_ip, pStr->CfgResult);
				write_action_log(client_ip,
					"3.23 IP config response:failed!");
				pthread_mutex_lock(&(entry->lock));
				if (entry->changed_ip) {
					free(entry->changed_ip);
					entry->changed_ip = NULL;
				}
				pthread_mutex_unlock(&(entry->lock));
			}
			break;
		}
	case O_FL_ENB_TO_LMT_REM_MODE_CFG_ACK: /* 基带板同步配置应答 */
		{
			printf("<----(0xF024) set base sync type response\n");
			wrFLEnbToLmtRemModeCfgAck_t *pStr =
				(wrFLEnbToLmtRemModeCfgAck_t *)recBuf;
			char cli_resq[128] = {0};
			if (!(pStr->CfgResult)) {
				snprintf(cli_resq, 128,
					"baseband sync type Success!\n");
				write_action_log(client_ip,
					"sync config success!");
			} else {
				snprintf(cli_resq, 128,
					"baseband sync type Failed!\n");
				write_action_log(client_ip,
					"sync config failed!");
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry =
				entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			send_generic_request_to_baseband(client_ip,
				O_FL_LMT_TO_ENB_SYNC_INFO_QUERY);
			break;
		}
	case O_FL_ENB_TO_LMT_SET_SYS_TMR_ACK:
		{
			printf("<--- base station dev time config response\n");
			wrFLEnbToLmtSetSysTmrAck_t *pStr =
				(wrFLEnbToLmtSetSysTmrAck_t *)recBuf;
			if (pStr->CfgResult == 0) {
				printf("set dev(IP:%s) Time Success!",
					client_ip);
				write_action_log(client_ip,
					"time config response:success!");
			} else {
				printf("set dev(IP:%s) Time Failed!",
					client_ip);
				write_action_log(client_ip,
					"time config response:failed!");
			}
			break;
		}

	case O_FL_ENB_TO_LMT_TAU_ATTACH_REJECT_CAUSE_CFG_ACK:// 3.67 位置更新拒绝原因值配置应答
		{
			printf("<---- Location Update refusal cause config response\n");
			wrFLEnbToLmtTauAttachRejectCauseCfg_t *pStr =
				(wrFLEnbToLmtTauAttachRejectCauseCfg_t *)recBuf; 
			char cli_resq[128] = {0};
			if(pStr->CfgResult == 0) {
				snprintf(cli_resq, 128,
					"Location Update refusal cause config success!\n");
			} else {
				snprintf(cli_resq, 128,
					"Location Update refusal cause config failed!\n");
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					cli_resq,
					strlen(cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_GPS_INFO_RESET_ACK: //3.78. Gps 信息复位应答
		{
			printf("<---- GPS info reset config response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (pStr->CfgResult == 0) {
				printf("band(IP:%s)GPS info reset success\n",
					client_ip);
			} else {
				printf("band(IP:%s)GPS info reset failed,"
					"error number(%u)\n",
					client_ip, pStr->CfgResult);
			}
			break;
		}
	case O_ENB_TO_LMT_GET_ENB_LOG_ACK://3.82. 获取基站Log应答
		{
			printf("<---- 3.82. get base station Log response\n");
			wrFLEnbToLmtFetEnbLogAck_t *pStr =
				(wrFLEnbToLmtFetEnbLogAck_t *)recBuf;
			if (pStr->CfgResult == 0) {
				printf("band(IP:%s) get base station Log success!\n",
					client_ip);
			} else {
				printf("band(IP:%s) get base station Log failed!%s\n",
					client_ip, pStr->failCause);
			}
			break;
		}
	case O_FL_ENB_TO_LMT_RA_ACCESS_EMPTY_REQ_ACK://7.4. 随机接入成功率清空响应
		{
			printf("<---- random access success rate empty request response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (!(pStr->CfgResult)) {
				printf("band(IP:%s)empty success\n", client_ip);
			} else {
				printf("band(IP:%s)empty error\n", client_ip);
			}
			break;
		}
	case O_FL_ENB_TO_LMT_TAC_MODIFY_REQ_ACK://7.6 TAC 手动修改配置下发应答
		{
			printf("<---- 7.6 TAC set config response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (!(pStr->CfgResult)) {
				printf("band(IP:%s)TAC set config success\n",
					client_ip);
			} else {
				printf("band(IP:%s)TAC set config error\n",
					client_ip);
			}
			break;
		}
	case O_FL_ENB_TO_LMT_SECONDARY_PLMNS_SET_ACK://7.8 辅 PLMN 配置应答
		{
			printf("<---- 7.8  PLMN list config response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			char cli_resq[64] = {0};
			if (!(pStr->CfgResult)) {
				snprintf(cli_resq, 64,
					"(IP:%s)assist PLMN config success!",
					client_ip);
			} else {
				snprintf(cli_resq, 64,
					"(IP:%s)assist PLMN config failed!",
					client_ip);
			}
			printf("%s\n", cli_resq);
#ifdef CLI
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd, cli_resq,
					strlen((S8 *)cli_resq) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_REM_ANT_CFG_ACK:  //扫频端口配置应答
		{
#if 1 
			printf("scan fre com config response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (!(pStr->CfgResult)) {
				printf("band(IP:%s)scan fre com config success\n",
					client_ip);
				if(strncmp(client_ip, BAND_38_IPADDR, 12) != 0)
					break;
				if(entry->scan_frequency_flag == 1){
					start_scan_fre(entry);
				} else if(entry->scan_frequency_flag == 2) {
					cell_up_down_config(entry, 1);
				}
			} else {
				printf("band(IP:%s)scan fre com config error\n",
					client_ip);
				if(strncmp(client_ip, BAND_38_IPADDR, 12) != 0)
					break;
				if(entry->scan_frequency_flag == 1 ||
					entry->scan_frequency_flag == 2) {
					cell_up_down_config(entry, 1);
					pthread_mutex_lock(&entry->lock);
					entry->scan_frequency_flag = 0;
					pthread_mutex_unlock(&entry->lock);
				}
			}
#endif
			break;
		}
	case O_FL_ENB_TO_LMT_NTP_SERVER_IP_CFG_ACK: /* NTP 服务器配置应答 */
		{
			printf("set NTP server ip addr response!\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (!(pStr->CfgResult)) {
				printf("Set NTP server ip Success!\n");
				write_action_log(client_ip,
					"set NTP server ip response :Success!");
			} else {
				printf("Set NTP server ip  Failed!\n");
				write_action_log(client_ip,
					"set NTP server ip response :Failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_TIME_TO_RESET_CFG_ACK: /* 定点重启配置应答 */
		{
			printf("set time of reset!\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (!(pStr->CfgResult)) {
				printf("Set time of reset Success!\n");
				write_action_log(client_ip,
					"set time of reset request:Success!");
			} else {
				printf("Set time of reset Failed!\n");
				write_action_log(client_ip,
					"set time of reset request:Failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_CONTROL_UE_LIST_CFG_ACK://4.6 F03A管控名单配置应答
		{
			wrFLEnbToLmtControlUeListCfgAck_t *pStr =
				(wrFLEnbToLmtControlUeListCfgAck_t *)recBuf;
			printf("<---- 4.6 Control list config response\n");
			if(pStr->CfgResult == 0) {
				printf("Control list config success\n");
				write_action_log(client_ip,
					"control list config success!");
			} else if(pStr->CfgResult == 1) {
				printf("Control list config delete error\n");
				write_action_log(client_ip,
					"control list config delete failed!");
			} else if(pStr->CfgResult == 2) {
				printf("Control list config: add error\n");
				write_action_log(client_ip,
					"control list config add failed!");
			} else {
				printf("Control list config: invalid value\n");
				write_action_log(client_ip,
					"control list config invalid value!");
			}
			/* 发送管控名单信息查询请求 */
			send_generic_request_to_baseband(client_ip,
				O_FL_LMT_TO_ENB_CONTROL_LIST_QUERY);
			break;
		}
	case O_FL_ENB_TO_LMT_SELF_ACTIVE_CFG_PWR_ON_QUERY_ACK://3.57. 上电自激活查询应答
		{
			printf("<---- nonautomatic build cell!\n");
			wrFLEnbToLmtSelfActiveCfgPwrOnQueryAck_t *pStr =
				(wrFLEnbToLmtSelfActiveCfgPwrOnQueryAck_t *)recBuf;
			if(pStr->SelfActiveCfg == 0) {
				printf("automatic build cell!\n");
				break;
			}
			// SelfActiveCfg == 1说明上电不自建小区
			// 发送上电自建小区配置
			wrFLLmtToEnbSelfActivePwrOnCfg_t sStr;
			wrMsgHeader_t WrmsgHeader;
			memset(&WrmsgHeader, '\0', sizeof(WrmsgHeader));
			WrmsgHeader.u32FrameHeader = MSG_FRAMEHEAD_DEF;
			WrmsgHeader.u16MsgLength = sizeof(sStr); /* 命令长度 */
			WrmsgHeader.u16frame = SYS_WORK_MODE_TDD;
			WrmsgHeader.u16SubSysCode = SYS_CODE_DEFAULT;
			WrmsgHeader.u16MsgType = O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON;
#ifdef BIG_END
			WrmsgHeader.u32FrameHeader = my_htonl_ntohl(WrmsgHeader.u32FrameHeader);
			WrmsgHeader.u16MsgLength = my_htons_ntohs(WrmsgHeader.u16MsgLength);
			WrmsgHeader.u16frame = my_htons_ntohs(WrmsgHeader.u16frame);
			WrmsgHeader.u16SubSysCode = my_htons_ntohs(WrmsgHeader.u16SubSysCode);
			WrmsgHeader.u16MsgType = my_htons_ntohs(WrmsgHeader.u16MsgType);
#endif
			sStr.WrmsgHeaderInfo = WrmsgHeader;
			sStr.SelfActiveCfg = 0;

			U8 *send_msg = calloc(1, sizeof(sStr) + 1);
			if (!send_msg) break;
			memcpy(send_msg, &sStr, sizeof(sStr));
			baicell_send_msg_to_band(send_msg, sizeof(sStr), client_ip);
			free(send_msg);
			break;
		}

	case O_FL_LMT_TO_ENB_SELFCFG_CELLPARA_REQ_ACK: // 3.86.小区自配置请求应答
		{
			printf("<---- cell config query response\n");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (!(pStr->CfgResult)) {
				printf("band(IP:%s)cell config success!\n",
					client_ip);
				write_action_log(client_ip,
					"set cell self-configuration success!");
			} else {
				printf("band(IP:%s)cell config error!"
					"Arfcn list without!\n", client_ip);
				write_action_log(client_ip,
					"set cell self-configuration failed!");
			}
			break;
		}

	case O_FL_ENB_TO_LMT_SELFCFG_ARFCN_CFG_REQ_ACK://3.90. 频点自配置后台频点添加/删除应答
		{
			printf("<---- 3.90. background Arfcn config response");
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
#ifdef BIG_END
			pStr->CfgResult = my_htonl_ntohl(pStr->CfgResult);
#endif
			switch (pStr->CfgResult) {
			case 0:
				printf("band(IP:%s)background Arfcn config success!\n"
					, client_ip);
				write_action_log("response:", "add/del arfcn OK!");
				break;
			case 1:
				printf("band(IP:%s)background Arfcn config add error,"
					"add Arfcn repetition.\n", client_ip);
				write_action_log("response", "add arfcn failed!");
				break;
			case 2:
				printf("band(IP:%s)background Arfcn config add error,"
					"add Arfcn spill.\n", client_ip);
				write_action_log("response", "add arfcn failed!");
				break;
			case 3:
				printf("band(IP:%s)background Arfcn config delete error,"
					"Arfcn nonentity.\n", client_ip);
				write_action_log("response", "del arfcn failed!");
				break;
			case 4:
				printf("band(IP:%s)background Arfcn config error,"
					"Arfcn invalid.\n", client_ip);
				write_action_log("response", "add/del arfcn failed!");
				break;
			}
			break;
		}
	case O_FL_ENB_TO_LMT_AGC_SET_ACK: //3.92 AGC配置应答
		{
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t*)recBuf;
			if (!(pStr->CfgResult)) {
				printf("AGC config success\n");
				write_action_log(client_ip,
					"set AGC config success!");
			} else {
				printf("AGC config error：error number:(%u)",
					pStr->CfgResult);
				write_action_log(client_ip,
					"set AGC config failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_SYS_ARFCN_MOD_ACK: //3.94小区频点动态修改应答
		{
			//!todo:nyb,2017.11.18,add dynamic earfcn set feature
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t*)recBuf;
			if (!(pStr->CfgResult)) {
				printf("Earfcn dynamic config success(3.94)\n");
				write_action_log(client_ip,
					"3.94 Earfcn dynamic config success");
			} else {
				printf("Earfcn dynamic config failed!(%u)\n",
					pStr->CfgResult);
				write_action_log(client_ip,
					"3.94 Earfcn dynamic config failed!");
			}
			break;
		}
	case O_FL_ENB_TO_LMT_IMEI_REQUEST_CFG_ACK: /*3.102.IMEI 捕获配置应答 */
		{
			wrFLGenericConfigSetAck_t *pStr =
				(wrFLGenericConfigSetAck_t *)recBuf;
			if (pStr->CfgResult == 0) {
				write_action_log(client_ip,
					"automatic build cell config success");
			} else {
				write_action_log(client_ip,
					"IMEI enable config failed");
			}
			break;
		}
	default:
		{
			bc_complicated_pkg *bcpkg =
				malloc(sizeof(bc_complicated_pkg));
			if (!bcpkg) {
				perror("malloc bc_complicated_pkg\n");
				break;
			}
#ifdef CLI
			bcpkg->cli_info = cli_info;
#endif
			bcpkg->ip = strdup(client_ip);
			bcpkg->pkg = (U8 *)malloc(msg_len + 1);
			if (bcpkg->pkg == NULL) {
				perror("malloc bcpkg->pkg\n");
				free(bcpkg->ip);
				free(bcpkg);
				break;
			}
			memcpy(bcpkg->pkg, recBuf, msg_len);
			bcpkg->pkg[msg_len] = 0;
			safe_pthread_create(thread_pare_bc_complicated_package,
				(void *)bcpkg);
			break;
		}
	}
}

/* 创建线程读取Baicell数据 */
void *baicell_while_recv_lte_value(void *arg)
{
	if (!arg) return NULL;
	band_entry_t *entry = (band_entry_t *)arg;
	U8 rec_buf[BAICELL_MAX_MSG_LENGTH] = {0};
	U32 ret = 0, i = 0;
	U32 msg_len = 0;
	U32 header_len = sizeof(wrMsgHeader_t);
	while (1) {
		ret = recv(entry->sockfd, rec_buf, header_len, 0);
		if (ret == -1) {
			continue;
		} else if (ret == 0) {
			printf("socket closed %s\n", entry->ipaddr);
			pthread_mutex_lock(&(entry->lock));
			close(entry->sockfd);
			pthread_mutex_unlock(&(entry->lock));
			return NULL;
		}
		msg_len = rec_buf[6] | (rec_buf[7] << 8);
		if (msg_len > header_len) {
			ret += recv(entry->sockfd, &rec_buf[ret],
				msg_len - header_len, 0);
		}
		printf("rec from %s len %u(%u)\n", entry->ipaddr, msg_len, ret);
		for (i = 0; i < ret; i++) {
			if (i == header_len) printf("\n");
			if (i == 4) 
				printf("[%02x ", rec_buf[i]);
			else if (i == 5) 
				printf("%02x] ", rec_buf[i]);
			else
				printf("%02x ", rec_buf[i]);
		}
		printf("\n");
		baicell_pare_recv_value(rec_buf, entry);
		memset(rec_buf, 0, ret);
	}/* end while(1) */
	return NULL;
}

/* 循环读取报文,并进行发送解析 */
void *thread_socket_to_baicell(void *arg)
{
	/* 初始化Baicell 基带板的socket连接 */
	if (!init_baicell_socket()) {
		return NULL;
	}
	if (args_info.bc_proto_arg == BAICELL_PROTOCOL_TCP) {
		printf("Accept Baicell base band device connect!\n");
		while(1) {
			bc_client_fd = accept(bc_server_fd,
				(struct sockaddr *)&bc_client_addr,
				(socklen_t *)&bc_addrlen);
			if(bc_client_fd == -1) {
				/* accept failed! */
				continue;
			}
			printf("Accept:LTE fd: %d, ipaddr: %s\n",
				bc_client_fd,
				inet_ntoa(bc_client_addr.sin_addr));
			//ret = Epoll_add(bc_client_fd,
			//	inet_ntoa(bc_client_addr.sin_addr));
			/* 通过ip在链表中进行查找，未找到则新建节点 */
			band_entry_t *entry =
				band_list_entry_search_add(
					inet_ntoa(bc_client_addr.sin_addr));
			if (entry->sockfd != bc_client_fd) {
				if (entry->sockfd != -1)
					close(entry->sockfd);

				pthread_mutex_lock(&(entry->lock));
				entry->u16RemMode = 0;
				entry->u16SyncState = 1;
				entry->sockfd = bc_client_fd;
				pthread_mutex_unlock(&(entry->lock));
			}
			safe_pthread_create(baicell_while_recv_lte_value,
				(void *)entry);
#ifndef VMWORK
#ifdef SYS86
			/* 发送功放信息查询请求 */
			U8 ma = entry->pa_num;
			send_serial_request(ma, DATA_SEND_TYPE_GET_ANT_STATUS);
#endif
#endif
		} /* end while (1) */
	} else { /* args_info.bc_proto_arg == BAICELL_PROTOCOL_UDP */
		printf("Recv Baicell device massages...\n");
		U8 rec_buf[BAICELL_MAX_MSG_LENGTH] = {0};
		S32 ret = 0;
		while(1) {
			ret = recvfrom(bc_server_fd, rec_buf,
				BAICELL_MAX_MSG_LENGTH, 0,
				(struct sockaddr *)&bc_client_addr,
				&bc_addrlen);
			if (ret == -1) {
				printf("Recvfrom Baicell sock failed!%s(%d)\n",
					strerror(errno), errno);
				continue;
			}
			printf("Recvfrom Baicell (ip:%s,port:%u)\n",
				inet_ntoa(bc_client_addr.sin_addr),
				ntohs(bc_client_addr.sin_port));
			if (ret > 0) {
				S8 *_ip = inet_ntoa(bc_client_addr.sin_addr);
				baicell_pare_recv_value(rec_buf,
					band_list_entry_search_add(_ip));
				memset(rec_buf, 0, ret);
			}
		} /* end while(1) */
	} /* end if ... else*/
}

/* 每隔args_info.sendsta_itv_arg 时间，检查用户信息json数组中有没有用户信息,
 * 如果有,加锁并将json转换成字符串并使用http协议上至服务器, 
 * 然后删除json中的所有信息,重新初始化json数组
 * */
void *thread_read_send_baicell_json_object(void *arg)
{
	char *send_str = NULL;
	while (1) {
		sleep(args_info.sendsta_itv_arg);
		bc_sta_json_obj_lock();
		if (!bc_sta_count) {
			bc_sta_json_obj_unlock();
			continue;
		}
		send_str = cJSON_PrintUnformatted(bc_sta_object);
		if (!send_str) {
			bc_sta_json_obj_unlock();
			continue;
		}
		printf("Timeout，send LTE info str:%s\n", send_str);
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
		bc_sta_json_delete();
		bc_sta_json_init();
		bc_sta_json_obj_unlock();
	} /* end while(... */
	return NULL;
}

