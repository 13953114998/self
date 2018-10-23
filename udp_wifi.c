/*
 * ============================================================================
 *
 *       Filename:  udp_wifi.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年03月30日 10时19分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>     
#include <stdlib.h>     
#include <string.h>
#include <limits.h>
#include <zlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <arpa/inet.h>
#include <pthread.h>

#include "config.h"
#include "arg.h"
#include "cJSON.h"
#include "fiberhome_lte.h"
#include "http.h"
#include "hash_list_active.h"

static pthread_mutex_t ap_json_lock, cli_json_lock;
static cJSON *ap_json_object = NULL, *ap_json_array = NULL;
static cJSON *cli_json_object = NULL, *cli_json_array = NULL;
static int ap_json_count = 0;
static int cli_json_count = 0;

static pthread_mutex_t imsi_2g_lock, imsi_3g_lock, imsi_4g_lock;
static cJSON *imsi_2G_object = NULL, *imsi_2G_array = NULL;
static cJSON *imsi_3G_object = NULL, *imsi_3G_array = NULL;
static cJSON *imsi_4G_object = NULL, *imsi_4G_array = NULL;
static int imsi_2G_count = 0;
static int imsi_3G_count = 0;
static int imsi_4G_count = 0;

static void ap_json_object_init()
{
	ap_json_object = cJSON_CreateObject();
	if (!ap_json_object) {
		printf("creat wifi object error!\n");
		return;
	}
	cJSON_AddStringToObject(ap_json_object, "topic", my_topic);
	cJSON_AddStringToObject(ap_json_object, "devmac", dev_mac_string);
	ap_json_array = cJSON_CreateArray();
	if (!ap_json_array) {
		printf("creat wifi array error!\n");
		cJSON_Delete(ap_json_object);
		return;
	}
	cJSON_AddItemToObjectCS(ap_json_object, "clients", ap_json_array);
	ap_json_count = 0;
}
static void cli_json_object_init()
{
	cli_json_object = cJSON_CreateObject();
	if (!cli_json_object) {
		printf("creat cli object error!\n");
		return;
	}
	cJSON_AddStringToObject(cli_json_object, "topic", my_topic);
	cJSON_AddStringToObject(cli_json_object, "devmac", dev_mac_string);
	cli_json_array = cJSON_CreateArray();
	if (!cli_json_array) {
		printf("creat wifi array error!\n");
		cJSON_Delete(cli_json_object);
		return;
	}
	cJSON_AddItemToObjectCS(cli_json_object, "clients", cli_json_array);
	cli_json_count = 0;
}
static void _json_object_lock_init()
{
	pthread_mutex_init(&ap_json_lock, NULL);
	pthread_mutex_init(&cli_json_lock, NULL);
	pthread_mutex_init(&imsi_4g_lock, NULL);
	pthread_mutex_init(&imsi_3g_lock, NULL);
	pthread_mutex_init(&imsi_2g_lock, NULL);
}

static void json_object_2g_lock_lock()
{
	pthread_mutex_lock(&imsi_2g_lock);
}
static void json_object_2g_lock_unlock()
{
	pthread_mutex_unlock(&imsi_2g_lock);
}

static void json_object_3g_lock_lock()
{
	pthread_mutex_lock(&imsi_3g_lock);
}
static void json_object_3g_lock_unlock()
{
	pthread_mutex_unlock(&imsi_3g_lock);
}

static void json_object_4g_lock_lock()
{
	pthread_mutex_lock(&imsi_4g_lock);
}
static void json_object_4g_lock_unlock()
{
	pthread_mutex_unlock(&imsi_4g_lock);
}

/* init 4G IMSI json */
static void json_4G_object_init()
{
	if (imsi_4G_object != NULL) {
		cJSON_Delete(imsi_4G_object);
	}
	imsi_4G_object = cJSON_CreateObject();
	if (!imsi_4G_object) return;
	imsi_4G_array = cJSON_CreateArray();
	if (!imsi_4G_object){
		cJSON_Delete(imsi_4G_object);
		return;
	}
	cJSON_AddStringToObject(imsi_4G_object, "topic", my_topic);
	cJSON_AddItemToObjectCS(imsi_4G_object, "clients", imsi_4G_array);
	imsi_4G_count = 0;
}
/* init 3G IMSI json */
static void json_3G_object_init()
{
	json_object_3g_lock_lock();
	if (imsi_3G_object != NULL) {
		cJSON_Delete(imsi_3G_object);
	}
	imsi_3G_object = cJSON_CreateObject();
	if (!imsi_3G_object) return;
	imsi_3G_array = cJSON_CreateArray();
	if (!imsi_3G_object){
		cJSON_Delete(imsi_3G_object);
		return;
	}
	cJSON_AddStringToObject(imsi_3G_object, "topic", my_topic);
	cJSON_AddItemToObjectCS(imsi_3G_object, "clients", imsi_3G_array);
	imsi_3G_count = 0;
	json_object_3g_lock_unlock();
}
/* init 2G imsi object */
static void json_2G_object_init(char *ip)
{
	json_object_2g_lock_lock();
	if (imsi_2G_object != NULL) {
		cJSON_Delete(imsi_2G_object);
	}
	imsi_2G_object = cJSON_CreateObject();;
	if (!imsi_2G_object) return;
	imsi_2G_array = cJSON_CreateArray();
	if (!imsi_2G_object){
		cJSON_Delete(imsi_2G_object);
		return;
	}
	cJSON_AddStringToObject(imsi_2G_object, "topic", my_topic);
	cJSON_AddStringToObject(imsi_2G_object, "ip", ip);
	cJSON_AddItemToObjectCS(imsi_2G_object, "clients", imsi_2G_array);
	imsi_2G_count = 0;
	json_object_2g_lock_unlock();
}

/* AP */
static void ap_json_object_lock_lock()
{
	pthread_mutex_lock(&ap_json_lock);
}
static void ap_json_object_lock_unlock()
{
	pthread_mutex_unlock(&ap_json_lock);
}
/* cli */
static void cli_json_object_lock_lock()
{
	pthread_mutex_lock(&cli_json_lock);
}
static void cli_json_object_lock_unlock()
{
	pthread_mutex_unlock(&cli_json_lock);
}

static void send_ap_value_to_http()
{
	char *wifi_str= NULL;
	ap_json_object_lock_lock();
	if (ap_json_count == 0) {
		ap_json_object_lock_unlock();
		return;
	}
	wifi_str = cJSON_PrintUnformatted(ap_json_object);
	/* send to http server */

	http_send(args_info.urlap_arg, wifi_str, NULL, NULL);
	free(wifi_str);
	cJSON_Delete(ap_json_object);
	ap_json_object_init();
	ap_json_object_lock_unlock();
	return;
}
static void send_cli_value_to_http()
{
	char *wifi_str = NULL;
	cli_json_object_lock_lock();
	if (cli_json_count == 0) {
		cli_json_object_lock_unlock();
		return;
	}
	wifi_str = cJSON_PrintUnformatted(cli_json_object);
	/* send to http server */
	http_send(args_info.urlcli_arg, wifi_str, NULL, NULL);
	free(wifi_str);
	cJSON_Delete(cli_json_object);
	cli_json_object_init();
	cli_json_object_lock_unlock();
	return;
}

static void *thread_send_wifi_json_object(void *arg)
{
	while (1) {
		sleep(args_info.sendsta_itv_arg);
		send_ap_value_to_http();
		send_cli_value_to_http();
	} /* end while(1) */
	return NULL;
}
void *thread_recv_wifi_and_send(void *arg)
{
	int sockfd = -1;
	int retval = 0;
	socklen_t sin_size = sizeof(struct sockaddr);
	int i = 0;
	char buf[1024] = {0};
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr; 
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(args_info.wifi_port_arg); 
	my_addr.sin_addr.s_addr = INADDR_ANY;

	memset(&their_addr, 0, sizeof(their_addr));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(args_info.uport_arg + 1);
	inet_pton(AF_INET, args_info.uhost_arg,
		&their_addr.sin_addr.s_addr);

	bzero(&(my_addr.sin_zero), 8); 
	_json_object_lock_init();
	ap_json_object_init();
	cli_json_object_init();
	json_4G_object_init();
	json_3G_object_init();
udp_start:
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		return NULL;
	}
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		return NULL;
	}
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
		perror("bind");
		return NULL;
	}
	safe_pthread_create(thread_send_wifi_json_object, NULL);
	while(1) {
		memset(buf, '\0', retval);
		retval = recvfrom(sockfd, buf, 1028, 0,
			(struct sockaddr *)&my_addr, &sin_size);
		if (retval == 0) {
			close(sockfd);
			break;
		}
		if (retval > 0) {
			sendto(sockfd, buf, retval, 0,
				(struct sockaddr *)&their_addr,
				sizeof(struct sockaddr_in));
			cJSON *rec_object = cJSON_Parse(buf);
			if (rec_object) {
				cJSON *type = cJSON_GetObjectItem(rec_object,
					"type");
				if (!type) {
					cJSON_Delete(rec_object);
					continue;
				}
				if (!strncmp(type->valuestring, "CLI", 3)) {
					cli_json_object_lock_lock();
					cJSON_AddItemToArray(cli_json_array, rec_object);
					++cli_json_count;
					cli_json_object_lock_unlock();
				} else if (!strncmp(type->valuestring, "AP", 2)) {
					ap_json_object_lock_lock();
					cJSON_AddItemToArray(ap_json_array, rec_object);
					++ap_json_count;
					ap_json_object_lock_unlock();
				} else if (!strncmp(type->valuestring, "4G", 2)) {
					cJSON *p = NULL;
					char *str = NULL;
					cJSON *list = cJSON_GetObjectItem(rec_object, "clients");
					if (list && cJSON_GetArraySize(list)) {
						json_object_4g_lock_lock();
						for (i = 0; i < cJSON_GetArraySize(list); i++) {
							p = cJSON_GetArrayItem(list, i);
							str = cJSON_PrintUnformatted(p);
							cJSON *x = cJSON_Parse(str);
							cJSON_AddItemToArray(imsi_4G_array, x);
							imsi_4G_count++;
							free(str);
							cJSON *ipaddr = cJSON_GetObjectItem(x, "ip");
							if(!ipaddr)
								continue;
							band_entry_t *entry = band_list_entry_search_add(ipaddr->valuestring);
							pthread_mutex_lock(&(entry->lock));
							entry->change_count++;
							entry->_auto++;
							entry->work_mode = 0;
							entry->s_version = strdup("U_recv1.0"); 
							pthread_mutex_unlock(&(entry->lock));
						}
						if (imsi_4G_count >= 10) {
							char *send_str = cJSON_PrintUnformatted(imsi_4G_object);
							if (send_str) {
								pthread_mutex_lock(&http_send_lock);
								http_send_client(args_info.station_api_arg, send_str,
									NULL, NULL);
								pthread_mutex_unlock(&http_send_lock);
								free(send_str);
							}
						}
						json_object_4g_lock_unlock();
					}
					cJSON_Delete(rec_object);
				} else if (!strncmp(type->valuestring, "2G", 2)
					|| !strncmp(type->valuestring, "3G", 2)) {
					cJSON *p = NULL;
					char *str = NULL;
					cJSON *list = cJSON_GetObjectItem(rec_object, "clients");
					if (list && cJSON_GetArraySize(list)) {
						for (i = 0; i < cJSON_GetArraySize(list); i++) {
							p = cJSON_GetArrayItem(list, i);
							str = cJSON_PrintUnformatted(p);
							cJSON *x = cJSON_Parse(str);
							free(str);
							cJSON *ipaddr = cJSON_GetObjectItem(x, "ip");
							if(!ipaddr) continue;
							if(imsi_2G_object == NULL || imsi_2G_count >= 10) {
								json_2G_object_init(ipaddr->valuestring);
							}
							cJSON_AddItemToArray(imsi_2G_array, x);
							imsi_2G_count++;

							band_entry_t *entry
								= band_list_entry_search_add(ipaddr->valuestring);
							pthread_mutex_lock(&(entry->lock));
							entry->change_count++;
							entry->_auto++;
							entry->work_mode = 2;
							entry->s_version = strdup("U_recv1.0"); 
							pthread_mutex_unlock(&(entry->lock));
						}
					}
					if (imsi_2G_count >= 10) {
						char* send_str = cJSON_PrintUnformatted(imsi_2G_object);
						if (send_str) {
							pthread_mutex_lock(&http_send_lock);
							http_send(args_info.gsm_cdma_rep_api_arg, send_str, NULL, NULL);
							pthread_mutex_unlock(&http_send_lock);
							free(send_str);
						}
					}
					cJSON_Delete(rec_object);
				} else {
					cJSON_Delete(rec_object);
					continue;
				}
				if (cli_json_count >= args_info.max_wificache_arg)
					send_cli_value_to_http();
				if (ap_json_count >= args_info.max_wificache_arg)
					send_ap_value_to_http();
			} /* end if (rec_object) */
		} /* if (retval > 0) */
	} /* end while(1) */
	goto udp_start;
	return NULL;
}

