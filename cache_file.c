/*
 * ============================================================================
 *
 *       Filename:  cache_file.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年06月06日 09时26分44秒
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
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "arg.h"
#include "cache_file.h"
#include "http.h"
#include "cJSON.h"
#include "mosquit_sub.h"
#include "hash_list_active.h"
#include "list.h"
static int net_state = 0;  //网络状态标识 0---网络异常 1---网络正常
static int cahefile_count = 0; // 缓存文件中当前数据条数
static int gsm_cahefile_count = 0; // gsm缓存文件中当前数据条数

struct save_creat_config{
	int net_mode;//同频异频状态
};
struct save_creat_config;

int get_netstate()
{
	return net_state;
}
static void set_netstate(int thisinput)
{
	if (net_state != thisinput)
		net_state = thisinput;
}

int get_check_internet_status()
{
	int status = 0;
	FILE *file = popen("uci get netstatus.internet.status", "r");
	if (file == NULL) {
		return status;
	}
	fscanf(file, "%d", &status);
	pclose(file);
	return status;
}
/* 操作缓存文件需要加锁访问 */
pthread_mutex_t cache_filelock;
void cache_filelock_init()
{
	pthread_mutex_init(&cache_filelock, NULL); // 初始化采集用户保存json锁
}
static void cache_filelock_lock()
{
	pthread_mutex_lock(&cache_filelock);
}
static void cache_filelock_unlock()
{
	pthread_mutex_unlock(&cache_filelock);
}

void check_and_creat_file(char *file_name)
{
	if (!file_name) return;
	int ret = access(file_name, 6);
	if (-1 == ret) {
		FILE *fd = fopen(file_name, "w+");
		if (fd == NULL) {
			perror("OPen file:");
		} else
			fclose(fd);
	}
	return;
}

/* 检查缓存文件 */
int check_cache_file()
{
	check_and_creat_file(args_info.gsm_cachefile_arg);
	check_and_creat_file(args_info.cachefile_arg);
	return 0;
}

/*读取GSM缓存文件函数*/
void read_gsm_cachefile()
{
	char *line_str = NULL;
	size_t line_len = 0;
	ssize_t read_len;
	cache_filelock_lock();
	FILE *gsm_cache_fp = fopen(args_info.gsm_cachefile_arg, "r+");
	if(gsm_cache_fp == NULL)
	{
		perror("readandwrite open gsm cachefile err");
		cache_filelock_unlock();
		return;
	}
	int fd = fileno(gsm_cache_fp);
	while ((read_len = getline(&line_str, &line_len, gsm_cache_fp)) != -1) {
		printf("read line_str:%s\n", line_str);
		pthread_mutex_lock(&http_send_lock);
		http_send_client(args_info.gsm_cdma_rep_api_arg,
			line_str, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
		sleep(1);
	}
	if (line_str) free(line_str);
	fflush(gsm_cache_fp);
	ftruncate(fd, 0);
	fclose(gsm_cache_fp);
	gsm_cahefile_count = 0;//清空文件，条目数归零
	cache_filelock_unlock();
	return;
}

/* 循环获取网络状态，并设置网络状态标识 */
void *thread_checknet_readfile()
{
	int err_num = 0;
	FILE *cache_fp = NULL;
	int net_number = 0;
	while(1) {
		if (get_check_internet_status()) {
			set_netstate(NETWORK_OK);
			
			if(net_number >= NETWORK_TIME || access(CUTNET_BRFORE, F_OK)){
				dev_sleep_state(1);
			}
			net_number = 0;
			

			char *line_str = NULL;
			size_t line_len = 0;
			ssize_t read_len;
			cache_filelock_lock();
			cache_fp = fopen(args_info.cachefile_arg, "r+");
			if (cache_fp == NULL) {
				perror("readandwrite open cachefile err");
				cache_filelock_unlock();
				read_gsm_cachefile();
				sleep(30);
				continue;
			}
			int fd = fileno(cache_fp);
			while ((read_len = getline(&line_str, &line_len, cache_fp)) != -1) {
				pthread_mutex_lock(&http_send_lock);
				http_send_client(args_info.station_api_arg,
					line_str, NULL, NULL);
				pthread_mutex_unlock(&http_send_lock);
				sleep(1);
			}
			if (line_str) free(line_str);
			fflush(cache_fp);
			ftruncate(fd, 0);
			fclose(cache_fp);
			cahefile_count = 0;//清空文件，条目数归零
			cache_filelock_unlock();
			read_gsm_cachefile();
		} else {
			set_netstate(NETWORK_NOK);

			if(!access(CUTNET_BRFORE, F_OK))
				net_number+=16;

			if(net_number++ == NETWORK_TIME)
				dev_sleep_state(2);	
		}
		sleep(20);
	} /* end while(1) */
	return NULL;
}
/* 将信息输入到文件中 */
int write_string_cachefile(char *write_string)
{
	if (write_string == NULL)
		return 0;
	int err_num = 0;
	cache_filelock_lock();
	if (cahefile_count++ > args_info.max_filecache_arg) { //如果当前文件中的条目数大于最大值，将不再写入
		cache_filelock_unlock();
		return 0;
	}
	FILE *cache_fp = fopen(args_info.cachefile_arg, "a");
	if (cache_fp == NULL) { 
		printf("add way open cachefile err:%s\n", strerror(err_num));
		cache_filelock_unlock();
		return 0;
	}
	fprintf(cache_fp, "%s\n", write_string);
	fclose(cache_fp);
	cache_filelock_unlock();
	return 1;
}


/* 将gsm信息输入到文件中 */
int write_string_gsmcachefile(char *write_string)
{
	if (write_string == NULL)
		return 0;
	int err_num = 0;
	cache_filelock_lock();
	if (gsm_cahefile_count++ > args_info.max_filecache_arg) { //如果当前文件中的条目数大于最大值，将不再写入
		cache_filelock_unlock();
		return 0;
	}
	FILE *gsm_cache_fp = fopen(args_info.gsm_cachefile_arg, "a");
	if (gsm_cache_fp == NULL) {
		printf("add way open gsm cachefile err:%s\n", strerror(err_num));
		cache_filelock_unlock();
		return 0;
	}
	fprintf(gsm_cache_fp, "%s\n", write_string);
	fclose(gsm_cache_fp);
	cache_filelock_unlock();
	return 1;
}
//初始化断网配置文件
void init_cutnet_config()
{
    if(access(CUTNET_CONFIG, F_OK)){
		char buf[256] = {0};
		snprintf(buf, 256, 
			"{\"work_mode\":\"%d\",\"powerderease\":\"%u\","
			"\"rxgain\":\"%u\"}",    args_info.cutnet_mode_arg, 
			args_info.cutnet_power_arg, args_info.cutnet_rxgain_arg);

		FILE *cut_net = fopen(CUTNET_CONFIG, "w");
		if(!cut_net){
            		printf("init_cutnet_config fopen failure\n");
            		exit(-1);
		}
		
        	fwrite(buf, 1, strlen(buf), cut_net);
        	fclose(cut_net);
    }
}
/*断网模式转换函数*/
void dev_sleep_state(int swth)
{
	if(swth != 1 && swth != 2){
		printf("swth failure\n");
		return;
	}
#if 0
	cJSON *pa_message = cJSON_CreateObject();
	if(!pa_message){
		printf("dev_sleep_state create cJSON failure\n");
		exit(-1);
	}

	cJSON_AddStringToObject(pa_message, "msgtype", "E003");
	cJSON_AddStringToObject(pa_message, "ip", "127.0.0.1");
	if(swth == 1)
		cJSON_AddStringToObject(pa_message, "command", "1");
	else
		cJSON_AddStringToObject(pa_message, "command", "2");

	char *str = cJSON_Print(pa_message);

	pare_config_json_message(str, FAULT_CLIENT_SOCKET_FD);

	cJSON_Delete(pa_message);
#endif
    if(swth == 2){/*开启断网模式*/
		/*保存断网前状态*/

		cJSON *cutnet_before = cJSON_CreateObject();
		if(!cutnet_before){
			printf("cJSON_CreateObject failure\n");
			return;
		}
		char buf[4] = {0};
		sprintf(buf, "%d", same_or_dif_state);
		cJSON_AddStringToObject(cutnet_before, "before_mode", buf);

		band_entry_t *tpos;
		band_entry_t *n;
		list_for_each_entry_safe(tpos, n, return_band_list_head(), node){
			//tpos->net_regain = tpos->regain;
			char buf[24] = {0};
			char str_gain[4] = {0};
			sprintf(str_gain, "%u", tpos->regain);
			sprintf(buf, "regain%s", tpos->ipaddr);
			cJSON_AddStringToObject(cutnet_before, buf, str_gain);

			memset(str_gain, 0, 4);
			memset(buf, 0, 24);
			sprintf(str_gain, "%d", tpos->powerderease);
			sprintf(buf, "power%s", tpos->ipaddr);
			cJSON_AddStringToObject(cutnet_before, buf, str_gain);
		}
		char *str = cJSON_Print(cutnet_before);
		cJSON_Delete(cutnet_before);

		FILE *before_file = fopen(CUTNET_BRFORE, "w");
		if(!before_file){
			printf("fopen failure\n");
			return;
		}
		if(!fwrite(str, 1, strlen(str), before_file)){
			printf("fwrite failure\n");
			return;
		}
		fclose(before_file);
		free(str);

		//开始配置
		int net_work = -1;
		U32 powerderease = -1;
		U32 rxgain = -1;

		if(access(CUTNET_CONFIG, F_OK)){/*文件不存在*/
			net_work = args_info.cutnet_mode_arg;
			powerderease = args_info.cutnet_power_arg;
			rxgain = args_info.cutnet_rxgain_arg;

		}else{
			FILE *config_file = fopen(CUTNET_CONFIG, "r");
			if(!config_file){
				printf("open %s failure\n", CUTNET_CONFIG);
				return;
			}
			char buf[128] = {0};
			fread(buf, 1, 128, config_file);
			fclose(config_file);
			cJSON *config = cJSON_Parse(buf);
			if(!config){
				printf("cJSON_Parse failuse\n");
				exit(-1);
			}
			cJSON *work_mode = cJSON_GetObjectItem(config, "work_mode");
			sscanf(work_mode->valuestring, "%d", &net_work);
			cJSON *power = cJSON_GetObjectItem(config, "powerderease");
			sscanf(power->valuestring, "%u", &powerderease);
			cJSON *gain = cJSON_GetObjectItem(config, "rxgain");
			sscanf(gain->valuestring, "%u", &rxgain);

			cJSON_Delete(config);
		}
		//配置同频异频状态
		if(same_or_dif_state != net_work && net_work != -1){
			
			cJSON *pa_message = cJSON_CreateObject();
			if(!pa_message){
				printf("dev_sleep_state create cJSON failure\n");
				exit(-1);
			}
			cJSON_AddStringToObject(pa_message, "msgtype", "E004");
			if(net_work == 1)
				cJSON_AddStringToObject(pa_message, "command", "1");
			else if(net_work == 2)
				cJSON_AddStringToObject(pa_message, "command", "2");
			else
				cJSON_AddStringToObject(pa_message, "command", "3");
			char *str = cJSON_Print(pa_message);

			cJSON_Delete(pa_message);
			pare_config_json_message(str, -1);
			free(str);
		}
		printf("\nnet_work: %d\npowerderease: %u\nrxgain: %u\n", 
			net_work, powerderease, rxgain);

		list_for_each_entry_safe(tpos, n, return_band_list_head(), node){
				
			if(powerderease != -1){
				cJSON *pow_message = cJSON_CreateObject();
				if(!pow_message){
					printf("dev_sleep_state create cJSON failure\n");
					exit(-1);
				}
				cJSON_AddStringToObject(pow_message, "msgtype", "F015");
				cJSON_AddStringToObject(pow_message, "ip", tpos->ipaddr);
				char buf[4] = {0};
				sprintf(buf, "%u", powerderease);
				cJSON_AddStringToObject(pow_message, "powerderease", buf);
				cJSON_AddStringToObject(pow_message, "is_saved", "1");

				char *str = cJSON_Print(pow_message);
				cJSON_Delete(pow_message);

				printf("%s\n", str);
				pare_config_json_message(str, -1);
				free(str);
			}
			if(rxgain != -1){
				cJSON *gain_message = cJSON_CreateObject();
				if(!gain_message){
					printf("dev_sleep_stat create cJSON failure\n");
					exit(-1);
				}
				cJSON_AddStringToObject(gain_message, "msgtype", "F013");
				cJSON_AddStringToObject(gain_message, "ip", tpos->ipaddr);
				char buf[4] = {0};
				sprintf(buf, "%u", rxgain);
				cJSON_AddStringToObject(gain_message, "rxgain", buf);

				char *str = cJSON_Print(gain_message);
				cJSON_Delete(gain_message);

				printf("\n%s\n", str);
				pare_config_json_message(str, -1);
				free(str);
			}
		}
		
    }else{/*网络重新连接*/


	    	char before_buf[256] = {0};
		FILE *before_file = fopen(CUTNET_BRFORE, "r");
		if(!before_file){
			printf("fopen failure\n");
			return;
		}
		fread(before_buf, 1, 256, before_file);
		fclose(before_file);
		//remove(CUTNET_BRFORE);
		if(unlink(CUTNET_BRFORE))
			printf("unlink %s success\n", CUTNET_BRFORE);

		cJSON *bef_cjon = cJSON_Parse(before_buf);
		cJSON *bef_mode = cJSON_GetObjectItem(bef_cjon, "before_mode");
	    

		//还原同频异频模式
		char mode[4] = {0};
		sprintf(mode, "%d", same_or_dif_state);
		if(bef_mode->valuestring){
			cJSON *connect_msg = cJSON_CreateObject();
			if(!connect_msg){
				printf("cJSON_Create failure\n");
				return ;
			}
			cJSON_AddStringToObject(connect_msg, "msgtype", "E004");
			
			cJSON_AddStringToObject(connect_msg, "command", bef_mode->valuestring);
			
			char *message = cJSON_Print(connect_msg);

			printf("\n%s\n", message);
			pare_config_json_message(message, -1);
			free(message);
		}

		band_entry_t *tpos;
		band_entry_t *n;
		list_for_each_entry_safe(tpos, n, return_band_list_head(), node){
				
			//还原功率衰减值
			char buf[24] = {0};
			sprintf(buf, "power%s", tpos->ipaddr);
			cJSON *bef_power = cJSON_GetObjectItem(bef_cjon, buf);

			cJSON *powr_message = cJSON_CreateObject();
			if(!powr_message){
				printf("cJOSN_Create failure\n");
				return;
			}
			cJSON_AddStringToObject(powr_message, "msgtype", "F015");
			cJSON_AddStringToObject(powr_message, "ip", tpos->ipaddr);
			cJSON_AddStringToObject(powr_message, "powerderease", bef_power->valuestring);
			cJSON_AddStringToObject(powr_message, "is_saved", "1");

			char *powr = cJSON_Print(powr_message);
			cJSON_Delete(powr_message);

			printf("\n%s\n", powr);
			pare_config_json_message(powr, -1);
			free(powr);

			//还原接收增益配置
			cJSON *gain_message = cJSON_CreateObject();
			if(!powr_message){
				printf("cJOSN_Create failure\n");
				return;
			}
			cJSON_AddStringToObject(gain_message, "msgtype", "F013");
			cJSON_AddStringToObject(gain_message, "ip", tpos->ipaddr);

			//获取断网之前接收增益配置
			memset(buf, 0, 24);
			sprintf(buf, "regain%s", tpos->ipaddr);
			cJSON *bef_rxgain = cJSON_GetObjectItem(bef_cjon, buf);
			
			cJSON_AddStringToObject(gain_message, "rxgain", bef_rxgain->valuestring);

			char *gain = cJSON_Print(gain_message);
			cJSON_Delete(gain_message);

			printf("\n%s\n", gain);
			pare_config_json_message(gain, -1);
			free(gain);
		}

    }
}
