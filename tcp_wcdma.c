/*
 * ============================================================================
 *
 *       Filename:  tcp_wcdma.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年09月25日 09时13分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  weichao lu (Chao), lwc_yx@163.com
 *   Organization:  
 *
 * ============================================================================
 */

#include "tcp_wcdma.h"
#include "cli_action.h"

wcdma_header_t header;
wcdma_user_info_t user_info;
int clientfd;


cJSON *w_sta_object = NULL;
cJSON *w_sta_array = NULL;
S32 w_sta_count = 0;
pthread_mutex_t w_sta_obj_lock; // 锁，拿到锁才能对-|w_sta_object 进行操作
// 						    |w_sta_array
void w_sta_json_obj_init()
{
	pthread_mutex_init(&w_sta_obj_lock, NULL);
}
void w_sta_json_obj_lock()
{
	pthread_mutex_lock(&w_sta_obj_lock);
}
void w_sta_json_obj_unlock()
{
	pthread_mutex_unlock(&w_sta_obj_lock);
}


//						    |w_sta_array
/* 初始化保存 WCDMA 基带板上传的IMSI信息的JSON数组 */
void w_sta_json_init()
{
	w_sta_object = cJSON_CreateObject();
	if (w_sta_object == NULL) exit(0);
	cJSON_AddStringToObject(w_sta_object, "topic", my_topic);
	w_sta_array = cJSON_CreateArray();
	if (w_sta_array == NULL) {
		cJSON_Delete(w_sta_object);
		exit(0);
	}
	cJSON_AddItemToObjectCS(w_sta_object,"clients", w_sta_array);
	w_sta_count = 0;
	printf("WCDMA Station json object Inited!\n");
}
/* 清除保存 WCDMA 基带板上传的IMSI信息的JSON数组 */
void w_sta_json_delete()
{
	cJSON_Delete(w_sta_object);
}

/*处理开启/关闭射频应答*/
void handle_wcdma_rf_res(char *wcdma_buf, int msg_len, struct cliinfo *info)
{
	char type = wcdma_buf[4];
	memcpy(header.wcdma_num, &wcdma_buf[WCDMA_TYPE_LEN], WCDMA_NUM_LEN);
	char cli_resq[128] = {0};
	if(!strcmp(header.wcdma_num, WCDMA_START_RF_RESPONSE))
	{
		// printf("WCDMA_START_RF_RESPONSE\n");
		if(type == '0')
			snprintf(cli_resq, 128, "wcdma start RF success!\n");
		else
			snprintf(cli_resq, 128, "wcdma start RF failed!\n");
	}
	else if(!strcmp(header.wcdma_num, WCDMA_STOP_RF_RESPONSE))
	{
		// printf("WCDMA_STOP_RF_RESPONSE\n");
		if(type == '0')
			snprintf(cli_resq, 128, "wcdma stop RF success!\n");
		else
			snprintf(cli_resq, 128, "wcdma stop RF failed!\n");
	}
	printf("%s\n", cli_resq);
	struct cli_req_if cli_info;
	snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
	cli_info.msgtype = O_FL_ENB_TO_LMT_SET_ADMIN_STATE_ACK;
	cli_info.sockfd = -1;
	cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
	if (cli_entry) {
		int xxx = write(cli_entry->info.sockfd,
			cli_resq,
			strlen(cli_resq) + 1);
		close(cli_entry->info.sockfd);
		del_entry_to_clireq_list(cli_entry);
	}
}

/*获取版本相关信息应答*/
void handle_wcdma_version(char *wcdma_buf, int msg_len, struct cliinfo *info)
{
	char *wcdma_msg_buf;
	memcpy(header.wcdma_num, &wcdma_buf[WCDMA_TYPE_LEN], WCDMA_NUM_LEN);
	if(!strcmp(header.wcdma_num, WCDMA_VERSION_RESPONSE))
	{
		wcdma_msg_buf = calloc(1, msg_len);
		if(wcdma_msg_buf == NULL)
		{
			printf("malloc err!\n");
			return;
		}
		memcpy(wcdma_msg_buf, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], msg_len);
		//printf("version:%s\n", wcdma_msg_buf);
		band_entry_t *entry = band_list_entry_search(info->addr);
		pthread_mutex_lock(&(entry->lock));
		entry->s_version = strdup(wcdma_msg_buf);
		pthread_mutex_unlock(&(entry->lock));
		if(wcdma_msg_buf)
			free(wcdma_msg_buf);
		return;
	}
	else if(!strcmp(header.wcdma_num, WCDMA_GET_DAC_RESPONSE))
	{
		char *json_str;
		wcdma_msg_buf = calloc(1, msg_len);
		if(wcdma_msg_buf == NULL)
		{
			printf("malloc err!\n");
			return;
		}
		memcpy(wcdma_msg_buf, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], msg_len);
		if (wcdma_msg_buf[msg_len] == '\n')
			wcdma_msg_buf[msg_len] = 0; 
		cJSON *object = cJSON_CreateObject();
		cJSON_AddStringToObject(object, "topic", my_topic);
		cJSON_AddStringToObject(object, "ip", info->addr);
		cJSON_AddStringToObject(object, "dac", wcdma_msg_buf);
		json_str = cJSON_PrintUnformatted(object);
		printf("mac info str:%s\n", cJSON_Print(object));
		pthread_mutex_lock(&http_send_lock);
		http_send(args_info.wcdma_dac_info_api_arg, json_str, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
		
		if (json_str)
			free(json_str);
		if (object)
			cJSON_Delete(object); // 删除object
		char *cli_resq[128] = {0};
		snprintf((S8 *)cli_resq, 128, "wcdma dev(ip:%s), DAC is %s",
			info->addr, wcdma_msg_buf);
		printf("%s\n", (S8 *)cli_resq);
		if(wcdma_msg_buf)
			free(wcdma_msg_buf);
		/* find cli entry and send response to cli  */
		struct cli_req_if cli_info;
		snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
		cli_info.msgtype = MQTT_WCDMA_GET_DAC + 1;
		cli_info.sockfd = -1;
		cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
		if (cli_entry) {
			write(cli_entry->info.sockfd,
				cli_resq,
				strlen((S8 *)cli_resq) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	}
	else if(!strcmp(header.wcdma_num, WCDMA_FTP_NUM_RESPONSE))
	{
		char type;
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
			printf("set ftp parameter success!\n");
		else
			printf("set ftp parameter err!\n");
	}
	else if(!strcmp(header.wcdma_num, WCDMA_FTP_DOWNLOAD_RESPONSE))
	{
		char type;
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
		{
			printf("download success!\n");
			char send_upgrade[12] = WCDMA_UPGRADE_CMD;
			send(info->fd, send_upgrade, sizeof(send_upgrade), 0);
		}
		else
			printf("download err!\n");
	}
	else if(!strcmp(header.wcdma_num, WCDMA_FTP_UPGRADE_RESPONSE))
	{
		char type;
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
			printf("wcdma upgrade success!\n");
		else
			printf("wcdma upgrade err!\n");
	}
	else if(!strcmp(header.wcdma_num, WCDMA_SET_FTP_INFO_RESPONSE))
	{
		char type;
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
			printf("set ftp info success!\n");
		else
			printf("set ftp info err!\n");
	}

	else if(!strcmp(header.wcdma_num, WCDMA_SET_FRE_RATIO))
	{
		char cli_resq[128] = {0};
		snprintf(cli_resq, 128,	"set WCDMA device pilot ratio OK!");
		printf("%s\n", cli_resq);
		/* find cli entry and send response to cli  */
		struct cli_req_if cli_info;
		snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
		cli_info.msgtype = MQTT_WCDMA_SET_RATIO_OFPILOT_SIG + 1;
		cli_info.sockfd = -1;
		cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
		if (cli_entry) {
			write(cli_entry->info.sockfd,
				cli_resq,
				strlen(cli_resq) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	}
	else if(!strcmp(header.wcdma_num, WCDMA_SET_LAC_RAC_TIME_RES))
	{
		//printf("WCDMA_SET_LAC_RAC_TIME_RES\n");
		char type;
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
			printf("set lac/rac time success!\n");
		else
			printf("set lac/rac time err!\n");
	}
	else if(!strcmp(header.wcdma_num, WCDMA_SET_DAC_OFFSET_RES))
	{
		// printf("WCDMA_SET_DAC_OFFSET_RES\n");
		char type;
		char cli_resq[128] = {0};
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
			snprintf(cli_resq, 128,
				"set wcdma device dac offset success!\n");
		else
			snprintf(cli_resq, 128,
				"set wcdma device dac offset failed!\n");
		printf("%s\n", cli_resq);
		/* find cli entry and send response to cli  */
		struct cli_req_if cli_info;
		snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
		cli_info.msgtype = MQTT_WCDMA_SET_DAC + 1;
		cli_info.sockfd = -1;
		cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
		if (cli_entry) {
			write(cli_entry->info.sockfd,
				cli_resq,
				strlen(cli_resq) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	}
	else if(!strcmp(header.wcdma_num, WCDMA_SET_PA_POWER_RES))
	{
		// printf("WCDMA_SET_PA_POWER_RES\n");
		char type;
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
			printf("set pa power success!\n");
		else
			printf("set pa power err!\n");
	}
	else if(!strcmp(header.wcdma_num, WCDMA_STOP_LAC_RAC_TIME_RES))
	{
		// printf("WCDMA_STOP_LAC_RAC_TIME_RES\n");
		char type;
		char cli_resq[128] = {0};
		memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);
		if(type == '0')
			snprintf(cli_resq, 128,
				"stop lac/rac time set by self uccess!\n");
		else
			snprintf(cli_resq, 128,
				"stop lac/rac time set by self failed!\n");
		printf("%s\n", cli_resq);
		/* find cli entry and send response to cli  */
		struct cli_req_if cli_info;
		snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
		cli_info.msgtype = MQTT_WCDMA_STOP_SET_TIME_OF_LAC_RAC + 1;
		cli_info.sockfd = -1;
		cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
		if (cli_entry) {
			write(cli_entry->info.sockfd,
				cli_resq,
				strlen(cli_resq) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	}
	else{
		printf("other info\n");
	}	
}

/*上报设备运行状态*/
void handle_work_state(char *msg, int msg_len, struct cliinfo *info)
{
	char type;
	int i = 0;
	band_entry_t *entry = band_list_entry_search(info->addr);
	if(entry == NULL)
	{
		printf("search entry by ip(%s) failed!\n", info->addr);
		return;
	}
	while(1)
	{
		if(msg[i] == '#')
		{
			type = msg[i + 1];
			break;
		}
		i++;
	}
	if(type == '1'|| type == '0')
	{
		char json_str[256] = {0};
		snprintf(json_str, 256, 
			"{\"topic\":\"%s\",\"ip\":\"%s\","
			"\"work_admin_state\":\"%c\"}",
			my_topic, info->addr, type);
		printf("wcdma status str:%s\n", json_str);
		pthread_mutex_lock(&http_send_lock);
		http_send(args_info.cellstat_api_arg,
			json_str, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
	}
	else
		return;
	pthread_mutex_lock(&entry->lock);
	entry->change_count++;
	entry->online = atoi(&type);
	pthread_mutex_unlock(&entry->lock);
	char send_version[12] = WCDMA_GET_VERSION_CMD;
	send(info->fd, send_version, sizeof(send_version), 0);
}

/*处理用户信息并上报*/
void handle_wcdma_user_info(char *msg, int msg_len, struct cliinfo *info)
{
	char *client_ip = info->addr;
	//printf("client ip:%s\n", client_ip);
	int i = 0;
	int num = 0;
	for(i = 0; i < msg_len; i++) {
		if(num == 0 && msg[i] == '#') {
			user_info.imsi = calloc(1, i);
			if(user_info.imsi == NULL) {
				printf("malloc err!\n");
				return;
			}
			memcpy(user_info.imsi, msg, i);
			//printf("imsi:%s\n", user_info.imsi);
			num = i + 1;
		}
		if(num < i && msg[i] == '#') {
			user_info.imei = calloc(1, i - num);
			if(user_info.imei == NULL) {
				printf("malloc err!\n");
				return;
			}
			memcpy(user_info.imei, &msg[num], i - num);
			//printf("imei:%s\n", user_info.imei);
			break;
		}
	}
	printf("<--WCDMA--IMSI %s\n", user_info.imsi);
	printf("<--WCDMA--IMEI %s\n", user_info.imei);
	time_t rawtime;
	struct tm * timeinfo;
	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);
	char now_time[20] = {0};

	cJSON *object = cJSON_CreateObject();
	if (!object)
		return;
	cJSON_AddStringToObject(object, "timestamp", now_time);
	cJSON_AddStringToObject(object, "imsi", user_info.imsi);
	cJSON_AddStringToObject(object, "ip", client_ip);
	cJSON_AddStringToObject(object, "rssi", " ");
	cJSON_AddStringToObject(object, "imei", user_info.imei);
	S8 *usend_str = cJSON_PrintUnformatted(object);
	if (usend_str) {
		/* send to udp  */
		send_string_udp(usend_str, strlen(usend_str));
		free(usend_str);
	}
	cJSON_AddItemToArray(w_sta_array, object);
	w_sta_count++;
	if (w_sta_count >= args_info.max_stacache_arg) {
		char *send_str = cJSON_PrintUnformatted(w_sta_object);
		if (send_str) {
			printf("collect user info report str:%s\n", send_str);
#ifdef SCACHE
			if (get_netstate() != NETWORK_OK) {
#endif
				pthread_mutex_lock(&http_send_lock);
				http_send_client(args_info.station_api_arg,
					send_str, NULL, NULL);
				pthread_mutex_unlock(&http_send_lock);
#ifdef SCACHE
			} else {
				write_string_cachefile(send_str);
			}
#endif
			free(send_str);
		}
		w_sta_json_delete();
		w_sta_json_init();
	} /* end if */
#ifdef SAVESQL
	/* insert into sqlite */
	_imsi_st s_point;
	strcpy(s_point.type, "3G");
	snprintf(now_time, 20, "%lu", (unsigned long)rawtime);
	snprintf(s_point.this_time, 20, "%04d-%02d-%02d %02d:%02d:%02d",
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
		timeinfo->tm_mday, timeinfo->tm_hour,
		timeinfo->tm_min, timeinfo->tm_sec);
	snprintf(s_point.imsi, 16, "%s", user_info.imsi);
	snprintf(s_point.imei, 16, "%s", user_info.imei);
	strcpy(s_point.dev_ip, client_ip);
	save2sqlite_insert(s_point);
#endif /* SAVESQL */
	if(user_info.imsi)
		free(user_info.imsi);
	if(user_info.imei)
		free(user_info.imei);
}

/*处理wcdma发来的各种信息*/
void handle_wcdma_info(char *wcdma_buf, int msg_len, struct cliinfo *info)
{
	memcpy(header.wcdma_num, &wcdma_buf[WCDMA_TYPE_LEN], WCDMA_NUM_LEN);
	char *msg = calloc(1, msg_len);
	if(msg == NULL) {
		printf("malloc err!\n");
		return;
	}
	memcpy(msg, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], msg_len);

	if(!strcmp(header.wcdma_num, WCDMA_WORK_STATE)) {
		printf("WCDMA_WORK_STATE\n");
		handle_work_state(msg, msg_len, info);
	} else if(!strcmp(header.wcdma_num, WCDMA_SET_POWER_RESPONSE)) {
		char cli_resq[128] = {0};
		// printf("WCDMA_SET_POWER_RESPONSE\n");
		if(!strcmp(msg, "SETPOWER 0"))
			snprintf(cli_resq, 128, "set wcdma TX-power success!\n");
		else
			snprintf(cli_resq, 128, "set wcdma TX-power failed!\n");
		printf("%s\n", cli_resq);
		/* find cli entry and send response to cli  */
		struct cli_req_if cli_info;
		snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
		cli_info.msgtype = O_FL_ENB_TO_LMT_SYS_PWR1_DEREASE_ACK;
		cli_info.sockfd = -1;
		cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
		if (cli_entry) {
			write(cli_entry->info.sockfd,
				cli_resq,
				strlen(cli_resq) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	} else if(!strcmp(header.wcdma_num, WCDMA_WORK_INFO_RESPONSE)) {
		// printf("WCDMA_WORK_INFO_RESPONSE\n");
		char cli_resq[128] = {0};
		if(!strcmp(msg, "0")) {
			snprintf(cli_resq, 128,
				"set WCDMA work mode success!\n");
		} else {
			snprintf(cli_resq, 128,
				"set WCDMA work mode failed!\n");
		}
		printf("%s\n", cli_resq);
		/* find cli entry and send response to cli  */
		struct cli_req_if cli_info;
		snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
		cli_info.msgtype = MQTT_WCDMA_SET_WORK_PARAM + 1;
		cli_info.sockfd = -1;
		cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
		if (cli_entry) {
			write(cli_entry->info.sockfd,
				cli_resq,
				strlen(cli_resq) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	} else if(!strcmp(header.wcdma_num, WCDMA_RESTART_RESPONSE)) {
		// printf("WCDMA_RESTART_RESPONSE\n");
		char cli_resq[128] = {0};
		if(!strcmp(msg, "0"))
			snprintf(cli_resq, 128,
				"wcdma device restart success!");
		else
			snprintf(cli_resq, 128,
				"wcdma device restart failed!");
		printf("%s\n", cli_resq);
		/* find cli entry and send response to cli  */
		struct cli_req_if cli_info;
		snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
		cli_info.msgtype = O_FL_ENB_TO_LMT_REBOOT_ACK;
		cli_info.sockfd = -1;
		cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
		if (cli_entry) {
			write(cli_entry->info.sockfd,
				cli_resq,
				strlen(cli_resq) + 1);
			close(cli_entry->info.sockfd);
			del_entry_to_clireq_list(cli_entry);
		}
	} else if(!strcmp(header.wcdma_num, WCDMA_USER_INFO)) {
		printf("WCDMA_USER_INFO\n");
		handle_wcdma_user_info(msg, msg_len, info);
	} else if(!strcmp(header.wcdma_num, WCDMA_SET_IP_RESPONSE)) {
		// printf("WCDMA_SET_IP_RESPONSE\n");
		if(!strcmp(msg, "0")) {
			Epoll_del(info);
			/*删除链表节点*/
			band_entry_t *entry = band_list_entry_search(info->addr);
			__list_del_entry(&(entry->node));
			
			printf("set wcdma ip success\n");
		} else if(!strcmp(msg, "1")) {
			printf("set wcdma ip err!\n");
		} else
			printf("WCDMA_SET_IP_RESPONSE err!\n");
	} else if(!strcmp(header.wcdma_num, WCDMA_GET_TIME)) {
		printf("get time\n");
		time_t rawtime;
		char my_time[15];
		struct tm * timeinfo;
		rawtime = time(NULL);
		timeinfo = localtime(&rawtime);
		char now_time[20] = {0};
		snprintf(now_time, 20, "%lu", (unsigned long)rawtime);
		snprintf(my_time, 15,
			"%04d%02d%02d%02d%02d%02d",
			timeinfo->tm_year + 1900,
			timeinfo->tm_mon +1, timeinfo->tm_mday,
			timeinfo->tm_hour, timeinfo->tm_min,
			timeinfo->tm_sec);

		char send_buf[27];
		int send_len = 14 + 4;
		sprintf(send_buf, "FFFF#%d#0318%s", send_len, my_time);
		printf("send_buf:%s\n", send_buf);
		send(info->fd, send_buf, sizeof(send_buf), 0);
	} else if(!strcmp(header.wcdma_num, WCDMA_SET_TIME_RES)) {
		if(!strcmp(msg, "0"))
		    printf("set time success\n");
		else if(!strcmp(msg, "1"))
		    printf("set time err!\n");
		else
		    printf("WCDMA_SET_TIME_RES err!\n");
	} else {
		//printf("other info\n");
		return;
	}
	if(msg)
		free(msg);
}

/*处理上报设备运行参数信息*/
void handle_wcdma_dev_info(char *msg, int msg_len, struct cliinfo *info)
{
	//wcdma_dev_info_t dev_info;
	char *dev_info[12];
	int i = 0;
	int a;
	int num = 0;
	int offset = 0;
	char *json_str = NULL;
	while(0 != msg[i]) {
		if(msg[i] == '#') {
			dev_info[num] = calloc(1, i - offset);
			if(dev_info[num] == NULL) {
				printf("malloc err!\n");
				for(a = 0; a < num; a++) {
					if(dev_info[a])
						free(dev_info[a]);
				}
				return;
			}
			memcpy(dev_info[num], &msg[offset], i - offset);
			offset = i + 1;
			num++;
		}
		i++;
	}
	cJSON *object = cJSON_CreateObject();
	cJSON_AddStringToObject(object, "topic", my_topic);
	cJSON_AddStringToObject(object, "ip", info->addr);
	cJSON_AddStringToObject(object, "mcc", dev_info[0]);
	cJSON_AddStringToObject(object, "mnc", dev_info[1]);
	cJSON_AddStringToObject(object, "lac", dev_info[2]);
	cJSON_AddStringToObject(object, "rac", dev_info[3]);
	cJSON_AddStringToObject(object, "psc", dev_info[4]);
	cJSON_AddStringToObject(object, "arfcn", dev_info[5]);
	cJSON_AddStringToObject(object, "cellid", dev_info[6]);
	cJSON_AddStringToObject(object, "power", dev_info[7]);
	cJSON_AddStringToObject(object, "fre_ratio", dev_info[8]);
	cJSON_AddStringToObject(object, "lac_rac_time", dev_info[9]);
	cJSON_AddStringToObject(object, "mode", dev_info[10]);
	cJSON_AddStringToObject(object, "pa_power", dev_info[11]);
	json_str = cJSON_PrintUnformatted(object);
	printf("dev info str:%s\n", cJSON_Print(object));
	pthread_mutex_lock(&http_send_lock);
	http_send(args_info.wcdma_dev_info_api_arg, json_str, NULL, NULL);
	pthread_mutex_unlock(&http_send_lock);
	for(i = 0; i < 12; i++) {
		// printf("dev_info[%d]:%s\n", i, dev_info[i]);
		if(dev_info[i])
			free(dev_info[i]);
	}
	if (json_str) {
		free(json_str);
		json_str = NULL;
	}
/* Add with jxj */
	struct cli_req_if cli_info;
	snprintf((S8 *)(cli_info.ipaddr), 20, "%s", info->addr);
	cli_info.msgtype = CLI_TO_BASE_MASTER_GET_WCDMA_DEV_INFO;
	cli_info.sockfd = -1;
	cli_req_t *cli_entry = entry_to_clireq_list_search(cli_info);
	if (cli_entry) {
		band_entry_t *entry = band_list_entry_search(info->addr);
		if (entry) {
			cJSON_AddStringToObject(object,
				"online", (entry->online)?"1":"0");
			cJSON_AddNumberToObject(object,
				"work_mode", entry->work_mode);
			cJSON_AddStringToObject(object,
				"software_version",
				(entry->s_version)?(entry->s_version):"");
		}
		// printf("----------->version:%s\n", entry->s_version);
		json_str = cJSON_PrintUnformatted(object);
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
	}
	/* end of Add with jxj */
	if (object)
		cJSON_Delete(object); // 删除object
}


void handle_wcdma_run_parameter(char *wcdma_buf, int msg_len, struct cliinfo *info)
{
	char *msg;
	memcpy(header.wcdma_num, &wcdma_buf[WCDMA_TYPE_LEN], WCDMA_NUM_LEN);
	msg = calloc(1, msg_len);
	if(msg == NULL)
	{
		printf("malloc err!\n");
		return;
	}
	memcpy(msg, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], msg_len);

	if(!strcmp(header.wcdma_num, WCDMA_DEV_INFO_RESPONSE))
	{
		// printf("WCDMA_DEV_INFO_RESPONSE\n");
		handle_wcdma_dev_info(msg, msg_len, info);
	}
	else{
		printf("other info\n");
	}
	if(msg)
		free(msg);
}


void handle_wcdma_mode(char *wcdma_buf, int msg_len)
{
	char type;
	memcpy(header.wcdma_num, &wcdma_buf[WCDMA_TYPE_LEN], WCDMA_NUM_LEN);
	memcpy(&type, &wcdma_buf[WCDMA_TYPE_LEN + WCDMA_NUM_LEN], 1);

	if(!strcmp(header.wcdma_num, WCDMA_SET_2G_INFO_RESPONSE))
	{
		// printf("WCDMA_SET_2G_INFO_RESPONSE\n");
		if(type == '0')
			printf("set to 2g info success\n");
		else if(type == '1')
			printf("set to 2g info err!\n");
		else
			printf("WCDMA_SET_2G_INFO_RESPONSE err!\n");
	}
	else if(!strcmp(header.wcdma_num, WCDMA_SET_TO_2G_RESPONSE))
	{
		// printf("WCDMA_SET_TO_2G_RESPONSE\n");
		if(type == '0')
			printf("set to 2g success\n");
		else if(type == '1')
			printf("set to 2g err!\n");
		else
			printf("WCDMA_SET_TO_2G_RESPONSE err!\n");
	}
	else{
		printf("other info\n");
	}
}

/*处理wcdma发来的信息buf*/
void handle_recv_wcdma(char *wcdma_buf, int buf_len, struct cliinfo *info)
{
	int msg_len;
	memcpy(header.wcdma_type, wcdma_buf, WCDMA_TYPE_LEN);
	msg_len = buf_len - WCDMA_TYPE_LEN - WCDMA_NUM_LEN;

	// printf("-------------type:%s\n", header.wcdma_type);
	if(!strcmp(header.wcdma_type, WCDMA_RF)) /* 00 */
	{
		// printf("WCDMA_RF\n");
		handle_wcdma_rf_res(wcdma_buf, msg_len, info);
	}
	else if(!strcmp(header.wcdma_type, WCDMA_INFO)) /* 03 */
	{
		printf("WCDMA_INFO\n");
		handle_wcdma_info(wcdma_buf, msg_len, info);
		// char a[12] = "FFFF#4#0901";
		// send(info->fd, a, sizeof(a), 0);
	}
	else if(!strcmp(header.wcdma_type, WCDMA_VERSION))
	{
		// printf("WCDMA_VERSION\n");
		handle_wcdma_version(wcdma_buf, msg_len, info);
	}
	else if(!strcmp(header.wcdma_type, WCDMA_RUN_PARAMETER))
	{
		// printf("WCDMA_RUN_PARAMETER\n");
		handle_wcdma_run_parameter(wcdma_buf, msg_len, info);
	}
	else if(!strcmp(header.wcdma_type, WCDMA_MODE))
	{
		// printf("WCDMA_MODE\n");
		handle_wcdma_mode(wcdma_buf, msg_len);
	}
	else
	{
		printf("info err!\n");
		return;
	}
}

/* 解析WCDMA 数据*/
void handle_package_wcdma(U8 *recBuf, struct cliinfo *info) 
{
	//printf("recv buffer is %s\n", recBuf);
	int buf_len;

	char wcdma_len_buf[WCDMA_MSG_LEN];
	memcpy(wcdma_len_buf, &recBuf[WCDMA_TYPE_FFFF + 1], WCDMA_MSG_LEN);
	buf_len = wcdma_len_buf[0] * 100 + wcdma_len_buf[1];
	//printf("len_buf:%d\n", wcdma_len_buf[1]);
	int wcdma_header_len = WCDMA_TYPE_FFFF + 1 + WCDMA_MSG_LEN;    //FFFF#加上两个字节长度字符
	if(recBuf[wcdma_header_len] ==  '#')
		return;
	int header_len = wcdma_header_len + 1;
	recBuf[header_len + buf_len] = '\0';
	//printf("msg:%s\n", &recBuf[header_len]);
	handle_recv_wcdma((char *)&recBuf[header_len], buf_len, info);
	return;

}

/* 与WCDMA设备进行通信 */
void *thread_tcp_to_baseband_wcdma(void *arg)
{
	int server_sockfd, client_sockfd;
	struct sockaddr_in server_addr, client_addr;

	server_sockfd = server_socket_init(args_info.wcdma_port_arg);
	if (server_sockfd == -1) {
		printf("get tcp socket error!\n");
		exit(-1);
	}
	printf("Creat WCDMA socket SUCCESS!\n");
	int ret = listen(server_sockfd, MAX_HLIST_LENGTH);
	if (ret == -1) {
		printf("listen failed %s(%d)\n", strerror(errno), errno);
		exit(-1);
	}
	printf("Listen WCDMA Socket...\n");
	socklen_t addrlen = sizeof(struct sockaddr_in);
	/* 循环读取socket 数据 */
	char arp_scmd[128] = {0};
	char wcdma_mac[18] = {0};
	while(1) {
		client_sockfd = accept(server_sockfd,
			(struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
		if(client_sockfd == -1) {
			sleep(1);
			continue;
		}
		printf("Accept WCDMA fd: %d, ipaddr: %s\n",
			client_sockfd, inet_ntoa(client_addr.sin_addr));
		ret = Epoll_add(client_sockfd, inet_ntoa(client_addr.sin_addr));
		/* 通过ip在链表中进行查找，未找到则新建节点 */
#ifdef VMWORK
		snprintf(arp_scmd, 128,
			"arp |grep '%s' |awk '{print $3}' |tr '[a-z]' '[A-Z]'",
			inet_ntoa(client_addr.sin_addr));
#else
/* in openwrt */
		snprintf(arp_scmd, 128,
			"arp |grep '%s' |awk '{print $4}' |tr '[a-z]' '[A-Z]'",
			inet_ntoa(client_addr.sin_addr));
#endif
		FILE *pfile = popen(arp_scmd, "r");
		if (!pfile) {
			perror("popen error");
		}
		fscanf(pfile, "%s", wcdma_mac);
		pclose(pfile);
		band_entry_t *entry =
			band_list_entry_search_add(inet_ntoa(client_addr.sin_addr));
		if (entry->sockfd != client_sockfd) {
			if (entry->sockfd != -1) {
				struct cliinfo cli;
				cli.fd = entry->sockfd;
				cli.addr = strdup(inet_ntoa(client_addr.sin_addr));
				Epoll_del(&cli);
				free(cli.addr);
			}
			pthread_mutex_lock(&(entry->lock));
			entry->sockfd = client_sockfd;
			entry->work_mode = WCDMA_BASE_DEVICE;
			entry->macaddr = calloc(1, 18);
			if (entry->macaddr)
				strncpy(entry->macaddr, wcdma_mac, 18);
			pthread_mutex_unlock(&(entry->lock));
		}
	} /* end while(1) */
	return NULL;
}

/* 每隔args_info.sendsta_itv_arg 时间，检查用户信息json数组中有没有用户信息
 * 如果有,加锁并将json转换成字符串并使用http协议上至服务器, 
 * 然后删除json中的所有信息,重新初始化json数组
 * */
void *thread_read_send_wcdma_json_object(void *arg)
{
	char *send_str = NULL;
	while (1) {
		sleep(args_info.sendsta_itv_arg);
		w_sta_json_obj_lock();
		if (!w_sta_count) {
			w_sta_json_obj_unlock();
			continue;
		}
		send_str = cJSON_PrintUnformatted(w_sta_object);
		if (!send_str) {
			w_sta_json_obj_unlock();
			continue;
		}
		printf("Timeout，send WCDMA info str:%s\n", send_str);
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
		w_sta_json_delete();
		w_sta_json_init();
		w_sta_json_obj_unlock();
	} /* end while(... */
	return NULL;
}
/* 解析从mqtt 服务器发来的数据 */
void wcdma_pare_config_json_message(S8 *mqtt_msg, band_entry_t *entry)
{
	cJSON *pa_object = cJSON_Parse(mqtt_msg);
	if (!pa_object) {
		printf("in function:%s,Pare json object failed!\n" , __func__);
		return;
	}
	cJSON *bandip = cJSON_GetObjectItem(pa_object, "ip");
	if (!bandip) {
		printf("Not founnd 'bandip'\n");
		cJSON_Delete(pa_object);
		return;
	}
	/* get config msg type */
	cJSON *msgtype = cJSON_GetObjectItem(pa_object, "msgtype");
	if (!msgtype) {
		cJSON_Delete(pa_object);
		return;
	}
	U8 send_request[MAXDATASIZE] = {0};
	U32 send_len = 0;
#ifdef BIG_END
	U32 msgt = 0;
#else
	U16 msgt = 0;
#endif
	sscanf(msgtype->valuestring, "%x", &msgt);
	switch (msgt) {
	case MQTT_WCDMA_TURN_ONOFF:/* F00D小区激活与去激活 */
		{
			printf("WCDMA turn ON/OFF freq!\n");
			write_action_log(bandip->valuestring,
				"WCDMA turn ON/OFF freq!");
			cJSON *work_admin_state =
				cJSON_GetObjectItem(pa_object,
					"work_admin_state");
			if (!work_admin_state) break;

			U8 radio_switch = 0;
			sscanf(work_admin_state->valuestring, "%x",
				(U32 *)&radio_switch);
			if (radio_switch) { /* turn on radio freq */
				sprintf((S8 *)send_request, "FFFF#4#0001");
			} else { /* turn off radio freq */
				sprintf((S8 *)send_request, "FFFF#4#0004");
			}
			break;
		}
	case MQTT_WCDMA_SET_TXPOWER: //(0xF015) 发射功率配置
		{
			printf("set txpower config\n");
			write_action_log(bandip->valuestring,
				"set txpower config request!");
			cJSON *powerderease =
				cJSON_GetObjectItem(pa_object, "powerderease");
			cJSON *is_saved =
				cJSON_GetObjectItem(pa_object, "is_saved");
			if (!powerderease) break;
			/* WCDMA */
			U32 s_txpower = 0;
			sscanf(powerderease->valuestring, "%u", &s_txpower);
			U32 s_length = WCDMA_REQUEST_PKGTYPE_LENGTH
				+ strlen("SETPOWER") + 1 + 2;
			sprintf((S8 *)send_request,
				"FFFF#%u#0312SETPOWER %02u",
				s_length, s_txpower);
			break;
		}
	case MQTT_WCDMA_SET_SYS_TINE: //(0xF01F) 3.26 设置基站系统时间
		{
			printf("set WCDMA device system time\n");
			write_action_log(bandip->valuestring,
				"set WCDMA device system time request!");
			cJSON *timestr =
				cJSON_GetObjectItem(pa_object, "timestr");
			if (!timestr) break;
			/* WCDMA */
			S8 s_time[16] = {0};
			S32 i, j;
			for (i = 0, j = i;
				i < strlen(timestr->valuestring); i++) {
				if ( timestr->valuestring[i] > '9' ||
					timestr->valuestring[i] < '0') {
					continue;
				} else {
					s_time[j++] = timestr->valuestring[i];
				}
			}
			S32 length = WCDMA_REQUEST_PKGTYPE_LENGTH + j;
			sprintf((S8 *)send_request, "FFFF#%u#%s",
				length, s_time);
			break;
		}
	case MQTT_WCDMA_REBOOT_DEV:/* (0xF00B) 3. 8. 重启指示 */
		{
			/* WCDMA 重启 */
			write_action_log(bandip->valuestring,
				"WCDMA dev reboot request!");
			send_len = printf((S8 *)send_request, "FFFF#4#0322");
			break;
		}
		/* JUST WCDMA Config Define*/
	case MQTT_WCDMA_SET_RATIO_OFPILOT_SIG: /* 6.15 设置导频比率 */
		{
			cJSON *ratio = cJSON_GetObjectItem(pa_object, "ratio");
			if (!ratio) break;
			U8 rat = 1;
			sscanf(ratio->valuestring, "%u", (unsigned int *)&rat);
			send_len = sprintf((S8 *)send_request,
				"FFFF#5#0800%1u",rat);
			break;
		}
	case MQTT_WCDMA_SET_TIME_OF_LAC_RAC: /* 6.17置自更改AC/RAC值的时间 */
		{
			cJSON *c_time = cJSON_GetObjectItem(pa_object, "c_time");
			if (c_time == NULL)
				break;
			U32 length = strlen(c_time->valuestring);
			send_len = sprintf((S8 *)send_request,
				"FFFF#%u#0802%s",
				length + WCDMA_REQUEST_PKGTYPE_LENGTH,
				c_time->valuestring);
			break;
		}
	case MQTT_WCDMA_SET_IPADDR: /* 6.10设置WCDMA基带板的IP地址 */
		{
			cJSON *ipaddr = cJSON_GetObjectItem(pa_object, "ipaddr");
			cJSON *mask = cJSON_GetObjectItem(pa_object, "netmask");
			cJSON *gateway = cJSON_GetObjectItem(pa_object, "gateway");
			if (!ipaddr || !mask || !gateway)
				break;
			/* FFFF#length#0320#IP#MASK#GATEWAY */
			U32 length = WCDMA_REQUEST_PKGTYPE_LENGTH
				+ strlen(ipaddr->valuestring) + 1
				+ strlen(mask->valuestring) + 1
				+ strlen(gateway->valuestring);
			send_len = sprintf((S8 *)send_request,
				"FFFF#%u#0320%s#%s#%s",
				length,	ipaddr->valuestring, mask->valuestring,
				gateway->valuestring);
			break;
		}
	case MQTT_WCDMA_SET_WORK_PARAM: /* 6.1 */ 
		{
			printf("<----set wcdma device workmode cfg request\n");
			cJSON *mcc = cJSON_GetObjectItem(pa_object, "mcc");
			cJSON *mnc = cJSON_GetObjectItem(pa_object, "mnc");
			cJSON *lac = cJSON_GetObjectItem(pa_object, "lac");
			cJSON *arfcno = cJSON_GetObjectItem(pa_object, "arfcno");
			cJSON *psc = cJSON_GetObjectItem(pa_object, "psc");
			cJSON *rac = cJSON_GetObjectItem(pa_object, "rac");
			cJSON *cellid = cJSON_GetObjectItem(pa_object, "cellid");
			if (!mcc || !mnc || !lac || !arfcno || !psc ||
				!rac || !cellid)
				break;
			U32 s_mcc, s_mnc, s_lac, s_arfcno;
			U32 s_psc, s_rac, s_cellid;
			sscanf(mcc->valuestring, "%u", &s_mcc);
			sscanf(mnc->valuestring, "%u", &s_mnc);
			sscanf(lac->valuestring, "%u", &s_lac);
			sscanf(arfcno->valuestring, "%u", &s_arfcno);
			sscanf(psc->valuestring, "%u", &s_psc);
			sscanf(rac->valuestring, "%u", &s_rac);
			sscanf(cellid->valuestring, "%u", &s_cellid);
			S8 tmp_str[128] = {0};
			U32 length = snprintf(tmp_str, 128,
				"0301"
				"MCC:%u@"
				"MNC:%02u@"
				"LAC:%u@"
				"ARFCNo:%u@"
				"PSC:%u@"
				"RAC:%u@"
				"CELLID:%u@",
				s_mcc, s_mnc, s_lac, s_arfcno,
				s_psc,s_rac, s_cellid);
			send_len = snprintf((S8 *)send_request, MAXDATASIZE,
				"FFFF#%u#%s",
				length, tmp_str);
			break;
		}
	case MQTT_WCDMA_STOP_SET_TIME_OF_LAC_RAC: /* 停止自更改LAC/RAC值 */ 
		{
			send_len = sprintf((S8 *)send_request, "FFFF#4#0810");
			break;

		}
	case MQTT_WCDMA_GET_DAC: /* 查询DAC */
		{
			send_len = sprintf((S8 *)send_request, "FFFF#4#0808");
			break;
		}
	case MQTT_WCDMA_SET_DAC: /* 设置DAC */
		{
			cJSON *offset = cJSON_GetObjectItem(pa_object, "offset");
			if (offset == NULL) {
				break;
			}
			S16 ofs = 0;
			sscanf(offset->valuestring, "%d", (U32 *)&ofs);
			send_len = sprintf((S8 *)send_request, "FFFF#5#0806%1d",ofs);
			break;
		}
	case MQTT_WCDMA_SET_DRIVE_TO_2G_INFO:/* 6.33 设置驱赶到2G的信息 */
		{
			cJSON *ncc = cJSON_GetObjectItem(pa_object, "gsm_ncc");
			cJSON *bcc = cJSON_GetObjectItem(pa_object, "gsm_bcc");
			cJSON *arfcn = cJSON_GetObjectItem(pa_object, "gsm_arfcn");
			if (!ncc || !bcc || !arfcn )
				break;
			U32 length = WCDMA_REQUEST_PKGTYPE_LENGTH
				+ strlen(ncc->valuestring)
				+ strlen(bcc->valuestring)
				+ strlen(arfcn->valuestring);
			send_len = sprintf((S8 *)send_request,
				"FFFF#%u#0614%s%s#%s",
				length,
				ncc->valuestring, bcc->valuestring,
				arfcn->valuestring);
			break;
		}
	case MQTT_WCDMA_SET_DRIVE_MOD: /* 6.35 设置基站驱赶模式的信息 */
		{
			cJSON *mod = cJSON_GetObjectItem(pa_object, "mod");
			if (!mod)
				break;
			U32 s_mod;
			sscanf(mod->valuestring, "%u", &s_mod);
			if (s_mod > 1 || s_mod < 0)
				break;
			send_len = sprintf((S8 *)send_request, "FFFF#5#0616%u", s_mod);
			break;
		}
	case MQTT_WCDMA_SET_FTP_DL_INFO: /* 6.37 设置ftp 下载文件的参数值 */
		{
			cJSON *ftp_url = cJSON_GetObjectItem(pa_object, "ftp_url");
			cJSON *ftp_user = cJSON_GetObjectItem(pa_object, "ftp_user");
			cJSON *ftp_passwd = cJSON_GetObjectItem(pa_object, "ftp_passwd");
			if (!ftp_url||!ftp_user||!ftp_passwd)
				break;
			U32 length = WCDMA_REQUEST_PKGTYPE_LENGTH
				+ strlen(ftp_url->valuestring)
				+ strlen(ftp_user->valuestring)
				+ strlen(ftp_passwd->valuestring);
			send_len = sprintf((S8 *)send_request,
				"FFFF#%u#0812%s#%s#%s",
				length,
				ftp_url->valuestring,
				ftp_user->valuestring,
				ftp_passwd->valuestring);
			break;
		}
	case MQTT_WCDMA_SET_PA_POWER: /* 6.42 设置基站外接功放的功率值 */
		{
			cJSON *wat = cJSON_GetObjectItem(pa_object, "wat");
			if (!wat) break;
			U32 length = WCDMA_REQUEST_PKGTYPE_LENGTH
				+ strlen(wat->valuestring);
			send_len = sprintf((S8 *)send_request, "FFFF#%u#0817%s",
				length, wat->valuestring);
			break;
		}
	case MQTT_WCDMA_SYNC_TIME: /* 6.44 基站请求控制台同步时间 */
		{
			send_len = sprintf((S8 *)send_request, "FFFF#4#0344");
			break;
		}
	case MQTT_WCDMA_SET_FTP_UL_INFO: /* 6.45 设置基站文件上传的ftp相关信息*/
		{
			cJSON *url = cJSON_GetObjectItem(pa_object, "ftp_url");
			cJSON *user = cJSON_GetObjectItem(pa_object, "ftp_user");
			cJSON *pass = cJSON_GetObjectItem(pa_object, "ftp_pass");
			cJSON *port = cJSON_GetObjectItem(pa_object, "ftp_port");
			cJSON *itv = cJSON_GetObjectItem(pa_object, "sendfileinterval");
			if (!url || !user || !pass || !port || !itv)
				break;
			U32 length = WCDMA_REQUEST_PKGTYPE_LENGTH
				+ strlen(url->valuestring)
				+ strlen(user->valuestring)
				+ strlen(pass->valuestring)
				+ strlen(port->valuestring)
				+ strlen(itv->valuestring);
			send_len = sprintf((S8 *)send_request,
				"FFFF#%u#0821%s#%s#%s#%s#%s",
				length,	url->valuestring, user->valuestring,
				pass->valuestring, port->valuestring,
				itv->valuestring);
			break;
		}
	case MQTT_WCDMA_GET_STATUS_OF_BASE: // (0xC082) 6.19 获取设备运行参数
		{
			send_len = sprintf((S8 *)send_request, "FFFF#4#0901");
			break;
		}
	default:
		{
			printf("not support command!\n");
			break;
		}
	}
	if (send_len > 0) {
		send(entry->sockfd, send_request, send_len, 0);
	}
	cJSON_Delete(pa_object);
}
