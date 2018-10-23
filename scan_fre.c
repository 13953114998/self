/*
 * ============================================================================
 *
 *       Filename:  scan_fre.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年10月31日 18时51分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  weichao lu (chao), lwc_yx@163.com
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
#include "mosquit_sub.h"
#include "my_log.h"
#include "baicell_net.h"
#include "scan_fre.h"

U32 sysArfcn_def[SYSARFCN_DEF_NUM] = {37900, 38098, 38400, 38544, 38950, 39148,
	39292, 40936, 39325, 39300};

wrMsgHeader_t set_wrMsgHeader(U16 msgtype)
{
	wrMsgHeader_t WrmsgHeader;
	memset(&WrmsgHeader, '\0', sizeof(WrmsgHeader));
	WrmsgHeader.u32FrameHeader = MSG_FRAMEHEAD_DEF;
	WrmsgHeader.u16MsgType = msgtype;
	WrmsgHeader.u16frame = SYS_WORK_MODE_TDD;
	WrmsgHeader.u16SubSysCode = SYS_CODE_DEFAULT;

	return WrmsgHeader;
}

/*小区激活去激活配置（0：去激活；1：激活）*/
int cell_up_down_config(band_entry_t *entry, U32 state)
{
	printf("----------%s----------\n", __FUNCTION__);
	wrMsgHeader_t WrmsgHeader;
	memset(&WrmsgHeader, '\0', sizeof(WrmsgHeader));
	WrmsgHeader = set_wrMsgHeader(O_FL_LMT_TO_ENB_SET_ADMIN_STATE_CFG);
	U8 send_request[MAXDATASIZE] = {0};

	wrFLLmtToEnbSetAdminStateCfg_t pStr;
	memset(&pStr, '\0', sizeof(pStr));
	WrmsgHeader.u16MsgLength = sizeof(pStr);
	pStr.WrmsgHeaderInfo = WrmsgHeader;
	pStr.workAdminState = state;

	memcpy(send_request, &pStr, sizeof(pStr));
	baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
	return 0;
}

/*辅PLMN配置*/
int assist_plmn_config(band_entry_t *entry,U8 cfgMode)
{
	printf("@cfg mode 0x%x,%s[%d]\n",cfgMode, __FUNCTION__,__LINE__);
	wrMsgHeader_t WrmsgHeader;
	memset(&WrmsgHeader, '\0', sizeof(WrmsgHeader));
	WrmsgHeader = set_wrMsgHeader(O_FL_LMT_TO_ENB_SECONDARY_PLMNS_SET);
	U8 send_request[MAXDATASIZE] = {0};

	wrEnbToLmtSecondaryPlmnsQueryAck_t pStr;
	memset(&pStr, '\0', sizeof(pStr));

	//fill MSG HED
	WrmsgHeader.u16MsgLength = sizeof(pStr);
	pStr.WrmsgHeaderInfo = WrmsgHeader;
	if(ADD_ACTION == cfgMode)
		pStr.u8SecPLMNNum = 2;
	else
		pStr.u8SecPLMNNum = 0;

	switch(cfgMode)
	{
		case ADD_ACTION:
		{
			write_action_log(entry->ipaddr,"Aux PLMN ADD_ACTION" );
			if(!strncmp(entry->ipaddr, BAND_38_IPADDR, 12))
			{
				strcpy((S8 *)&(pStr.u8SecPLMNList[0]), "46001");
				strcpy((S8 *)&(pStr.u8SecPLMNList[1]), "46011");
			}
#if 0
			else if(!strncmp(entry->ipaddr, BAND_39_IPADDR, 12))
			{
				strncpy((S8 *)(pStr.u8SecPLMNList[0]), "46001", 5);
				strncpy((S8 *)(pStr.u8SecPLMNList[1]), "46011", 5);
			}
#endif
			else if(!strncmp(entry->ipaddr, BAND_36_IPADDR, 12))
			{
				strcpy((S8 *)&(pStr.u8SecPLMNList[0]), "46011");
				//strncpy((S8 *)(pStr.u8SecPLMNList[1]), "46000", 5);
				pStr.u8SecPLMNNum = 1;
			}
#if 0
			else if(!strncmp(entry->ipaddr, BAND_37_IPADDR, 12))
			{
				strncpy((S8 *)(pStr.u8SecPLMNList[0]), "46001", 5);
				strncpy((S8 *)(pStr.u8SecPLMNList[1]), "46000", 5);
			}
#endif
			else
			{
				write_action_log(entry->ipaddr,"invalid IP ADDR for Aux PLMN!" );
				return -1;
			}
			break;
		}
		case CLR_ACTION:
		{
			//do nothing,init zero will clear the secdonary plmn
			write_action_log(entry->ipaddr,"Aux PLMN CLR_ACTION" );
			break;
		}
		default:
			break;
	}
	memcpy(send_request, &pStr, sizeof(pStr));
	baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
	return 0;
}

/*配置扫频端口（0：RX；1：SNF）*/
int scan_fre_com_config(band_entry_t *entry, U32 state)
{
	printf("----------%s----------\n", __FUNCTION__);
	wrMsgHeader_t WrmsgHeader;
	memset(&WrmsgHeader, '\0', sizeof(WrmsgHeader));
	WrmsgHeader = set_wrMsgHeader(O_FL_LMT_TO_ENB_REM_ANT_CFG);
	U8 send_request[MAXDATASIZE] = {0};

	wrFLLmtToEnbRemAntCfg_t pStr;
	memset(&pStr, '\0', sizeof(pStr));
	WrmsgHeader.u16MsgLength = sizeof(pStr);
	pStr.WrmsgHeaderInfo = WrmsgHeader;
	pStr.RxorSnf = state;

	memcpy(send_request, &pStr, sizeof(pStr));
	baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
	return 0;
}

/*开启扫频*/
int start_scan_fre(band_entry_t *entry)
{
	printf("----------%s----------\n", __FUNCTION__);
	wrMsgHeader_t WrmsgHeader;
	memset(&WrmsgHeader, '\0', sizeof(WrmsgHeader));
	WrmsgHeader = set_wrMsgHeader(O_FL_LMT_TO_ENB_REM_CFG);
	U8 send_request[MAXDATASIZE] = {0};

	wrFLLmtToEnbRemcfg_t pStr;
	memset(&pStr, '\0', sizeof(pStr));
	WrmsgHeader.u16MsgLength = sizeof(pStr);
	pStr.WrmsgHeaderInfo = WrmsgHeader;
	pStr.wholeBandRem = 0;
	pStr.sysArfcnNum = SYSARFCN_DEF_NUM;
	int i;
	for(i = 0; i < SYSARFCN_DEF_NUM; i++)
	{
		pStr.sysArfcn[i] = sysArfcn_def[i];
		printf("--->sysarfcn[%d]:%u\n", i, pStr.sysArfcn[i]);
	}
	memcpy(send_request, &pStr, sizeof(pStr));
	baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
	return 0;
}

/*判断小区状态，开启LTE扫频任务*/
int lte_scan_fre(band_entry_t *entry)
{
	pthread_mutex_lock(&entry->lock);
	entry->scan_frequency_flag = 1;
	pthread_mutex_unlock(&entry->lock);

	printf("当前小区状态:%d, ", entry->cellstate);
	if (entry->cellstate == 1 /* REM 执行中 */) {
		return 0;
	}
	int count = 4;
	while (count--) {
		if(entry->cellstate == 3){
			printf("小区已激活, 需要先去激活!\n");
			cell_up_down_config(entry, 0);
		} else {
			scan_fre_com_config(entry, 1);
			break;
		}
		sleep(3);
	}
	return 0;
}

/*开启GSM扫频任务*/
int gsm_scan_fre()
{
	char json_buf[64];
	sprintf(json_buf, "{\"msgtype\":\"0x03\",\"ip\":\"192.168.178.202\"}");
	pare_config_json_message(json_buf, FAULT_CLIENT_SOCKET_FD);
	return 0;
}

/*每过一段时间，对基带板进行手动扫频，并上报
* 1.小区去激活（0）
* 2.配置扫频端口为SNF（1）
* 3.进行扫频
* 4.配置扫频端口为RX（0）
* 5.小区激活（1）
*/
void *thread_scan_frequency(void *arg)
{
	printf("----------%s----------\n", __FUNCTION__);
	band_entry_t *entry;
	FILE *scan_fre_fp;
	char scanfre_flag = '0';
	while(1)
	{
		sleep(60 * 60);
		scan_fre_fp = fopen(SCAN_FRE_FILE, "r+");
		if(scan_fre_fp == NULL)
		{
			printf("open scanfre_file err!\n");
			continue;
		}
		scanfre_flag = fgetc(scan_fre_fp);
		if(scanfre_flag == '0')
		{
			fclose(scan_fre_fp);
			continue;
		}
		fseek(scan_fre_fp, SEEK_SET, 0);
		scanfre_flag = '0';
		fputc(scanfre_flag, scan_fre_fp);
		fclose(scan_fre_fp);

		entry = band_list_entry_search(BAND_38_IPADDR);
		if (!entry)
		{
			printf("search entry by ip(38) failed!\n");
			goto goto_gsm_scan;
		}
		lte_scan_fre(entry);
		sleep(1);
goto_gsm_scan:
		entry = band_list_entry_search(GSM_IP_ADDR);
		if(!entry) {
			printf("search entry by ip(202) failed!\n");
			continue;
		}
		gsm_scan_fre();
	}
	return NULL;
}
