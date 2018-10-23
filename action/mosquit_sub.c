/*
 * ============================================================================
 *
 *       Filename:  mqttc.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年04月07日 16时44分28秒
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
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <mosquitto.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>

#include "udp_gsm.h"
#include "mosquit_sub.h"
#include "arg.h"
#include "cJSON.h"
#include "hash_list_active.h"
#include "config.h"
#include "baicell_net.h"
#include "include/http.h"
#include "tcp_wcdma.h"
#ifdef CLI
#include "cli_action.h"
#endif
#include "my_log.h"
#include "fiberhome_lte.h"
#ifndef VMWORK
#ifdef SYS86
#include "gpio_ctrl.h"
#include "serialtest_sh.h"
#include "serialtest_ln.h"
#endif
#endif
#include "save2sqlite.h"
#include "scan_fre.h"

/* ************** 下载服务器指定文件并进行操作 相关宏定义 ************** */
#define UPDATE_FILE "/tmp/update.sh" /* 操作文件 */
/* 文件类型 */
#define FILE_TYPE_IPK   (0)// 0：ipk文件  file_name 后缀名: .ipk
#define FILE_TYPE_IMG   (1)// 1：固件文件 file_name 后缀名: .img/.bin
#define FILE_TYPE_TAR   (2)// 2：压缩文件 file_name 后缀名: .tar.gz
#define FILE_TYPE_SHELL (3)// 3：脚本文件 file_name 后缀名: .sh
/* 下载链接类型 */
#define DOWNLOAD_TYPE_HTTP (0)// 0：http协议
#define DOWNLOAD_TYPE_FTP  (1)// 1：ftp协议
/* ********************************************************************* */

/* 频点默认配置信息 */
def_band_config_t default_config[] =
{
	/*上频,    下频,  PLMN,   带宽, 频段, 物区ID(pci),   区码,  小区ID, UePMax,EnodeBPMax */
//  { 19650,   1650, "46001",   25,    3,   501,        12345,   123,     23,     20},
    {0x4CC2,  0x672, "46001", 0x19,    3,   0x1F5,      0x3039,  0x7B,   0x17,   0x14}, /* 36-同频 */
//  { 19925,   1925, "46001",   25,    3,   501,        12345,   123,     23,     20},
    {0x4DD5,  0x785, "46001", 0x19,    3,   0x1F5,      0x3039,  0x7B,   0x17,   0x14}, /* 36-异频 */
//  { 19825,   1825, "46000",   25,    3,   501,        12345,   123,     23,     20},
    {0x4D71,  0x721, "46011", 0x19,    3,   0x1F5,      0x3039,  0x7B,   0x17,   0x14}, /* 37 */
//  {   255,  37900, "46000",   25,   38,   501,        1264,    123,     23,     20},
    {  0xFF, 0x940C, "46000", 0x19, 0x26,   0x1F5,      0x4F0,   0x7B,   0x17,   0x14}, /* 38-同频 */
//  {   255,  39300, "46000",   25,   40,   501,        12345,   123,     23,     20}
    {  0xFF, 0x9984, "46000", 0x19, 0x28,   0x1F5,      0x3039,  0x7B,   0x17,   0x14}, /* 38-异频 */
//  {   255,  38400, "46000",   25,   39,   501,        12345,   123,     23,     20},
    {  0xFF, 0x9600, "46000", 0x19, 0x27,   0x1F5,      0x3039,  0x7B,   0x17,   0x14}  /* 39 */
};

void client_config_cleanup(struct mosq_config *cfg)
{
	int i;
	free(cfg->id);
	free(cfg->id_prefix);
	free(cfg->host);
	free(cfg->file_input);
	free(cfg->message);
	free(cfg->topic);
	free(cfg->bind_address);
	free(cfg->username);
	free(cfg->password);
	free(cfg->will_topic);
	free(cfg->will_payload);
	if(cfg->topics){
		for(i=0; i<cfg->topic_count; i++){
			free(cfg->topics[i]);
		}
		free(cfg->topics);
	}
	if(cfg->filter_outs) {
		for(i=0; i<cfg->filter_out_count; i++) {
			free(cfg->filter_outs[i]);
		}
		free(cfg->filter_outs);
	}
}

int client_opts_set(struct mosquitto *mosq, struct mosq_config *cfg)
{
	int rc;

	if(cfg->will_topic && mosquitto_will_set(mosq, cfg->will_topic,
				cfg->will_payloadlen, cfg->will_payload,
				cfg->will_qos, cfg->will_retain))
	{

		if(!cfg->quiet)
		{
			fprintf(stderr, "Error: Problem setting will.\n");
		}
		mosquitto_lib_cleanup();
		return 1;
	}
	if(cfg->username &&
		mosquitto_username_pw_set(mosq, cfg->username, cfg->password))
	{
		if(!cfg->quiet)
		{
			printf("Error:Problem setting username and password.\n");
		}
		mosquitto_lib_cleanup();
		return 1;
	}
	mosquitto_max_inflight_messages_set(mosq, cfg->max_inflight);
	mosquitto_opts_set(mosq,
		MOSQ_OPT_PROTOCOL_VERSION, &(cfg->protocol_version));
	return MOSQ_ERR_SUCCESS;
}

int client_connect(struct mosquitto *mosq, struct mosq_config *cfg)
{
	char err[1024];
	int rc;

	rc = mosquitto_connect_bind(mosq, cfg->host, cfg->port,
		cfg->keepalive, cfg->bind_address);
	if (rc > 0)
	{
		if (!cfg->quiet)
		{
			if (rc == MOSQ_ERR_ERRNO)
			{
				strerror_r(errno, err, 1024);
				fprintf(stderr, "Error: %s\n", err);
			}
			else
			{
				fprintf(stderr, "Unable to connect (%s).\n",
					mosquitto_strerror(rc));
			}
		}
		mosquitto_lib_cleanup();
		return rc;
	}
	return MOSQ_ERR_SUCCESS;
}

/* mosquitto connect callback function */
void my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int i;
	struct mosq_config *cfg;
	assert(obj);
	cfg = (struct mosq_config *)obj;

	if (!result)
	{
		for (i = 0; i < cfg->topic_count; i++)
		{
			mosquitto_subscribe(mosq, NULL, cfg->topics[i], cfg->qos);
		}
	}
	else
	{
		if (result && !cfg->quiet)
		{
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}
/* 修改server 端IP和URL地址
 * 返回值:
 * -1 数据错误 或者不合法
 *  0 无需修改
 *  1 IP
 *  2 URL
 *  3 IP与URL需要修改
 * */
static int test_seerver_ip_url(cJSON *pa_object)
{
	if (pa_object == NULL) return -1;
	cJSON *server_ip = cJSON_GetObjectItem(pa_object, "server_ip");
	cJSON *sys_web_service = cJSON_GetObjectItem(pa_object, "sys_web_service");
	if (!server_ip && !sys_web_service) {
		printf("get information failed!\n");
		return -1;
	}
	int ret = 0;
	if (memcmp(args_info.hostip_arg, server_ip->valuestring,
			strlen(server_ip->valuestring)))
	{
		int test_socket = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in serveraddr;
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(1187);
		serveraddr.sin_addr.s_addr = inet_addr(server_ip->valuestring);
		if (connect(test_socket, (struct sockaddr*)&serveraddr,
				sizeof(struct sockaddr)) == -1)
		{
			printf("connection(IP:%s) failed!\n",
				server_ip->valuestring);
			return -1;
		}
		else
		{
			ret += 1;
			close(test_socket);
		}
		char config_cmd[128] = {0};
		snprintf(config_cmd, 128, "%s %s %s",
			"/usr/sbin/set_one_masterconfig",
			"hostip", server_ip->valuestring);
		printf("config_cmd: [%s]\n", config_cmd);
		system(config_cmd);
	}
	else
	{
		printf("[%s]--[%s]\n",
			args_info.hostip_arg, server_ip->valuestring);
		printf("need not update host ip!\n");
	}
	if (memcmp(args_info.url_arg, sys_web_service->valuestring,
			strlen(sys_web_service->valuestring)))
	{
		char test_cmd[136] = {0};
		char tmp_str[64] = {0};
		snprintf(test_cmd, 136, "curl http://%s/check_ok",
			sys_web_service->valuestring);
		FILE *test_fp = popen(test_cmd, "r");
		if (test_fp == NULL)
		{
			return -1;
		}
		fread(tmp_str, 1, 64, test_fp);
		if (strstr(tmp_str, "success") && strstr(tmp_str, "ctrl success!"))
		{
			ret += 2;
		}
		char config_cmd[256] = {0};
		snprintf(config_cmd, 256, "%s %s %s",
			"/usr/sbin/set_one_masterconfig",
			"url", sys_web_service->valuestring);
		printf("config_cmd: [%s]\n", config_cmd);
		system(config_cmd);
	}
	else
	{
		printf("args_info.url_arg--sys_web_service->valuestring[%s]--[%s]\n",
			(S8 *)(args_info.url_arg), sys_web_service->valuestring);
		printf("need not update url!\n");
	}
	return ret;
}


/*修改基带板、功放状态的配置文件*/
void wr_band_pa_file(int cmd)
{
	same_or_dif_state = cmd; //work model,1/2/3
	//baseband status
	state_band_pa.band_state[0] = GPIO_PORT_ENABLE;  /*36 default enable*/
	state_band_pa.band_state[2] = GPIO_PORT_ENABLE;  /*38 default enable*/
	state_band_pa.band_state[4] = GPIO_PORT_ENABLE;  /*GSM always enable, actually we need to consider the configure file*/
	state_band_pa.band_state[5] = GPIO_PORT_ENABLE;  /*WCDMA or WIFI default disable*/
	//PA status, B3 ADDR 0, B38 ADDR 1, B39 ADDR 2
	state_band_pa.pa_state[0] = PA_ENABLE;    /*B3 unicom PA*/
	state_band_pa.pa_state[1] = PA_ENABLE;    /*B38 PA*/

	char logStr[200] = {0};
	sprintf(logStr,"%s %x","Baseband and PA status was changed with cmd: ",cmd);
	write_action_log("wr_band_pa_file:", logStr);

	switch(cmd)
	{
		case INTRA_WORK_MODEL:
		{
			state_band_pa.band_state[1] = GPIO_PORT_ENABLE;
			state_band_pa.band_state[3] = GPIO_PORT_ENABLE;
			state_band_pa.pa_state[2] = PA_ENABLE;
			break;
		}
		case INTER_A_WORK_MODEL:
		{
			state_band_pa.band_state[1] = GPIO_PORT_DISABLE;
			state_band_pa.band_state[3] = GPIO_PORT_DISABLE;
			state_band_pa.pa_state[2] = PA_DISABLE;
			break;
		}
		case INTER_B_WORK_MODEL:
		{
			state_band_pa.band_state[1] = GPIO_PORT_ENABLE;
			state_band_pa.band_state[3] = GPIO_PORT_DISABLE;
			state_band_pa.pa_state[2] = PA_DISABLE;
			break;
		}
		default:
			write_action_log("wr_band_pa_file:", "we didn't support current cfg work model");
			return;
	}
	char *errmsg;
	char sql_cmd[1024];
	sprintf(sql_cmd, "update band_pa_state set same_or_dif_state = %d,\
					band0_state = %d,band1_state = %d,band2_state = %d,\
					band3_state = %d,band4_state = %d,band5_state = %d,\
			        pa0_state = %d,pa1_state = %d,pa2_state = %d;",
					same_or_dif_state,
					state_band_pa.band_state[0],
					state_band_pa.band_state[1],
					state_band_pa.band_state[2],
					state_band_pa.band_state[3],
					state_band_pa.band_state[4],
					state_band_pa.band_state[5],
					state_band_pa.pa_state[0],
					state_band_pa.pa_state[1],
					state_band_pa.pa_state[2]);
	if(sqlite3_exec(band_pa_state_db, sql_cmd,  NULL,  NULL, &errmsg) !=  SQLITE_OK)
		 printf("sqlite3 set band_pa_state info error:%s\n", errmsg);
	if(errmsg)
		sqlite3_free(errmsg);
}

void set_base_band_online_status(int cmd)
{
	band_entry_t *tpos;
	band_entry_t *n;
	pthread_mutex_lock(&band_list_lock);
	switch(cmd) {
	case SET_STATE_SAME://设置同频
		list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
			tpos->online = 1;
			tpos->change_count = 1;
		}
		break;
	case SET_STATE_DIF_A://设置异频A
		list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
			if (!strcmp(tpos->ipaddr, BAND_37_IPADDR) ||
				!strcmp(tpos->ipaddr, BAND_39_IPADDR)) {
				tpos->online = 0;
				tpos->change_count = -10;
			}
		}
		break;
	case SET_STATE_DIF_B://设置异频B
		list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
			if (!strcmp(tpos->ipaddr, BAND_39_IPADDR)) {
				tpos->online = 0;
				tpos->change_count = -10;
			}
		}
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&band_list_lock);
	char *json_string = get_lte_base_band_status(GET_DEFAULT_CONFIG, NULL);
	if (json_string == NULL)
		return;
	pthread_mutex_lock(&http_send_lock);
	http_send(args_info.baseband_api_arg, json_string, NULL, NULL);
	pthread_mutex_unlock(&http_send_lock);
	free(json_string);
}
/*控制基带板与功放开关*/
void band_and_pa_ctrl(int cmd, int cli_fd)
{
#ifndef VMWORK
#ifdef SYS86
	int i;
	write_action_log("band_and_pa_ctrl, intral or inter model cfg cmd:", &cmd);
	switch(cmd) {
	case INTRA_OR_INTER_MODEL://0:获取状态
		{
#ifdef CLI
			if (cli_fd != -1) {
				char cli_str[128] = {0};
				snprintf(cli_str, 128, "Work mode is %s",
					(same_or_dif_state == 1)? "same frequency":
					((same_or_dif_state == 2)?("different frequency A"):
					 ("different frequency B")));
				write(cli_fd, cli_str, strlen(cli_str) + 1);
			}
#endif
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			pthread_mutex_lock(&set_band_pa_file_lock);
			cJSON_AddNumberToObject(object, "band_type",
				same_or_dif_state);
			pthread_mutex_unlock(&set_band_pa_file_lock);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				break;
			}
			printf("same or different frequency send str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.same_dif_fre_api_arg,
				send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
		case INTRA_WORK_MODEL://1:设置同频
		{
			pthread_mutex_lock(&set_band_pa_file_lock);

			//here ,we only need to consider LTE band,except GSM and WCDMA
			for(i = 0; i < BAND_NUM-2; i++)
			{
				if(state_band_pa.band_state[i] == GPIO_PORT_DISABLE)
				{
					switch_gpio(i, LEVEL_HIG);
					state_band_pa.band_state[i] = GPIO_PORT_ENABLE;
				}
			}

			//below we need to diff SH and LN PA via index, for SH PA address, it's addr was 0,1,2 seperately,but LN was B3 0x40/0X50, B38 0X10, b39 0X30
			for(i = 0; i < PA_NUM; i++)
			{
				if(state_band_pa.pa_state[i] == PA_DISABLE)
				{
					send_serial_request(i, DATA_SEND_TYPE_SET_ANT_UP);
					state_band_pa.pa_state[i] = PA_ENABLE;
				}
			}
			same_or_dif_state = INTRA_WORK_MODEL;
			pthread_mutex_unlock(&set_band_pa_file_lock);
#ifdef CLI
			if (cli_fd != -1) {
				char cli_str[128] = {0};
				snprintf(cli_str, 128, "Set same frequency success!");
				write(cli_fd, cli_str, strlen(cli_str) + 1);
			}
#endif
			break;
		}
		case INTER_A_WORK_MODEL:// 2:设置异频A;
		{
			pthread_mutex_lock(&set_band_pa_file_lock);
			for(i = 0; i < BAND_NUM-2; i++)
			{
				if(i == BBU37_ID || i == BBU39_ID)
				{
					if(state_band_pa.band_state[i] == GPIO_PORT_ENABLE)
					{
						switch_gpio(i, LEVEL_LOW);
						state_band_pa.band_state[i] = GPIO_PORT_DISABLE;
					}
					
				}
				if(i == BBU36_ID || i == BBU38_ID)
				{
					if(state_band_pa.band_state[i] == GPIO_PORT_DISABLE)
					{
						switch_gpio(i, LEVEL_HIG);
						state_band_pa.band_state[i] = GPIO_PORT_ENABLE;
					}
				}
			}
			for(i = 0; i < PA_NUM; i++)
			{
				if(i == PA_39_ID)
				{
					if(state_band_pa.pa_state[i] == PA_ENABLE)
					{
						send_serial_request(i, DATA_SEND_TYPE_SET_ANT_DOWN);
						state_band_pa.pa_state[i] = PA_DISABLE;
					}
				}else{
					if(state_band_pa.pa_state[i] == PA_DISABLE)
					{
						send_serial_request(i, DATA_SEND_TYPE_SET_ANT_UP);
						state_band_pa.pa_state[i] = PA_ENABLE;
					}
				}
			}
			same_or_dif_state = INTER_A_WORK_MODEL;
			pthread_mutex_unlock(&set_band_pa_file_lock);
#ifdef CLI
			if (cli_fd != -1) {
				char cli_str[128] = {0};
				snprintf(cli_str, 128, "Set pilotA frequency success!");
				write(cli_fd, cli_str, strlen(cli_str) + 1);
			}
#endif
			break;
		}
		case INTER_B_WORK_MODEL://3:设置异频B
		{
			pthread_mutex_lock(&set_band_pa_file_lock);
			for(i = 0; i < BAND_NUM-2; i++)
			{
				if(i == BBU39_ID)
				{
					if(state_band_pa.band_state[i] == GPIO_PORT_ENABLE)
					{
						switch_gpio(i, LEVEL_LOW);
						state_band_pa.band_state[i] = GPIO_PORT_DISABLE;
					}
				}
				else
				{
					if(state_band_pa.band_state[i] == GPIO_PORT_DISABLE)
					{
						switch_gpio(i, LEVEL_HIG);
						state_band_pa.band_state[i] = GPIO_PORT_ENABLE;
					}
				}
			}
			for(i = 0; i < PA_NUM; i++)
			{
				if(i == PA_39_ID)
				{
					if(state_band_pa.pa_state[i] == PA_ENABLE)
					{
						send_serial_request(i, DATA_SEND_TYPE_SET_ANT_DOWN);
						state_band_pa.pa_state[i] = PA_DISABLE;
					}
				}
				else
				{
					if(state_band_pa.pa_state[i] == PA_DISABLE)
					{
						send_serial_request(i, DATA_SEND_TYPE_SET_ANT_UP);
						state_band_pa.pa_state[i] = PA_ENABLE;
					}
				}
			}
			same_or_dif_state = INTER_B_WORK_MODEL;
			pthread_mutex_unlock(&set_band_pa_file_lock);
#ifdef CLI
			if (cli_fd != -1) {
				char cli_str[128] = {0};
				snprintf(cli_str, 128, "Set pilotB frequency success!");
				write(cli_fd, cli_str, strlen(cli_str) + 1);
			}
#endif
			break;
		}
	}
#endif
#endif
	return;
}
/* 下载服务器指定文件并进行操作 */
static void update_downloadfile(char *d_url, char *f_name, char *f_md5,
	int f_type, int d_type, char *ftp_uname, char *ftp_passwd)
{
	char wget_cmd[128] = {0};
	if (DOWNLOAD_TYPE_HTTP == d_type) {
		snprintf(wget_cmd, 128,
			"wget -c -q "
			"--http-user=$ftp_uname --http-password=$ftp_passwd "
			"$download_url -P /tmp/\n");
	} else if (DOWNLOAD_TYPE_FTP) {
		snprintf(wget_cmd, 128,
			"wget -c -q "
			"--ftp-user=$ftp_uname --ftp-password=$ftp_passwd "
			"$download_url -P /tmp/\n");
	} else {
		return;
	}
	char active_cmd[128] = {0};
	if (FILE_TYPE_IPK == f_type) {
		snprintf(active_cmd, 128,
			"	opkg install /tmp/%s\n"
			"	reboot\n"
			, f_name);
	} else if (FILE_TYPE_IMG == f_type) {
		snprintf(active_cmd, 128,
			"	sysupgrade  /tmp/%s &\n"
			, f_name);
	} else if (FILE_TYPE_TAR == f_type) {
		snprintf(active_cmd, 128,
			"	tar -zxvf /tmp/%s -C / \n"
			"	reboot\n"
			, f_name);
	} else if (FILE_TYPE_SHELL == f_type) {
		snprintf(active_cmd, 128,
			"	chmod a+x /tmp/%s\n"
			"	/tmp/%s\n" ,
			f_name,	f_name);
	}
	FILE *wfd = fopen(UPDATE_FILE, "w");
	if (NULL == wfd) {
		perror("open "UPDATE_FILE" error");
		return;
	}
	fprintf(wfd,
		"#!/bin/sh\n"
		"download_url=\"%s/%s\"\n"
		"file_md5=$(echo \"%s\" | tr '[A-Z]' '[a-z]')\n"
		"ftp_uname=\"%s\"\n"
		"ftp_passwd=\"%s\"\n"
		"echo file downloading... \n"
		"%s\n"
		"echo file downloaded! \n"
		"echo check file MD5... \n"
		"df_md5=\"$(md5sum /tmp/%s "
		"| awk '{print $1}' | tr '[A-Z]' '[a-z]')\"\n"
		"\n"
		"#echo \"file_md5: $file_md5\"\n"
		"#echo \"  df_md5: $df_md5\"\n"
		"if [ \"$file_md5\"x = \"$df_md5\"x ]; then\n"
		"	echo 'check file MD5 success!'\n"
		"%s\n"
		"else\n"
		"	echo 'check file MD5 failed!'\n"
		"	rm -rf /tmp/%s\n"
		"fi\n"
		"\n"
		,
		d_url, f_name,
		f_md5,
		ftp_uname,
		ftp_passwd,
		wget_cmd,
		f_name,
		active_cmd,
		f_name);
	fclose(wfd);
	chmod(UPDATE_FILE, 0755);
	system(UPDATE_FILE"&");
}
/* 通过IP 和type写日志 */
void write_action_log_by_type(char *cli_ip, U16 type)
{
	switch(type) {
	case MQTT_WCDMA_GET_STATUS_OF_BASE:
		// (0xC082) 6.19 获取设备运行参数
		write_action_log(cli_ip,
			"get base band dev generic information request!");
		break;
	case O_FL_LMT_TO_ENB_GET_ENB_STATE:
		// (0xF01A) 3.20. 获取小区最后一次操作执行的状态请求
		write_action_log(cli_ip,
			"get last hole config value infor request!");
		break;
	case O_FL_LMT_TO_ENB_GET_ARFCN:
		// (0xF027) 3.34. 小区频点配置相关参数查询
		write_action_log(cli_ip, "get arfcn infor request!");
		break;
	case O_FL_LMT_TO_ENB_SYNC_INFO_QUERY:
		// (0xF02D) 3.42. 基站同步信息查询
		write_action_log(cli_ip, "get base band sync infor request!");
		break;
	case O_FL_LMT_TO_ENB_CELL_STATE_INFO_QUERY:
		// (0xF02F) 3.44. 小区状态信息查询
		write_action_log(cli_ip, "get cell status infor request!");
		break;
	case O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY:
		// (0xF031) 3.48. 接收增益和发射功率查询
		write_action_log(cli_ip, "get regain/txpower infor request!");
		break;
	case O_FL_LMT_TO_ENB_ENB_IP_QUERY:
		// (0xF033) 3.48. IP地址查询
		write_action_log(cli_ip, "get IP request!");
		break;
	case O_FL_LMT_TO_ENB_QRXLEVMIN_VALUE_QUERY:
		// (0xF035) 3.50. 小区选择 QRxLevMin 查询
		write_action_log(cli_ip, "get QRxlevMin infor request!");
		break;
	case O_FL_LMT_TO_ENB_REM_CFG_QUERY:
		// (0xF037) 3.54. 扫频参数配置查询
		write_action_log(cli_ip, "get sweep config status request!");
		break;
	case O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG_QUERY:
		// (0xF03F) 3.56. 重定向参数配置查询
		write_action_log(cli_ip, "get redirect config status request!");
		break;
	case O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON_QUERY:
		// (0xF041) 3.58. 上电小区自激活配置查询
		write_action_log(cli_ip,
			"get active cell when system up status request!");
		break;
	case O_FL_LMT_TO_ENB_SYS_LOG_LEVL_QUERY:
		// (0xF047) 3.62. 查询 log 打印级别
		write_action_log(cli_ip, "get log print level request!");
		break;
	case O_FL_LMT_TO_ENB_TDD_SUBFRAME_ASSIGNMENT_QUERY:
		// (0xF04B) 3.64. TDD 子帧配置查询
		write_action_log(cli_ip, "get TDD subframe config request!");
		break;
	case O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY:
		// (0xF05C) 3.73. GPS经纬高度查询
		write_action_log(cli_ip, "get GPS status request!");
		break;
	case O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_QUERY:
		// (0xF06B) 3.75. 位置区更新拒绝原因查询
		write_action_log(cli_ip,
			"get Location Update refusal cause request!");
		break;
	case O_FL_LMT_TO_ENB_GPS_INFO_RESET:
		// (0xF06D) 3.77. Gps 信息复位配置
		write_action_log(cli_ip, "get reset GPS infor request!");
		break;
	case O_FL_LMT_TO_ENB_GPS1PPS_QUERY:
		// (0xF073) 3.83. GPS 同步偏移量查询
		write_action_log(cli_ip, "get GPS sync offset request!");
		break;
	case O_FL_LMT_TO_ENB_SELFCFG_ARFCN_QUERY:
		// (0xF04D) 3.87. 频点自配置后台频点列表查询
		write_action_log(cli_ip, "get arfcn list query request!");
		break;
	case O_FL_LMT_TO_ENB_MEAS_UE_CFG_QUERY:
		// (0xF03D) 4.3. UE 测量配置查询
		write_action_log(cli_ip, "get UE config request!");
		break;
	case O_FL_LMT_TO_ENB_CONTROL_LIST_QUERY:
		// (0xF043) 4.7 管控名单查询(黑白名单)
		write_action_log(cli_ip, "get control list request!");
		break;
	case O_FL_LMT_TO_ENB_RA_ACCESS_QUERY:
		// (0xF065) 7.1. 随机接入成功率问询
		write_action_log(cli_ip, "get random access success request!");
		break;
	case O_FL_LMT_TO_ENB_RA_ACCESS_EMPTY_REQ:
		// (0xF067) 7.3. 随机接入成功率清空请求
		write_action_log(cli_ip, "reset random access success request!");
		break;
	case O_FL_LMT_TO_ENB_SECONDARY_PLMNS_QUERY:
		// (0xF062) 7.9. 辅 PLMN 列表查询
		write_action_log(cli_ip, "get assist PLMN list request!");
		break;
	default:
		break;
	}
}
/* 解析json数据并进行相关的配置 */
void pare_config_json_message(char *my_msg, int cli_sfd)
{
#if 1
	cJSON *pa_object = cJSON_Parse(my_msg);
	if (!pa_object) {
		printf("Pare json object of config msg from server error!\n");
		return;
	}
	int i, j;
	/* get config msg type */
	cJSON *msgtype = cJSON_GetObjectItem(pa_object, "msgtype");
	if (!msgtype) {
		printf("Not founnd 'msgtype'\n");
		goto go_return;
	}
	U32 msgt = 0;
	sscanf(msgtype->valuestring, "%x", &msgt);
	/* get target ip addr */
	cJSON *bandip = cJSON_GetObjectItem(pa_object, "ip");
	/* the buffer of save value that will send to base band device */
	U8 send_request[MAXDATASIZE] = {0};
	/* init LTE message header */
	wrMsgHeader_t WrmsgHeader;
	memset(&WrmsgHeader, '\0', sizeof(WrmsgHeader));
	WrmsgHeader.u32FrameHeader = MSG_FRAMEHEAD_DEF;
	WrmsgHeader.u16SubSysCode = SYS_CODE_DEFAULT;
#ifdef BIG_END
	WrmsgHeader.u32FrameHeader = my_htonl_ntohl(WrmsgHeader.u32FrameHeader);
	WrmsgHeader.u16SubSysCode = my_htons_ntohs(WrmsgHeader.u16SubSysCode);
#endif
	/*发送GSM请求消息体头*/
	WrtogsmHeader_t WrtogsmHeader;
	memset(&WrtogsmHeader, '\0', sizeof(WrtogsmHeader));

	switch(msgt) {
#if 0
	case O_CHANGE_SERVER_IP_CFG:
		{ //0xE002 配置主控板链接的IP
			printf("server IP and url set\n");
			int ret = test_seerver_ip_url(pa_object);
			if (ret == 0) {
				goto go_return;
			}
			if (ret == -1) {
				printf("config information is failed!\n");
				goto go_return;
			}
			else printf("config is diffrent, need update!\n");
			exit(1);
		}
#endif
	case O_SERVER_TO_BAND_AND_PA_CMD://0xE004 切换同频异频/获取同频异频状态
	{
		U32 cmd = 0;
		cJSON *command = cJSON_GetObjectItem(pa_object, "command");
		if (command) {
			sscanf(command->valuestring, "%d", &cmd);
		}
		printf("same or different frequency command:%d\n", cmd);

		if(cmd == INTRA_OR_INTER_MODEL)	{
			band_and_pa_ctrl(cmd, cli_sfd);
			goto go_return;
		} else {
			band_and_pa_ctrl(cmd, cli_sfd);
			/* update status in databases */
			wr_band_pa_file(cmd);
			set_base_band_online_status(cmd);
		}
		wrFLLmtToEnbSysArfcnCfg_T pStr;
		memset(&pStr, '\0', sizeof(pStr));
		WrmsgHeader.u16MsgType = O_FL_LMT_TO_ENB_SYS_ARFCN_CFG;
		WrmsgHeader.u16MsgLength = sizeof(pStr);
#ifdef BIG_END
		WrmsgHeader.u16MsgType = my_htons_ntohs(WrmsgHeader.u16MsgType); 
		WrmsgHeader.u16MsgLength = my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
		pStr.WrmsgHeaderInfo = WrmsgHeader;
		/* Set Pilot A/B */
		if (cmd == INTER_A_WORK_MODEL || cmd == INTER_B_WORK_MODEL)
		{
			/* find 36 and set Pilot arfcn */
			band_entry_t *entry
				= band_list_entry_search(BAND_36_IPADDR);
			if (entry) {
				pStr.sysUlARFCN = default_config[1].sysUlARFCN;
				pStr.sysDlARFCN = default_config[1].sysDlARFCN;
				strcpy((S8 *)(pStr.PLMN),(S8 *)(default_config[1].PLMN));
				pStr.sysBandwidth = default_config[1].sysBandwidth;
				pStr.sysBand = default_config[1].sysBand;
				pStr.PCI = default_config[1].PCI;
				pStr.TAC = default_config[1].TAC;
				pStr.CellId = default_config[1].CellId;
				pStr.UePMax = default_config[1].UePMax;
				pStr.EnodeBPMax = default_config[1].EnodeBPMax;
				if (entry->work_mode == TDD)
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_TDD;
				else
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_FDD;
#ifdef BIG_END
				pStr.sysUlARFCN = my_htonl_ntohl(pStr.sysUlARFCN);
				pStr.sysDlARFCN = my_htonl_ntohl(pStr.sysDlARFCN);
				pStr.sysBand = my_htonl_ntohl(pStr.sysBand);
				pStr.PCI = my_htons_ntohs(pStr.PCI);
				pStr.TAC = my_htons_ntohs(pStr.TAC);
				pStr.CellId = my_htonl_ntohl(pStr.CellId);
				pStr.UePMax = my_htons_ntohs(pStr.UePMax);
				pStr.EnodeBPMax = my_htons_ntohs(pStr.EnodeBPMax);
				pStr.WrmsgHeaderInfo.u16frame
					= my_htons_ntohs(pStr.WrmsgHeaderInfo.u16frame);
#endif
				//intral A mode, we need to cfg IP 36, but for Intral B mode, 36 and 37 will work on 1650 or 1825 earfcn
				if(INTER_A_WORK_MODEL == cmd)
					assist_plmn_config(entry,ADD_ACTION);

				memcpy(send_request, &pStr, sizeof(pStr));
				sleep(1);
				baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
			}
			/* find 38 and set Pilot arfcn */
			entry = band_list_entry_search(BAND_38_IPADDR);
			if (entry)
			{
				pStr.sysUlARFCN = default_config[4].sysUlARFCN;
				pStr.sysDlARFCN = default_config[4].sysDlARFCN;
				strcpy((S8 *)(pStr.PLMN),
					(S8 *)(default_config[4].PLMN));
				pStr.sysBandwidth
					= default_config[4].sysBandwidth;
				pStr.sysBand = default_config[4].sysBand;
				pStr.PCI = default_config[4].PCI;
				pStr.TAC = default_config[4].TAC;
				pStr.CellId = default_config[4].CellId;
				pStr.UePMax = default_config[4].UePMax;
				pStr.EnodeBPMax = default_config[4].EnodeBPMax;
				if (entry->work_mode == TDD)
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_TDD;
				else
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_FDD;
#ifdef BIG_END
				pStr.sysUlARFCN
					= my_htonl_ntohl(pStr.sysUlARFCN);
				pStr.sysDlARFCN
					= my_htonl_ntohl(pStr.sysDlARFCN);
				pStr.sysBand = my_htonl_ntohl(pStr.sysBand);
				pStr.PCI = my_htons_ntohs(pStr.PCI);
				pStr.TAC = my_htons_ntohs(pStr.TAC);
				pStr.CellId = my_htonl_ntohl(pStr.CellId);
				pStr.UePMax = my_htons_ntohs(pStr.UePMax);
				pStr.EnodeBPMax
					= my_htons_ntohs(pStr.EnodeBPMax);
				pStr.WrmsgHeaderInfo.u16frame
					= my_htons_ntohs(pStr.WrmsgHeaderInfo.u16frame);
#endif
				//only intra A we need to set the aux plmn
				if(INTER_A_WORK_MODEL == cmd)
					assist_plmn_config(entry,ADD_ACTION);

				memcpy(send_request, &pStr, sizeof(pStr));
				sleep(1);
				baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
			} else {
				printf("search entry by "
					"ip(192.168.2.38) failed!\n");
			}

			/* Set same mode */
		} else if (cmd == INTRA_WORK_MODEL) {
			/* find 36 and set same arfcn */
			band_entry_t *entry
				= band_list_entry_search(BAND_36_IPADDR);
			if (entry) {
				pStr.sysUlARFCN = default_config[0].sysUlARFCN;
				pStr.sysDlARFCN = default_config[0].sysDlARFCN;
				strcpy((S8 *)(pStr.PLMN),(S8 *)(default_config[0].PLMN));
				pStr.sysBandwidth = default_config[0].sysBandwidth;
				pStr.sysBand = default_config[0].sysBand;
				pStr.PCI = default_config[0].PCI;
				pStr.TAC = default_config[0].TAC;
				pStr.CellId = default_config[0].CellId;
				pStr.UePMax = default_config[0].UePMax;
				pStr.EnodeBPMax = default_config[0].EnodeBPMax;
				if (entry->work_mode == TDD) {
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_TDD;
				} else {
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_FDD;
				}
#ifdef BIG_END
				pStr.sysUlARFCN
					= my_htonl_ntohl(pStr.sysUlARFCN);
				pStr.sysDlARFCN
					= my_htonl_ntohl(pStr.sysDlARFCN);
				pStr.sysBand = my_htonl_ntohl(pStr.sysBand);
				pStr.PCI = my_htons_ntohs(pStr.PCI);
				pStr.TAC = my_htons_ntohs(pStr.TAC);
				pStr.CellId = my_htonl_ntohl(pStr.CellId);
				pStr.UePMax = my_htons_ntohs(pStr.UePMax);
				pStr.EnodeBPMax
					= my_htons_ntohs(pStr.EnodeBPMax);
				pStr.WrmsgHeaderInfo.u16frame
					= my_htons_ntohs(pStr.WrmsgHeaderInfo.u16frame);
#endif
				//clear the aux plmn if we have ever set for B36
				assist_plmn_config(entry, CLR_ACTION);

				//cfg plmn
				memcpy(send_request, &pStr, sizeof(pStr));
				sleep(1);
				baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
			}
			/* find 38 and set same arfcn */
			entry = band_list_entry_search(BAND_38_IPADDR);
			if (entry) {
				pStr.sysUlARFCN = default_config[3].sysUlARFCN;
				pStr.sysDlARFCN = default_config[3].sysDlARFCN;
				strcpy((S8 *)(pStr.PLMN),(S8 *)(default_config[3].PLMN));
				pStr.sysBandwidth = default_config[3].sysBandwidth;
				pStr.sysBand = default_config[3].sysBand;
				pStr.PCI = default_config[3].PCI;
				pStr.TAC = default_config[3].TAC;
				pStr.CellId = default_config[3].CellId;
				pStr.UePMax = default_config[3].UePMax;
				pStr.EnodeBPMax = default_config[3].EnodeBPMax;
				if (entry->work_mode == TDD)
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_TDD;
				else
					pStr.WrmsgHeaderInfo.u16frame
						= SYS_WORK_MODE_FDD;
#ifdef BIG_END
				pStr.sysUlARFCN
					= my_htonl_ntohl(pStr.sysUlARFCN);
				pStr.sysDlARFCN
					= my_htonl_ntohl(pStr.sysDlARFCN);
				pStr.sysBand = my_htonl_ntohl(pStr.sysBand);
				pStr.PCI = my_htons_ntohs(pStr.PCI);
				pStr.TAC = my_htons_ntohs(pStr.TAC);
				pStr.CellId = my_htonl_ntohl(pStr.CellId);
				pStr.UePMax = my_htons_ntohs(pStr.UePMax);
				pStr.EnodeBPMax
					= my_htons_ntohs(pStr.EnodeBPMax);
				pStr.WrmsgHeaderInfo.u16frame
					= my_htons_ntohs(pStr.WrmsgHeaderInfo.u16frame);
#endif
				//clear the aux plmn if we have ever set for B38
				assist_plmn_config(entry,CLR_ACTION);

				memcpy(send_request, &pStr, sizeof(pStr));
				sleep(1);
				baicell_send_msg_to_band(send_request, sizeof(pStr), entry->ipaddr);
			} else {
				printf("search entry "
					"by ip(192.168.2.38)failed!\n");
			}
		} /* else if (cmd == INTRA_WORK_MODEL) */
		goto go_return;
	}
	case O_SERVER_TO_BAND_UPDATE_FILE://(E005)下载服务器指定文件并进行操作
	{
		printf("download file and action !\n");
		cJSON *f_type = cJSON_GetObjectItem(pa_object, "f_type");
		cJSON *f_name = cJSON_GetObjectItem(pa_object, "f_name");
		cJSON *f_md5 = cJSON_GetObjectItem(pa_object, "f_md5");
		cJSON *d_url = cJSON_GetObjectItem(pa_object, "d_url");
		cJSON *d_type = cJSON_GetObjectItem(pa_object, "d_type");
		cJSON *uname = cJSON_GetObjectItem(pa_object, "uname");
		cJSON *passwd = cJSON_GetObjectItem(pa_object, "passwd");
		if (!f_name || !f_md5 || !d_url || !d_type || !f_type) break;
		update_downloadfile(d_url->valuestring,
			f_name->valuestring,
			f_md5->valuestring,
			atoi(f_type->valuestring),
			atoi(d_type->valuestring),
			uname->valuestring,
			passwd->valuestring);
		goto go_return;
	}
	case O_SET_NOT_SEND_CLIENT_IP: /* (0xE006) 配置取消传输client信息的服务器IP*/
	{
		printf("Set not send client server IP (E006)\n");
		cJSON *ip_num = cJSON_GetObjectItem(pa_object, "num");
		cJSON *ip_list = cJSON_GetObjectItem(pa_object, "list");
		if (!ip_num || atoi(ip_num->valuestring) == 0) {
			remove(NOT_SEND_CLI_SERVER_FILE);
		} else {
			S32 array_num = cJSON_GetArraySize(ip_list);
			if (array_num != atoi(ip_num->valuestring)) {
				printf("list num is failed!\n");
			} else {
				S32 nu;
				S8 *write_l[10] = {NULL};
				for (nu = 0; nu < array_num; nu++) {
					cJSON *s =
						cJSON_GetArrayItem(ip_list, nu);
					write_l[nu] = strdup(s->valuestring);
				}
				FILE *wfd
					= fopen(NOT_SEND_CLI_SERVER_FILE, "w");
				if (wfd) {
					for (nu = 0; nu < array_num; nu++) {
						fprintf(wfd, "%s\n", write_l[nu]);
						free(write_l[nu]);
					}
					fclose(wfd);
				}
			}
		}
		init_http_send_client_server_list();
		goto go_return;
	}
	case O_FL_ENB_TO_LMT_CUTNET_CONFIG_QUERY:	// (0xF045) 查询断网配置文件内容
		{
			char buf[256] = {0};
			if (!access(CUTNET_CONFIG, F_OK)) {
				FILE *cutnet_file = fopen(CUTNET_CONFIG, "r");
				if(!cutnet_file) {
					printf("open %s failuer\n",
						CUTNET_CONFIG);
					break;
				}
				if (!fread(buf, 1, 256, cutnet_file)) {
					snprintf(buf, 8, "nothing");
				}
			} else {
				snprintf(buf, 8, "nothing");
			}

			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);

			if (strcmp(buf, "nothing")) {
				cJSON *msg = cJSON_Parse(buf);
				cJSON *mode = cJSON_GetObjectItem(msg, "work_mode");
				cJSON *power = cJSON_GetObjectItem(msg, "powerderease");
				cJSON *gain = cJSON_GetObjectItem(msg, "rxgain");

				cJSON_AddStringToObject(object, "work_mode", mode->valuestring);
				cJSON_AddStringToObject(object, "powerderease", power->valuestring);
				cJSON_AddStringToObject(object, "rxgain", gain->valuestring);
			} else {
				cJSON_AddStringToObject(object, "work_mode", "nothing");
				cJSON_AddStringToObject(object, "powerderease", "nothing");
				cJSON_AddStringToObject(object, "rxgain", "nothing");
			}
			char *send_str= cJSON_PrintUnformatted(object);
			cJSON_Delete(object);

			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.cut_network_api_arg, send_str,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			goto go_return;
		}
	case O_FL_LMT_TO_ENB_CUTNET_CONFIG_SETUP:	// (0xF046) 配置断网配置文件内容
		{
			
			cJSON *mode = cJSON_GetObjectItem(pa_object, "work_mode");
			cJSON *power = cJSON_GetObjectItem(pa_object, "powerderease");
			cJSON *gain = cJSON_GetObjectItem(pa_object, "rxgain");
/* 
			FILE *cutnet_con = fopen(CUTNET_CONFIG, "r+");
			if(!cutnet_con){
				printf("open %s failuer\n", CUTNET_CONFIG);
				return;
			}
			fread(buf, 1, 256, cutnet_con);
			fclose(cutnet_con);
*/
			printf("\nmode: %s \npowerderease: %s \n rxgain: %s\n",
				mode->valuestring, power->valuestring,
				gain->valuestring);
			cJSON *cutnet_msg = cJSON_CreateObject();
			if(!cutnet_msg)
				break;
			if(strcmp(mode->valuestring, "nothing")){
				cJSON_AddStringToObject(cutnet_msg,
					"work_mode", mode->valuestring);
			}
			if(strcmp(power->valuestring, "nothing")){
				cJSON_AddStringToObject(cutnet_msg,
					"powerderease", power->valuestring);
			}
			if(strcmp(power->valuestring, "nothing")){
				cJSON_AddStringToObject(cutnet_msg,
					"rxgain", gain->valuestring);
			}
			char *str = cJSON_Print(cutnet_msg);
			FILE *cutnet_file = fopen(CUTNET_CONFIG, "w");
			if(!cutnet_file){
				printf("open %s failuer\n", CUTNET_CONFIG);
				return;
			}
			if(!fwrite(str, 1, strlen(str), cutnet_file)){
				printf("write failure\n");
				exit(-1);
			}
			fclose(cutnet_file);
			free(str);
			goto go_return;
	}
	case O_SERVER_TO_BASE_MASTER_GPIO_REBOOT_BAND_CMD: // (0xF099) 硬件操控基带板电源
	{
		printf("hardware ctrl band power request\n");
#ifndef VMWORK
#ifdef SYS86
		if (!bandip) {
			printf("Not founnd 'bandip'\n");
			goto go_return;
		}
		cJSON *control_type = cJSON_GetObjectItem(pa_object, "cmd");
		if (!control_type) goto go_return;
		U32 c_command;
		sscanf(control_type->valuestring, "%u", &c_command);

		write_action_log("MsgTyep: 0xF099, work mode cfg cmd:",
			&c_command);

		if (c_command == LEVEL_LOW) {
			printf("OFF band:%s\n",	bandip->valuestring);
		} else if (c_command == LEVEL_HIG) {
			printf("ON band:%s\n",bandip->valuestring);
		} else if (c_command == LEVEL_RES) {
			printf("restart band:%s\n", bandip->valuestring);
		} else {
			printf("band(IP:%s)ctrl err:%d\n",
				bandip->valuestring, c_command);
			goto go_return;
		}

		S32 gpio_num = get_base_switch_gpio_by_ip(bandip->valuestring);
		//record action log
		char logStr[200] = {0};
		sprintf(logStr,"IP %s,operate GPIO Num: %d",
			bandip->valuestring, gpio_num);
		write_action_log("MsgTyep: 0xF099:", logStr);
		printf("gpio_num:%d\n", gpio_num);

		//operate GPIO to control corresponding baseBand
		switch_gpio(gpio_num, c_command);
#ifdef CLI
		if ((cli_sfd != -1)) {
			char cli_res[64] = {0};
			if (c_command == LEVEL_LOW) {
				snprintf(cli_res, 64, "hard turn OFF band:%s\n",
					bandip->valuestring);
			} else if (c_command == LEVEL_HIG) {
				snprintf(cli_res, 64, "hard turn ON band:%s\n",
					bandip->valuestring);
			} else if (c_command == LEVEL_RES) {
				snprintf(cli_res, 64, "hard restart band:%s\n",
					bandip->valuestring);
			}
			write(cli_sfd, cli_res, strlen(cli_res));
			close(cli_sfd);
			cli_sfd = -1;
		}
#endif

		pthread_mutex_lock(&set_band_pa_file_lock);
		if(c_command == 0) {
			state_band_pa.band_state[gpio_num] = GPIO_PORT_DISABLE;
		} else if(c_command == 1 || c_command == 2) {
			state_band_pa.band_state[gpio_num] = GPIO_PORT_ENABLE;
		}
		//!todo,nyb,2018.04.06,below action why?
		U8 set_num = 4;
		wr_band_pa_file(set_num);
		pthread_mutex_unlock(&set_band_pa_file_lock);
#endif
#endif
		goto go_return;
	}
	case O_SERVER_TO_BASE_MASTER_ANT_CONFIG://(0xE003) 功放操作
	{
		printf("pa ctrl\n");
#ifndef VMWORK
#ifdef SYS86
		if (!bandip) {
			printf("Not founnd 'bandip'\n");
			goto go_return;
		}
		S32 cmd_in = 0;
		cJSON *command = cJSON_GetObjectItem(pa_object, "command");
		if (command) {
			sscanf(command->valuestring, "%d", &cmd_in);
		}
		U8 ma = get_base_pa_addr_by_ip(bandip->valuestring);
		send_serial_request(ma,	cmd_in);

		pthread_mutex_lock(&set_band_pa_file_lock);
		if(cmd_in == 2) {
			state_band_pa.pa_state[ma] = PA_DISABLE;
		} else if(cmd_in == 1) {
			state_band_pa.pa_state[ma] = PA_ENABLE;
		}
		//!todo: orignal value 4, here we config it as 0xff
		U8 set_num = 4;
		wr_band_pa_file(set_num);
		pthread_mutex_unlock(&set_band_pa_file_lock);
#endif
#endif
		goto go_return;
	}
	default:
		break;
	}/* end switch(msgt) */

	/* if the msg is set base_band device config
	 * bandip need not nil
	 * */
	if (!bandip) {
		printf("Not founnd 'bandip' in buffer from mqtt\n");
		goto go_return;
	}
	/* search lte base_band entry from band list */
	band_entry_t *entry = band_list_entry_search(bandip->valuestring);
	if (!entry) {
		printf("search entry by %s failed!\n", bandip->valuestring);
#ifdef CLI
		if ((cli_sfd != -1)) {
			char no_ip[128] = {0};
			snprintf(no_ip, 128, "sorry, no search ip(%s),"
				"please check it and try again!\n",
				bandip->valuestring);
			write(cli_sfd, no_ip, strlen(no_ip) + 1);
			close(cli_sfd);
		}
#endif
		goto go_return;
	}
#ifdef CLI
	/* check cli fd , if fd != -1, need send some value to cli
	 * or add cmd to cli check command list
	 * */
	if ((cli_sfd != -1)) {
		if (entry->online == 0) {
			char *offline_ret =
				"sorry, you can not config to offlinedevice!";
			write(cli_sfd, offline_ret, strlen(offline_ret) + 1);
			close(cli_sfd);
		} else {
			struct cli_req_if info;
			info.sockfd = cli_sfd;
			info.msgtype = msgt + 1;
			snprintf((S8 *)(info.ipaddr), 20, "%s",
				bandip->valuestring);
			add_entry_to_clireq_list(info);
		}
	}
#endif
	if (entry->work_mode == FIBERHOME_BASE_BAND_WMODE) {
		printf("Config To FiberHome base band device!\n");
		/* 烽火基带板采用独立的配置模式 */
		fiberhome_pare_config_json_message(my_msg, entry);
		goto go_return;
	} else if (entry->work_mode == WCDMA_BASE_DEVICE) {
		printf("Config To WCDMA base band device!\n");
		/* WCDMA 基带板采用独立配置模式 */
		wcdma_pare_config_json_message(my_msg, entry);
		goto go_return;
	}
	if (entry->work_mode == TDD) {
		WrmsgHeader.u16frame = SYS_WORK_MODE_TDD;
	} else {
		WrmsgHeader.u16frame = SYS_WORK_MODE_FDD;
	}
	WrmsgHeader.u16MsgType = msgt;
#ifdef BIG_END
	WrmsgHeader.u16frame = my_htons_ntohs(WrmsgHeader.u16frame);
	WrmsgHeader.u16MsgType = my_htons_ntohs(WrmsgHeader.u16MsgType); 
#endif
	U8 send_len = 0;
	/* check and compose config request to base band device */
	switch(msgt) {
	case O_FL_LMT_TO_ENB_SYS_ARFCN_CFG: //(0xF003) 3.3 频点配置
		{
			printf("<---- Arfcn config\n");
			write_action_log(bandip->valuestring, "Arfcn config request!");
			cJSON *sysuiarfcn = cJSON_GetObjectItem(pa_object, "sysuiarfcn");
			cJSON *sysdiarfcn = cJSON_GetObjectItem(pa_object, "sysdiarfcn");
			cJSON *plmn = cJSON_GetObjectItem(pa_object, "plmn");
			cJSON *sysbandwidth = cJSON_GetObjectItem(pa_object, "sysbandwidth");
			cJSON *sysband = cJSON_GetObjectItem(pa_object, "sysband");
			cJSON *pci = cJSON_GetObjectItem(pa_object, "pci");
			cJSON *tac = cJSON_GetObjectItem(pa_object, "tac");
			cJSON *cellid = cJSON_GetObjectItem(pa_object, "cellid");
			cJSON *uepmax = cJSON_GetObjectItem(pa_object, "uepmax");
			cJSON *enodebpmax = cJSON_GetObjectItem(pa_object, "enodebpmax");
			wrFLLmtToEnbSysArfcnCfg_T pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			if (!sysuiarfcn	|| !sysdiarfcn || !plmn || !sysband ||
				!cellid || !uepmax || !enodebpmax || !pci ||
				!tac || !sysbandwidth)
				goto go_return;
			sscanf(sysuiarfcn->valuestring, "%u", &(pStr.sysUlARFCN)); //U32
			sscanf(sysdiarfcn->valuestring, "%u", &(pStr.sysDlARFCN)); //U32
			sscanf(plmn->valuestring, "%s", pStr.PLMN); //U8 [7]
			sscanf(sysband->valuestring, "%u", &(pStr.sysBand)); //U32
			sscanf(cellid->valuestring, "%u", &(pStr.CellId));//U32

			U32 PCI_t = 0, TAC_t = 0, sys_b = 0;
			U32 UePMax_t = 0, EnodeBPMax_t = 0;
			sscanf(pci->valuestring, "%u", &(PCI_t));
			sscanf(tac->valuestring, "%u", &(TAC_t));
			sscanf(uepmax->valuestring, "%u", &(UePMax_t));
			sscanf(enodebpmax->valuestring, "%u", &(EnodeBPMax_t));
			sscanf(sysbandwidth->valuestring, "%u", &sys_b);

			pStr.PCI = PCI_t;
			pStr.TAC = TAC_t;
			pStr.UePMax = UePMax_t;
			pStr.EnodeBPMax = EnodeBPMax_t;
			pStr.sysBandwidth = sys_b;
#ifdef BIG_END

			pStr.sysUlARFCN = my_htonl_ntohl(pStr.sysUlARFCN);
			pStr.sysDlARFCN = my_htonl_ntohl(pStr.sysDlARFCN);
			pStr.sysBand = my_htonl_ntohl(pStr.sysBand);
			pStr.PCI = my_htons_ntohs(pStr.PCI);
			pStr.TAC = my_htons_ntohs(pStr.TAC);
			pStr.CellId = my_htonl_ntohl(pStr.CellId);
			pStr.UePMax = my_htons_ntohs(pStr.UePMax);
			pStr.EnodeBPMax = my_htons_ntohs(pStr.EnodeBPMax);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_FREQ_OFFSET_CFG: //(0xF059) 3.68. 初始频偏配置
		{
			printf("0xF059 3.68 initial frequency offset config\n");
			write_action_log(bandip->valuestring,
				"3.68 initial frequency offset config request!");
			wrFLLmtToEnbFreqOffsetCfg_t pStr;
			memset(&pStr,'\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			cJSON *freeoffset = cJSON_GetObjectItem(pa_object, "freq_offset");
			sscanf(freeoffset->valuestring, "%u", &(pStr.FreqOffset));
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
			pStr.FreqOffset = my_htonl_ntohl(pStr.FreqOffset);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_UPDATE_SOFT_VERSION_CFG://3.79. 基站版本升级配置(0xF06F)
		{
			printf("3.79 base station version update\n");
			write_action_log(bandip->valuestring,
				"3.79 base station version update request!");
			cJSON *updatetype =
				cJSON_GetObjectItem(pa_object, "update_type");
			cJSON *updatefilename =
				cJSON_GetObjectItem(pa_object, "update_filename");
			cJSON *isreservedcfg =
				cJSON_GetObjectItem(pa_object, "is_reserved_cfg");
			cJSON *iscfgftpserver =
				cJSON_GetObjectItem(pa_object, "is_cfg_ftp_server");
			cJSON *ftpserverip =
				cJSON_GetObjectItem(pa_object, "ftp_serverip");
			cJSON *ftpserverport =
				cJSON_GetObjectItem(pa_object, "ftp_serverport");
			cJSON *ftploginname =
				cJSON_GetObjectItem(pa_object, "ftp_loginname");
			cJSON *ftppassword =
				cJSON_GetObjectItem(pa_object, "ftp_password");
			cJSON *ftpserverfilepath =
				cJSON_GetObjectItem(pa_object, "ftp_serverfilepath");
			if (!updatetype || !updatefilename || !isreservedcfg
				|| !iscfgftpserver || !ftpserverip
				|| !ftpserverport || !ftploginname
				|| !ftppassword || !ftpserverfilepath) {
				printf("update info absence,not set\n");
				break;
			}
			wrFLLmtToEnbUpdateSoftVersionCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);

			U32 ty = 0, dcfg = 0, ftps = 0;
			sscanf(updatetype->valuestring, "%x", &ty);
			sscanf(isreservedcfg->valuestring, "%x", &dcfg);
			sscanf(iscfgftpserver->valuestring, "%x", &ftps);
			/* U8 */
			pStr.updateType = ty;
			pStr.isReservedCfg = dcfg;
			pStr.isCfgFtpServer = ftps;

			sscanf(updatefilename->valuestring, "%s", pStr.updateFileName);
			sscanf(ftpserverip->valuestring, "%s", pStr.FtpServerIp);
			sscanf(ftpserverport->valuestring, "%u", &(pStr.FtpServerPort));
			sscanf(ftploginname->valuestring, "%s", pStr.FtpLoginNam);
			sscanf(ftppassword->valuestring, "%s", pStr.FtpPassword);
			sscanf(ftpserverfilepath->valuestring, "%s", pStr.FtpServerFilePath);
#ifdef BIG_END
			pStr.FtpServerPort = my_htonl_ntohl(pStr.FtpServerPort);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;

			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SYNCARFCN_CFG:// (0xF05E) 3.73异频同步频点及 band 配置
		{
			printf("(0xF05E) 3.73 different-frequency frequency point and band config\n");
			write_action_log(bandip->valuestring,
				"3.73 different-frequency frequency point and band config request");
			cJSON *syncarfcn = cJSON_GetObjectItem(pa_object, "sync_arfcn"); 
			cJSON *syncband = cJSON_GetObjectItem(pa_object, "sync_band");
			if (!syncarfcn || !syncband) break;

			FLLmtToEnbSyncarfcnCfg_t pStr;
			memset(&pStr, '\0',sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(syncarfcn->valuestring, "%u", &(pStr.SyncArfcn));
			sscanf(syncband->valuestring, "%u", &(pStr.SyncBand));
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
			pStr.SyncArfcn = my_htonl_ntohl(pStr.SyncArfcn);
			pStr.SyncBand = my_htonl_ntohl(pStr.SyncBand);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;

			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_GET_ENB_LOG://(0xF071) 3.81. 获取基站Log
		{
			printf("get base station Log\n");
			write_action_log(bandip->valuestring,
				"3.81 get base station Log request");

			cJSON *cfgftpserver =
				cJSON_GetObjectItem(pa_object, "cfg_ftpserver");
			cJSON *ftpserverip =
				cJSON_GetObjectItem(pa_object, "ftp_serverip");
			cJSON *reserved =
				cJSON_GetObjectItem(pa_object, "reserved");
			cJSON *ftpserverport =
				cJSON_GetObjectItem(pa_object, "ftp_serverport");
			cJSON *ftploginname =
				cJSON_GetObjectItem(pa_object, "ftp_loginname");
			cJSON *ftppassword =
				cJSON_GetObjectItem(pa_object, "ftp_password");
			cJSON *ftpserverfilepath =
				cJSON_GetObjectItem(pa_object, "ftp_serverfilepath");
			if (!cfgftpserver
				|| !ftpserverip || !ftpserverport
				|| !ftploginname || !ftppassword
				|| !ftpserverfilepath) {
				break;
			}
			wrFLLmtToEnbGetEnbLog_t pStr;
			memset(&pStr, '\0',sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);

			sscanf(ftpserverport->valuestring, "%u", &(pStr.FtpServerPort));
			memcpy(pStr.FtpServerIp, ftpserverip->valuestring,
				strlen(ftpserverip->valuestring));
			memcpy(pStr.FtpPassword, ftppassword->valuestring,
				strlen(ftppassword->valuestring));
			memcpy(pStr.FtpLoginNam, ftploginname->valuestring,
				strlen(ftploginname->valuestring));
			memcpy(pStr.FtpServerFilePath,
				ftpserverfilepath->valuestring,
				strlen(ftpserverfilepath->valuestring));
			U32 isconfig = 0;
			sscanf(cfgftpserver->valuestring, "%x", &isconfig);
			pStr.isCfgFtpServer = isconfig;
#ifdef BIG_END
			pStr.FtpServerPort = my_htonl_ntohl(pStr.FtpServerPort);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;

			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_MEAS_UE_CFG: //(0xF006) 4.1. 设置基站测量 UE 配置
		{
			printf("base station UE config\n");
			write_action_log(bandip->valuestring,
				"4.1 base station UE config request!");
			wrFLLmtToEnbMeasUecfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			cJSON *work_mode =
				cJSON_GetObjectItem(pa_object, "work_mode");
			if (!work_mode) {
				break;
			}
			/* 周期 */
			cJSON *capture_period =
				cJSON_GetObjectItem(pa_object, "capture_period");
			/* 管控 */
			cJSON *control_submode =
				cJSON_GetObjectItem(pa_object,"control_submode");
			/* 定位 */
			cJSON *gst_imsi =
				cJSON_GetObjectItem(pa_object, "dst_imsi");
			cJSON *report_itv =
				cJSON_GetObjectItem(pa_object, "report_itv");
			cJSON *max_txflag =
				cJSON_GetObjectItem(pa_object, "max_txflag");
			cJSON *ue_max_txpower =
				cJSON_GetObjectItem(pa_object, "ue_max_txpower");
			cJSON *posi_flag =
				cJSON_GetObjectItem(pa_object, "posi_flag");
			cJSON *cap_flag =
				cJSON_GetObjectItem(pa_object, "cap_flag");

			pStr.u8WorkMode = atoi(work_mode->valuestring);
			if (pStr.u8WorkMode == 1) {/* 周期抓号模式 */
				if (!capture_period) break;
				pStr.u16CapturePeriod =
					atoi(capture_period->valuestring);
			} else if (pStr.u8WorkMode == 2) {/* 定位模式 */
				if (!gst_imsi || !report_itv || !max_txflag
					|| !ue_max_txpower || !posi_flag
					|| !cap_flag) {
					break;
				}
				memcpy(pStr.IMSI, gst_imsi->valuestring,
					strlen(gst_imsi->valuestring));
				pStr.u8MeasReportPeriod =
					atoi(report_itv->valuestring);
				pStr.SchdUeMaxPowerTxFlag =
					atoi(max_txflag->valuestring);
				pStr.SchdUeMaxPowerValue =
					atoi(ue_max_txpower->valuestring);
				pStr.SchdUeUlFixedPrbSwitch =
					atoi(posi_flag->valuestring);
				pStr.CampOnAllowedFlag =
					atoi(cap_flag->valuestring);
			} else if (pStr.u8WorkMode == 3) {/* 管控模式 */
				if (!control_submode) break;
				pStr.u8ControlSubMode =
					atoi(control_submode->valuestring);
			}
#ifdef BIG_END
			pStr.u16CapturePeriod =
				my_htons_ntohs(pStr.u16CapturePeriod);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
			sscanf(control_submode->valuestring, "%u",
				(U32 *)&(pStr.u8ControlSubMode));
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			int ZZ = 0;
			for (ZZ = 0; ZZ < send_len; ZZ++) {
				printf("%02X\n", send_request[ZZ]);
			}
			break;
		}
	case O_FL_LMT_TO_ENB_REM_CFG: //(0xF009)3.6 系统扫频配置
		{
			printf("system scan-frequency config\n");
			write_action_log(bandip->valuestring,
				"3.6 system scan-frequency config request!");
			cJSON *band_rem = cJSON_GetObjectItem(pa_object, "band_rem");
			if (!band_rem) break;
			cJSON *arfcn_num = cJSON_GetObjectItem(pa_object, "arfcn_num");
			cJSON *sysarfcn = cJSON_GetObjectItem(pa_object, "sys_arfcn");

			wrFLLmtToEnbRemcfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(band_rem->valuestring, "%u", &(pStr.wholeBandRem));
			sscanf(arfcn_num->valuestring, "%u", &(pStr.sysArfcnNum));
			int count = cJSON_GetArraySize(sysarfcn);
			int i;
			printf("arfcn number is %d\n", count);
			for (i = 0; i< count; i++) {
				cJSON *arf = cJSON_GetArrayItem(sysarfcn, i);
				if (!arf) continue;
				sscanf(arf->valuestring, "%u", &(pStr.sysArfcn[i]));
#ifdef BIG_END
				pStr.sysArfcn[i] = my_htonl_ntohl(pStr.sysArfcn[i]);
#endif
				printf("sysarfcn[%d]-%u\n", i, pStr.sysArfcn[i]);
			}
#ifdef BIG_END
			pStr.wholeBandRem = my_htonl_ntohl(pStr.wholeBandRem);
			pStr.sysArfcnNum = my_htonl_ntohl(pStr.sysArfcnNum);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SET_ADMIN_STATE_CFG: //(0xF00D) 3.10 小区激活去激活配置 1-激活小区，0-去激活小区
		{
			printf("cell activation or deactivation config\n");
			write_action_log(bandip->valuestring,
				"3.10 cell activation or deactivation config request!");
			/* GSM */
			if(!strcmp(entry->ipaddr, GSM_IP_ADDR)) {
				printf("set GSM FR ON/OFF\n");
				U32 work_admin_state;
				U8 carrier_type;

				cJSON *work_admin_state_cjson =
					cJSON_GetObjectItem(pa_object,
						"work_admin_state");
				if (!work_admin_state_cjson)
					goto go_return;
				cJSON *car_type = cJSON_GetObjectItem(pa_object,
						"gsm_carrier");
				if (car_type) {
					if (car_type->type == cJSON_Number) {
						carrier_type = car_type->valueint;
					} else if (car_type->type == cJSON_String) {
						sscanf(car_type->valuestring, "%u",
							(U32 *)&(carrier_type));
					}
				}
				sscanf(work_admin_state_cjson->valuestring,
					"%d", &work_admin_state);
				if(work_admin_state == 1)
					WrtogsmHeader.msg_type = O_SEND_TO_GSM_ON_RF;
				else if(work_admin_state == 0)
					WrtogsmHeader.msg_type = O_SEND_TO_GSM_OFF_RF;
				WrtogsmHeader.msg_action = 0;
				WrtogsmHeader.msg_len = sizeof(WrtogsmHeader);

				handle_send_to_gsm(&WrtogsmHeader,
					(carrier_type == 2)?GSM_CARRIER_TYPE_ALL:
					((carrier_type == 0)?GSM_CARRIER_TYPE_MOBILE:
					GSM_CARRIER_TYPE_UNICOM));
#ifdef CLI
				char no_info_str[32] = {0};
				snprintf(no_info_str, 32,
					"RF switch config success");
				struct cli_req_if cli_info;
				snprintf((S8 *)(cli_info.ipaddr), 20, "%s", GSM_IP_ADDR);
				cli_info.msgtype = O_FL_LMT_TO_ENB_SET_ADMIN_STATE_CFG + 1;
				cli_info.sockfd = -1;
				cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
				if (cli_entry) {
					write(cli_entry->info.sockfd,
						no_info_str,
						strlen(no_info_str) + 1);
					close(cli_entry->info.sockfd);
					del_entry_to_clireq_list(cli_entry);
				}
#endif
				goto go_return;
			} /* end if */
			/* End GSM */
			cJSON *work_admin_state =
				cJSON_GetObjectItem(pa_object, "work_admin_state");
			if (!work_admin_state) break;

			/* LTE */
			wrFLLmtToEnbSetAdminStateCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			U32 sta = 0;
			sscanf(work_admin_state->valuestring, "%u", &(sta));
			if (!sta)
				pStr.workAdminState = 0;
			else
				pStr.workAdminState = 1;
#ifdef BIG_END
			pStr.workAdminState
				= my_htonl_ntohl(pStr.workAdminState);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;

			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SYS_RxGAIN_CFG:  //(0xF013) 3.14 增益配置
		{
			printf("set rxgain config\n");
			write_action_log(bandip->valuestring,
				"3.14 set rxgain config request!");
			cJSON *rxgain = cJSON_GetObjectItem(pa_object, "rxgain");
			cJSON *is_save = cJSON_GetObjectItem(pa_object, "is_save");
			if (!rxgain) break;

			wrFLLmtToEnbSysRxGainCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			if (!is_save) {
				pStr.RxGainSaveFlag = 1;
			} else {
				U32 save_t = 0;
				sscanf(is_save->valuestring, "%x", (U32 *)&save_t);
				pStr.RxGainSaveFlag = save_t;
			}
			sscanf(rxgain->valuestring, "%u", &(pStr.Rxgain));
#ifdef BIG_END
			pStr.Rxgain = my_htonl_ntohl(pStr.Rxgain);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SYS_PWR1_DEREASE_CFG: //(0xF015) 3.16 发射功率配置
		{
			printf("set txpower config\n");
			write_action_log(bandip->valuestring,
				"3.16 set txpower config request!");
			cJSON *powerderease =
				cJSON_GetObjectItem(pa_object, "powerderease");
			cJSON *is_saved =
				cJSON_GetObjectItem(pa_object, "is_saved");
			if (!powerderease) break;
			/* LTE */
			wrFLLmtToEnbSysPwr1DegreeCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(powerderease->valuestring, "%u",
				&(pStr.Pwr1Derease));
			if (!is_saved) {
				pStr.IsSave = 0;
			} else {
				pStr.IsSave = atoi(is_saved->valuestring);
			}
#ifdef BIG_END
			pStr.Pwr1Derease = my_htonl_ntohl(pStr.Pwr1Derease);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG: //(0xF017) 3.18 重定向信息配
		{
			printf("set redirect info\n");
			write_action_log(bandip->valuestring,
				"3.18 set redirect info request!");
			cJSON *onoff = cJSON_GetObjectItem(pa_object, "onoff");	
			cJSON *earfcn = cJSON_GetObjectItem(pa_object, "earfcn");
			cJSON *redirect_type = cJSON_GetObjectItem(pa_object, "redirect_type");
			wrFLLmtToEnbRedirectInfoCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			//重定向开关(0开启,1关闭)
			if (onoff) {
				sscanf(onoff->valuestring, "%x", &(pStr.OnOff));
			}
			//重定向频点
			if (earfcn) {
				sscanf(earfcn->valuestring, "%x", &(pStr.EARFCN));
			}
			//重定向类型(0-4G,1-3G,2-2G)
			if (redirect_type) {
				sscanf(redirect_type->valuestring, "%x",
					&(pStr.RedirectType));
			}
#ifdef BIG_END
			pStr.OnOff = my_htonl_ntohl(pStr.OnOff); 
			pStr.EARFCN = my_htonl_ntohl(pStr.EARFCN); 
			pStr.RedirectType = my_htonl_ntohl(pStr.RedirectType);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SET_SYS_TMR: //(0xF01F) 3.26 设置基站系统时间
		{
			printf("set base station system time\n");
			write_action_log(bandip->valuestring,
				"3.26 set base band sys time request!");
			cJSON *timestr = cJSON_GetObjectItem(pa_object, "timestr");
			if (!timestr) break;
			wrFLLmtToEnbSetSysTmr_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			snprintf((S8 *)(pStr.TmrStr), 20, "%s", timestr->valuestring); /*“2015.01.20-10:10:10”*/
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_REM_MODE_CFG: //(0xF023) 3.30 设置基站同步方式
		{
			printf("set base station synchronous mode\n");
			write_action_log(bandip->valuestring,
				"3.30 set type of sync request!");
			cJSON *Remmode = cJSON_GetObjectItem(pa_object, "remmode");
			if (!Remmode) break;
			wrFLLmtToEnbRemModeCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(Remmode->valuestring, "%x", &(pStr.Remmode));
#ifdef BIG_END
			pStr.Remmode = my_htonl_ntohl(pStr.Remmode);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_GPS_PP1S_CFG://(0xF029) 3.36 GPS 同步偏移调节配置
		{
			printf("set GPS Synchronization offset config\n");
			write_action_log(bandip->valuestring,
				"3.36 set GPS Synchronization offset config request!");
			cJSON *offset = cJSON_GetObjectItem(pa_object, "offset");
			if (!offset) break;
			wrFLgpsPp1sCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(offset->valuestring, "%x", &(pStr.Gpsppsls));
#ifdef BIG_END
			pStr.Gpsppsls = my_htonl_ntohl(pStr.Gpsppsls);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_AGC_SET://(0xF079) 3.91 AGC 配置 仅FDD有效
		{
			printf("3.91 set AGC config(only FDD)\n");
			write_action_log(bandip->valuestring,
				"3.91 set AGC config request!");
			cJSON *agcflag = cJSON_GetObjectItem(pa_object, "agcflag");
			if (!agcflag) break;
			wrFLLmtToEnbAGCSet_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(agcflag->valuestring, "%u", &(pStr.AgcFlag));
#ifdef BIG_END
			pStr.AgcFlag = my_htonl_ntohl(pStr.AgcFlag);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SYS_ARFCN_MOD:  //(0xF080) 3.93.  小区频点动态修改
		{
			printf("3.93 Earfcn dynamic configure\n");
			write_action_log(bandip->valuestring,
				"3.93 arfcn dynamic config request!");
			cJSON *sysUlARFCN = cJSON_GetObjectItem(pa_object, "sysUlARFCN");
			cJSON *sysDlARFCN = cJSON_GetObjectItem(pa_object, "sysDlARFCN");
			cJSON *PLMN = cJSON_GetObjectItem(pa_object, "PLMN");
			cJSON *sysBand = cJSON_GetObjectItem(pa_object, "sysBand");
			cJSON *CellId = cJSON_GetObjectItem(pa_object, "CellId");
			cJSON *UePMax = cJSON_GetObjectItem(pa_object, "UePMax");

			wrFLLmtToEnbSysArfcnMod_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);

			pStr.sysUlARFCN = atoi(sysUlARFCN->valuestring);
			pStr.sysDlARFCN = atoi(sysDlARFCN->valuestring);
			pStr.CellId = atoi(CellId->valuestring);
			pStr.UePMax = atoi(UePMax->valuestring);
			memcpy(pStr.PLMN, PLMN->valuestring, strlen(PLMN->valuestring));
#ifdef BIG_END
			pStr.sysUlARFCN = my_htonl_ntohl(pStr.sysUlARFCN);
			pStr.sysDlARFCN = my_htonl_ntohl(pStr.sysDlARFCN);
			pStr.CellId = my_htonl_ntohl(pStr.CellId);
			pStr.UePMax = my_htonl_ntohl(pStr.UePMax);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.sysBand = atoi(sysBand->valuestring);
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_IMEI_REQUEST_CFG: /*3.101. IMEI  捕获功能配置 0xF08A*/
	{
		printf("3.101 IMEI enable cfg\n");
		write_action_log(bandip->valuestring,"3.101 IMEI enable for 4G");

		//!todo:nyb,2018.05.04, add code body to enable IMEI feature
		cJSON *ImeiEnable = cJSON_GetObjectItem(pa_object, "ImeiEnable");
		if (!ImeiEnable) break;
		wrFLLmtToEnbImeiEnableCfg_t pStr;
		memset(&pStr, '\0', sizeof(pStr));
		WrmsgHeader.u16MsgLength = sizeof(pStr);
#ifdef BIG_END
		//except below keyword, others were changed at above step
		WrmsgHeader.u16MsgLength
			= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
		U32 enb_t = 0;
		sscanf(ImeiEnable->valuestring, "%u", &enb_t);
		pStr.ImeiEnable = enb_t;
		pStr.WrmsgHeaderInfo = WrmsgHeader;
		memcpy(send_request, &pStr, sizeof(pStr));
		send_len = sizeof(pStr);
		break;
	}
	case O_FL_LMT_TO_ENB_TDD_SUBFRAME_ASSIGNMENT_SET: //(0xF049) 3.62 设置TD子帧配置
		{
			printf("set TDD subframe config\n");
			write_action_log(bandip->valuestring,
				"3.62 TDD subframe config request!");
			cJSON *u8tddsfassignment
				= cJSON_GetObjectItem(pa_object,
					"tddsf_assignment");
			cJSON *u8tddspecialsfpatterns
				= cJSON_GetObjectItem(pa_object,
					"tdd_special_patterns");
			wrFLLmtOrEnbSendYDDSubframeSetAndGet_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			U32 tddt = 0, tddp = 0;
			sscanf(u8tddsfassignment->valuestring, "%x", &tddt);
			sscanf(u8tddspecialsfpatterns->valuestring, "%x", &tddp);
			pStr.u8TddSfAssignment = my_htonl_ntohl(tddt);
			pStr.u8TddSpecialSfPatterns = my_htonl_ntohl(tddp);
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SELFCFG_ARFCN_CFG_REQ: //(0xF051) 3.89 频点自配置频点增/删
		{
			printf("3.89 Arfcn self-configuration add/delete\n");
			write_action_log(bandip->valuestring,
				"3.89 Arfcn self-configuration add/delete request!");
			cJSON *cfg_type = cJSON_GetObjectItem(pa_object, "cfg_type");
			cJSON *arfcn_value = cJSON_GetObjectItem(pa_object, "arfcn");
			if (!cfg_type || !arfcn_value) break;
			wrFLLmtToEnbSelfcfgArfcnCfgReq_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(cfg_type->valuestring, "%u", &(pStr.Cfgtype));
			sscanf(arfcn_value->valuestring, "%u", &(pStr.ArfcnValue));
#ifdef BIG_END
			pStr.Cfgtype = my_htonl_ntohl(pStr.Cfgtype);
			pStr.ArfcnValue = my_htonl_ntohl(pStr.ArfcnValue);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_TAC_MODIFY_REQ://(0xF069) 7.5 TAC手动修改配置下发
		{
			printf("7.5 TAC manually modify config\n");
			write_action_log(bandip->valuestring,
				"7.5 TAC manually modify config request!");
			cJSON *tac_value = cJSON_GetObjectItem(pa_object, "tac_value");
			if (!tac_value) break;
			wrFLLmtToEnbTACModifyReq_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			sscanf(tac_value->valuestring, "%u", &(pStr.TacValue));
#ifdef BIG_END
			pStr.TacValue = my_htonl_ntohl(pStr.TacValue);	
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SECONDARY_PLMNS_SET://(0xF060) 7.7 辅 PLMN 列表配置
		{
			printf("7.7 plmn list config\n");
			write_action_log(bandip->valuestring,
				"7.7 plmn list config request!");
			cJSON *plmn_num = cJSON_GetObjectItem(pa_object, "plmn_num");
			cJSON *plmnlist = cJSON_GetObjectItem(pa_object, "plmn_list");
			printf("%s\n", plmn_num->valuestring);
			if (!plmnlist) {
				printf("plmn list is null\n");
				break;
			}
			wrEnbToLmtSecondaryPlmnsQueryAck_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);

			U8 count = cJSON_GetArraySize(plmnlist);
			U8 number = 0;
			//modify count++ to number++
			//to fix the auxiliary PLMN issue, nyb,2017.10.08
			for (number = 0; number < count; number++) {
				cJSON *list = cJSON_GetArrayItem(plmnlist,
					(int)number);
				memcpy(pStr.u8SecPLMNList[number],
					list->valuestring,
					strlen(list->valuestring));
			}
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.u8SecPLMNNum = atoi(plmn_num->valuestring);
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_CONTROL_UE_LIST_CFG://(0xF039) 4.5 管控名单配置
		{
			printf("Control list config\n");
			write_action_log(bandip->valuestring,
				"4.5 Control list config request!");
			cJSON *list_type= cJSON_GetObjectItem(pa_object, "type");
			cJSON *imsi_list= cJSON_GetObjectItem(pa_object, "imsi");
			if (!list_type || !imsi_list) break;

			wrFLLmtToEnbControlUeListCfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			(atoi(list_type->valuestring) == 1)?
				(pStr.ControlMovement = 1):
				(pStr.ControlMovement = 0);
			int imsi_count = 0;
			int str_len = strlen(imsi_list->valuestring);
			char tmp_imsi[C_MAX_IMSI_LEN];
			for (i = 0, j = i; i < str_len; i++) {
				if ((imsi_list->valuestring[i] == ',')) {
					memset(pStr.ControlUEIdentity[imsi_count],
						'\0', C_MAX_IMSI_LEN);
					memcpy(pStr.ControlUEIdentity[imsi_count],
						&(imsi_list->valuestring)[j], i - j);
					j = i + 1;
					imsi_count += 1;
				}
				if (i == str_len -1) {
					memset(pStr.ControlUEIdentity[imsi_count],
						'\0', C_MAX_IMSI_LEN);
					memcpy(pStr.ControlUEIdentity[imsi_count],
						&(imsi_list->valuestring)[j], i - j + 1);
					j = i + 1;
					imsi_count += 1;
				}
			}
			printf("control list[%d]:%s\n", i, pStr.ControlUEIdentity[i]);
			pStr.ControlUENum = imsi_count;
			pStr.ControlUEProperty = 0;
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_CFG: // (0xF057) 3.66 位置区更新拒绝原因配置
		{
			printf("3.38 lane place update refusal cause config\n");
			write_action_log(bandip->valuestring,
				"3.38 lane place update refusal cause config request!");
			wrFLLmtToEnbTauAttachRejectCauseCfg_t pStr;
			cJSON *rejectcause = cJSON_GetObjectItem(pa_object, "reject_cause");
			sscanf(rejectcause->valuestring, "%u", &(pStr.RejectCause));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
#ifdef BIG_END
			pStr.RejectCause = my_htonl_ntohl(pStr.RejectCause);
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_SELFCFG_CELLPARA_REQ://(0xF04F)小区自配置请求
		{
			printf("cell self-configuration\n");
			write_action_log(bandip->valuestring,
				"cell self-configuration request!");
			cJSON *selfband = cJSON_GetObjectItem(pa_object, "self_band");
			if (!selfband) break;
			FLLmtToEnbSelfcfgCellparaReq_t pStr;
			memset(&pStr, '\0', sizeof(pStr));
			WrmsgHeader.u16MsgLength = sizeof(pStr);
			U32 sel_t = 0;
			sscanf(selfband->valuestring, "%x", &sel_t);
			pStr.SelfBand = sel_t;
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_NTP_SERVER_IP_CFG: /* (0xF075) NTP服务器IP配置 */
		{
			printf("set NTP server ip config request!\n");
			wrFLLmtToEnbNtpServerCfg_t pStr;
			cJSON *ntpip = cJSON_GetObjectItem(pa_object, "ntpip");
			if (!ntpip) break;
			strcpy((S8 *)pStr.ntpServerIp, ntpip->valuestring);
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_TIME_TO_RESET_CFG:  /* (0xF086)定点重启配置 */
		{
			printf("set time reset config request!\n");
			wrFLLmtToEnbTimeToResetCfg_t pStr;
			cJSON *ac = cJSON_GetObjectItem(pa_object, "switch");
			cJSON *rtime = cJSON_GetObjectItem(pa_object, "rtime");
			if (!ac) {
				pStr.ResetSwitch = 0;
			} else {
				pStr.ResetSwitch = atoi(ac->valuestring);
			}
			strcpy(pStr.ResetTime, rtime->valuestring);
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
		/*GSM下发命令*/
	case O_SEND_TOGSM_GETCFGONE:
	case O_SEND_TOGSM_GETCFGTWO:
		{
			printf("GSM get\n");
			wrtogsmfreqcfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));

			WrtogsmHeader.msg_len = SEND_TO_GSM_LEN;
			WrtogsmHeader.msg_type = O_SEND_TO_GSM_QUERY;
			if(WrmsgHeader.u16MsgType == O_SEND_TOGSM_GETCFGONE)
				WrtogsmHeader.carrier_type = 0;
			else if(WrmsgHeader.u16MsgType == O_SEND_TOGSM_GETCFGTWO)
				WrtogsmHeader.carrier_type = 1;
			WrtogsmHeader.msg_action = 0;

			pStr.WrtogsmHeaderInfo = WrtogsmHeader;
			pStr.bcc_len = GSM_MSG_HEADER_LEN + 8;
			pStr.bcc_type = O_GSM_CARRIER_FRE;
			pStr.mcc_len = GSM_MSG_HEADER_LEN + 8;
			pStr.mcc_type = O_GSM_MCC;
			pStr.mnc_len = GSM_MSG_HEADER_LEN + 8;
			pStr.mnc_type = O_GSM_MNC;
			pStr.lac_len = GSM_MSG_HEADER_LEN + 8;
			pStr.lac_type = O_GSM_LAC;
			pStr.lowatt_len = GSM_MSG_HEADER_LEN + 8;
			pStr.lowatt_type = O_GSM_DOWN_ATTEN;
			pStr.upatt_len = GSM_MSG_HEADER_LEN + 8;
			pStr.upatt_type = O_GSM_UP_ATTEN;
			pStr.cnum_len = GSM_MSG_HEADER_LEN + 8;
			pStr.cnum_type = O_GSM_CI;
			pStr.cfgmode_len = GSM_MSG_HEADER_LEN + 1;
			pStr.cfgmode_type = O_GSM_CONFIG_MODE;
			pStr.workmode_len = GSM_MSG_HEADER_LEN + 1;
			pStr.workmode_type = O_GSM_WORK_MODE;
			pStr.startfreq_1_len = GSM_MSG_HEADER_LEN + 2;
			pStr.startfreq_1_type = O_GSM_START_FRE_ONE;
			pStr.endfreq_1_len = GSM_MSG_HEADER_LEN + 2;
			pStr.endfreq_1_type = O_GSM_STOP_FRE_ONE;
			pStr.startfreq_2_len = GSM_MSG_HEADER_LEN + 2;
			pStr.startfreq_2_type = O_GSM_START_FRE_TWO;
			pStr.endfreq_2_len = GSM_MSG_HEADER_LEN + 2;
			pStr.endfreq_2_type = O_GSM_STOP_FRE_TWO;
			pStr.freqoffset_len = GSM_MSG_HEADER_LEN + 2;
			pStr.freqoffset_type = O_GSM_FRE_OFFSET;

			handle_gsm_req(&pStr);
			goto go_return;
		}
	case O_SEND_TOGSM_SETCFGONE:
	case O_SEND_TOGSM_SETCFGTWO:
		{
			printf("GSM set\n");
			wrtogsmfreqcfg_t pStr;
			memset(&pStr, '\0', sizeof(pStr));

			WrtogsmHeader.msg_len = SEND_TO_GSM_LEN;
			WrtogsmHeader.msg_type = O_SEND_TO_GSM_SET;
			if(WrmsgHeader.u16MsgType == O_SEND_TOGSM_SETCFGONE)
				WrtogsmHeader.carrier_type = 0;
			else if(WrmsgHeader.u16MsgType == O_SEND_TOGSM_SETCFGTWO)
				WrtogsmHeader.carrier_type = 1;
			WrtogsmHeader.msg_action = 0;

			cJSON *bcc = cJSON_GetObjectItem(pa_object, "bcc");
			cJSON *mcc = cJSON_GetObjectItem(pa_object, "mcc");
			cJSON *mnc = cJSON_GetObjectItem(pa_object, "mnc");
			cJSON *lac = cJSON_GetObjectItem(pa_object, "lac");
			cJSON *lowatt = cJSON_GetObjectItem(pa_object, "lowatt");
			cJSON *upatt = cJSON_GetObjectItem(pa_object, "upatt");
			cJSON *cnum = cJSON_GetObjectItem(pa_object, "cnum");
			cJSON *cfgmode = cJSON_GetObjectItem(pa_object, "cfgmode");
			cJSON *workmode = cJSON_GetObjectItem(pa_object, "workmode");
			cJSON *startfreq_1 = cJSON_GetObjectItem(pa_object, "startfreq_1");
			cJSON *endfreq_1 = cJSON_GetObjectItem(pa_object, "endfreq_1");
			cJSON *startfreq_2 = cJSON_GetObjectItem(pa_object, "startfreq_2");
			cJSON *endfreq_2 = cJSON_GetObjectItem(pa_object, "endfreq_2");
			cJSON *freqoffset = cJSON_GetObjectItem(pa_object, "freqoffset");
			if(!bcc || !mcc || !mnc || !lac|| !lowatt || !upatt ||
				!cnum || !cfgmode || !workmode ||
				!startfreq_1 || !endfreq_1 || !startfreq_2 ||
				!endfreq_2|| !freqoffset ) break;

			pStr.WrtogsmHeaderInfo = WrtogsmHeader;
			pStr.bcc_len = GSM_MSG_HEADER_LEN + 8;
			pStr.bcc_type = O_GSM_CARRIER_FRE;
			sscanf(bcc->valuestring, "%s", pStr.bcc);
			pStr.mcc_len = GSM_MSG_HEADER_LEN + 8;
			pStr.mcc_type = O_GSM_MCC;
			sscanf(mcc->valuestring, "%s", pStr.mcc);
			pStr.mnc_len = GSM_MSG_HEADER_LEN + 8;
			pStr.mnc_type = O_GSM_MNC;
			sscanf(mnc->valuestring, "%s", pStr.mnc);
			pStr.lac_len = GSM_MSG_HEADER_LEN + 8;
			pStr.lac_type = O_GSM_LAC;
			sscanf(lac->valuestring, "%s", pStr.lac);
			pStr.lowatt_len = GSM_MSG_HEADER_LEN + 8;
			pStr.lowatt_type = O_GSM_DOWN_ATTEN;
			sscanf(lowatt->valuestring, "%s", pStr.lowatt);
			pStr.upatt_len = GSM_MSG_HEADER_LEN + 8;
			pStr.upatt_type = O_GSM_UP_ATTEN;
			sscanf(upatt->valuestring, "%s", pStr.upatt);
			pStr.cnum_len = GSM_MSG_HEADER_LEN + 8;
			pStr.cnum_type = O_GSM_CI;
			sscanf(cnum->valuestring, "%s", pStr.cnum);
			pStr.cfgmode_len = GSM_MSG_HEADER_LEN + 1;
			pStr.cfgmode_type = O_GSM_CONFIG_MODE;
			sscanf(cfgmode->valuestring, "%d", (S32 *)&(pStr.cfgmode));
			pStr.workmode_len = GSM_MSG_HEADER_LEN + 1;
			pStr.workmode_type = O_GSM_WORK_MODE;
			sscanf(workmode->valuestring, "%d", (S32 *)&(pStr.workmode));
			pStr.startfreq_1_len = GSM_MSG_HEADER_LEN + 2;
			pStr.startfreq_1_type = O_GSM_START_FRE_ONE;
			sscanf(startfreq_1->valuestring, "%d", (S32 *)&(pStr.startfreq_1));
			pStr.endfreq_1_len = GSM_MSG_HEADER_LEN + 2;
			pStr.endfreq_1_type = O_GSM_STOP_FRE_ONE;
			sscanf(endfreq_1->valuestring, "%d", (S32 *)&(pStr.endfreq_1));
			pStr.startfreq_2_len = GSM_MSG_HEADER_LEN + 2;
			pStr.startfreq_2_type = O_GSM_START_FRE_TWO;
			sscanf(startfreq_2->valuestring, "%d", (S32 *)&(pStr.startfreq_2));
			pStr.endfreq_2_len = GSM_MSG_HEADER_LEN + 2;
			pStr.endfreq_2_type = O_GSM_STOP_FRE_TWO;
			sscanf(endfreq_2->valuestring, "%d", (S32 *)&(pStr.endfreq_2));
			pStr.freqoffset_len = GSM_MSG_HEADER_LEN + 2;
			pStr.freqoffset_type = O_GSM_FRE_OFFSET;
			sscanf(freqoffset->valuestring, "%d", (S32 *)&(pStr.freqoffset));

			printf("-------------->bcc: %s\n", pStr.bcc);
			printf("-------------->mcc: %s\n", pStr.mcc);
			printf("-------------->mnc: %s\n", pStr.mnc);
			printf("-------------->lac: %s\n", pStr.lac);
			printf("-------------->lowatt: %s\n", pStr.lowatt);
			printf("-------------->upatt: %s\n", pStr.upatt);
			printf("-------------->cnum: %s\n", pStr.cnum);
			printf("-------------->cfgmode: %d\n", pStr.cfgmode);
			printf("-------------->workmode: %d\n", pStr.workmode);
			printf("-------------->startfreq_1: %d\n", pStr.startfreq_1);
			printf("-------------->endfreq_1: %d\n", pStr.endfreq_1);
			printf("-------------->startfreq_2: %d\n", pStr.startfreq_2);
			printf("-------------->endfreq_2: %d\n", pStr.endfreq_2);
			printf("-------------->freqoffset: %d\n", pStr.freqoffset);

			handle_gsm_req(&pStr);
			goto go_return;
		}
	case O_GET_GSM_CAPTIME:
		{
			printf("get gsm cap time\n");

			cJSON *object = cJSON_CreateObject();
			if (object == NULL)
				goto go_return;
			int captime_t = get_gsm_cap_time();
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddStringToObject(object, "ip", entry->ipaddr);
			cJSON_AddNumberToObject(object, "captime", captime_t);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				break;
			}
			printf("get gsm cap time str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.gsm_captime_api_arg,
				send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);

#ifdef CLI
			char no_info_str[32] = {0};
			snprintf(no_info_str, 32, "captime: %d min", captime_t);
			struct cli_req_if cli_info;
			snprintf((S8 *)(cli_info.ipaddr), 20, "%s", GSM_IP_ADDR);
			cli_info.msgtype = O_GET_GSM_CAPTIME + 1;
			cli_info.sockfd = -1;
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					no_info_str,
					strlen(no_info_str) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			goto go_return;
		}
	case O_SET_GSM_CAPTIME:
		{
			printf("set gsm cap time\n");

			cJSON *captime = cJSON_GetObjectItem(pa_object, "captime");
			if (!captime) goto go_return;
			int captime_t;
			sscanf(captime->valuestring, "%d", (S32 *)&captime_t);
			set_gsm_cap_time(captime_t);

#ifdef CLI
			char no_info_str[32] = {0};
			snprintf(no_info_str, 32, "set captime [%d] success", captime_t);
			struct cli_req_if cli_info;
			snprintf((S8 *)(cli_info.ipaddr), 20, "%s", GSM_IP_ADDR);
			cli_info.msgtype = O_SET_GSM_CAPTIME + 1;
			cli_info.sockfd = -1;
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					no_info_str,
					strlen(no_info_str) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			goto go_return;
		}
	case O_SEND_TO_GSM_NETSCAN:
		{
			printf("network scan\n");
			WrtogsmHeader.msg_type = O_SEND_TO_GSM_NETSCAN;
			WrtogsmHeader.msg_action = 0;
			WrtogsmHeader.msg_len = sizeof(WrtogsmHeader);
			handle_send_to_gsm(&WrtogsmHeader,GSM_CARRIER_TYPE_ALL);
			/*新建保存扫频信息的文件，并清空，此文件中保存的是最后一次扫频的结果*/
			FILE *scanfile_fp = fopen(SCAN_GSM_FILE, "w");
			if(scanfile_fp == NULL)
				printf("open gsm scan file err!\n");
			fclose(scanfile_fp);
			char no_info_str[32] = {0};
			snprintf(no_info_str, 32, "start gsm scan...");
#ifdef CLI
			struct cli_req_if cli_info;
			snprintf((S8 *)(cli_info.ipaddr), 20, "%s", GSM_IP_ADDR);
			cli_info.msgtype = O_SEND_TO_GSM_NETSCAN + 1;
			cli_info.sockfd = -1;
			cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
			if (cli_entry) {
				write(cli_entry->info.sockfd,
					no_info_str,
					strlen(no_info_str) + 1);
				close(cli_entry->info.sockfd);
				del_entry_to_clireq_list(cli_entry);
			}
#endif
			goto go_return;
		}
	case O_SEND_TO_GSM_CARRIER_RES:
		{
			printf("carrier reset\n");
			U8 reset_cmd;
			cJSON *reset_cmd_cjson = cJSON_GetObjectItem(pa_object, "reset_cmd");
			if (reset_cmd_cjson)
				sscanf(reset_cmd_cjson->valuestring, "%d",
					(S32 *)&reset_cmd);
			WrtogsmHeader.msg_type = O_SEND_TO_GSM_CARRIER_RES;
			WrtogsmHeader.msg_action = reset_cmd;
			WrtogsmHeader.msg_len = sizeof(WrtogsmHeader);

			handle_send_to_gsm(&WrtogsmHeader,
				GSM_CARRIER_TYPE_ALL);
			goto go_return;
		}
	case O_FL_LMT_TO_ENB_REBOOT_CFG: /* (0xF00B) 3. 8. 重启指示 */
		{
			if (!strncmp(bandip->valuestring, "127.0.0.1", 9)) {
				write_action_log(bandip->valuestring,
					"base master dev reboot request!");
				/* 主控板重启 */
				system("reboot");
			} else	if(!strcmp(entry->ipaddr, GSM_IP_ADDR)) {
				write_action_log(bandip->valuestring,
					"GSM dev reboot request!");
				/* GSM 重启 */
				printf("gsm restart\n");
				WrtogsmHeader.msg_type = O_SEND_TO_GSM_RES_SYS;
				WrtogsmHeader.msg_action = 0;
				WrtogsmHeader.msg_len = sizeof(WrtogsmHeader);
				handle_send_to_gsm(&WrtogsmHeader,
					GSM_CARRIER_TYPE_ALL);
				goto go_return;
			} else {
				/* LTE 重启 */
				write_action_log(bandip->valuestring,
					"LTE dev reboot request!");
				WrmsgHeader.u16MsgLength = sizeof(WrmsgHeader);
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
				memcpy(send_request, &WrmsgHeader, sizeof(WrmsgHeader));
				send_len = sizeof(WrmsgHeader);
			}
			break;
		}
	case O_FL_LMT_TO_ENB_IP_CFG:/* 请求设置基带板IP地址 */
		{
			write_action_log(bandip->valuestring,
				"set base band dev ip request!");
			cJSON *ipaddr = cJSON_GetObjectItem(pa_object, "ipaddr");
			cJSON *mask = cJSON_GetObjectItem(pa_object, "netmask");
			cJSON *gateway = cJSON_GetObjectItem(pa_object, "gateway");
			if (!ipaddr || !mask || !gateway)
				break;
			wrFLLmtToEnbIpCfg_t pStr;
			WrmsgHeader.u16MsgLength = sizeof(pStr);
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			pStr.WrmsgHeaderInfo = WrmsgHeader;
			snprintf((S8 *)pStr.eNBIPStr, 50, "%s#%s#%s#",
				ipaddr->valuestring,
				mask->valuestring,
				gateway->valuestring);
			pthread_mutex_lock(&(entry->lock));
			entry->changed_ip = strdup(ipaddr->valuestring);
			pthread_mutex_unlock(&(entry->lock));
			memcpy(send_request, &pStr, sizeof(pStr));
			send_len = sizeof(pStr);
			break;
		}
	case O_FL_LMT_TO_ENB_GET_ENB_STATE:   // (0xF01A) 3.20. 获取小区最后一次操作执行的状态请求
	case O_FL_LMT_TO_ENB_GET_ARFCN:       // (0xF027) 3.34. 小区频点配置相关参数查询
	case O_FL_LMT_TO_ENB_SYNC_INFO_QUERY: // (0xF02D) 3.42. 基站同步信息查询
	case O_FL_LMT_TO_ENB_CELL_STATE_INFO_QUERY:      // (0xF02F) 3.44. 小区状态信息查询
	case O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY: // (0xF031) 3.48. 接收增益和发射功率查询
	case O_FL_LMT_TO_ENB_ENB_IP_QUERY:          // (0xF033) 3.48. IP地址查询
	case O_FL_LMT_TO_ENB_QRXLEVMIN_VALUE_QUERY: // (0xF035) 3.50. 小区选择 QRxLevMin 查询
	case O_FL_LMT_TO_ENB_REM_CFG_QUERY:         // (0xF037) 3.54. 扫频参数配置查询
	case O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG_QUERY:    // (0xF03F) 3.56. 重定向参数配置查询
	case O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON_QUERY:  // (0xF041) 3.58. 上电小区自激活配置查询
	case O_FL_LMT_TO_ENB_SYS_LOG_LEVL_QUERY:            // (0xF047) 3.62. 查询 log 打印级别
	case O_FL_LMT_TO_ENB_TDD_SUBFRAME_ASSIGNMENT_QUERY: // (0xF04B) 3.64. TDD 子帧配置查询
	case O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY:            // (0xF05C) 3.73. GPS经纬高度查询
	case O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_QUERY: // (0xF06B) 3.75. 位置区更新拒绝原因查询
	case O_FL_LMT_TO_ENB_GPS_INFO_RESET: // (0xF06D) 3.77. Gps 信息复位配置
	case O_FL_LMT_TO_ENB_GPS1PPS_QUERY:  // (0xF073) 3.83. GPS 同步偏移量查询
	case O_FL_LMT_TO_ENB_SELFCFG_ARFCN_QUERY:// (0xF04D) 3.87. 频点自配置后台频点列表查询
	case O_FL_LMT_TO_ENB_MEAS_UE_CFG_QUERY:  // (0xF03D) 4.3. UE 测量配置查询
	case O_FL_LMT_TO_ENB_CONTROL_LIST_QUERY: // (0xF043) 4.7 管控名单查询(黑白名单)
	case O_FL_LMT_TO_ENB_RA_ACCESS_QUERY:    // (0xF065) 7.1. 随机接入成功率问询
	case O_FL_LMT_TO_ENB_RA_ACCESS_EMPTY_REQ:  // (0xF067) 7.3. 随机接入成功率清空请求
	case O_FL_LMT_TO_ENB_SECONDARY_PLMNS_QUERY:// (0xF062) 7.9. 辅 PLMN 列表查询
		{
			write_action_log_by_type(bandip->valuestring,
				WrmsgHeader.u16MsgType);
			/* GSM */
			if(entry->work_mode == 2 || entry->work_mode == 3) {
				char json_str[256] = {0};
				snprintf(json_str, 256,
					"{\"topic\":\"%s\",\"ip\":\"%s\","
					"\"work_admin_state\":\"%d\"}",
					my_topic, entry->ipaddr,
					entry->cellstate);
				printf("gsm status str:%s\n", json_str);
				pthread_mutex_lock(&http_send_lock);
				http_send(args_info.cellstat_api_arg,
					json_str, NULL, NULL);
				pthread_mutex_unlock(&http_send_lock);
				break;
			}
			WrmsgHeader.u16MsgLength = sizeof(WrmsgHeader);
#ifdef BIG_END
			WrmsgHeader.u16MsgLength
				= my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
			memcpy(send_request, &WrmsgHeader, sizeof(WrmsgHeader));
			send_len = sizeof(WrmsgHeader);
			break;
		}
	default:
		goto go_return;
	}/* end switch(WrmsgHeader.u16MsgType) */
	if (send_request[0] != 0) {
		sleep(1);
		int ret = baicell_send_msg_to_band(send_request, send_len,
			entry->ipaddr);
		if (ret == -1) {
			perror("send value failed！");
		}
	}
go_return:
	if (pa_object) cJSON_Delete(pa_object);
#endif
}

/* mosquitto receive message callback function */
void my_message_callback(struct mosquitto *mosq,
	void *obj,
	const struct mosquitto_message *message)
{
	struct mosq_config *cfg;
	int i;
	bool res;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if (message->retain && cfg->no_retain) {
		return;
	}
	if (cfg->filter_outs) {
		for (i = 0; i < cfg->filter_out_count; i++) {
			mosquitto_topic_matches_sub(cfg->filter_outs[i],
				message->topic, &res);
			if(res) return;
		}
	}

	if (!(message->payloadlen)) return;
	int length = message->payloadlen;
	int msg_type = 0;
	char *my_msg = (char *)calloc(1, length + 1);
	if (my_msg == NULL) {
		printf("File:%s Function:%s Line:%d,calloc error:%s(%d)\n",
			__FILE__, __func__, __LINE__,
			strerror(errno), errno);
		return;
	}
	memcpy(my_msg, message->payload, length);
	my_msg[length] = '\0';
	printf("get buffer from mqttpub is:%s\n", my_msg);
	write_action_log("get command from mqtt", my_msg);
	pare_config_json_message(my_msg, FAULT_CLIENT_SOCKET_FD);
	free(my_msg);
}
