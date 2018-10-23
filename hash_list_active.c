/*
 * ============================================================================
 *
 *       Filename: hash_list_active.c 
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年04月19日 09时17分49秒
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
#include <dirent.h>
#include <sys/types.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "arg.h"
#include "config.h"
#include "list.h"
#include "hash_list_active.h"
#include "cmdline.h"
#include "timer.h"
#include "save2sqlite.h"
#include "mosquit_sub.h"
#include "baicell_net.h"
#ifdef SYS86
#include "gpio_ctrl.h"
#endif
#include "cJSON.h"

/* Hash function */
/* Hash function by ipaddr (string)*/
static int Hash_By_ip(char *ipin) {
	if (!ipin)
		return 0;
	int tmp;
	int hash;
	sscanf(ipin, "%d.%d.%d.%d", &tmp, &tmp, &tmp, &hash);
	return hash;
}

/* init function */
static void _hash_init(hash_list_t **init_head)
{
	*init_head = (hash_list_t *)calloc(MAX_HLIST_LENGTH, sizeof(hash_list_t));
	if (init_head == NULL) {
		printf("%s:malloc error:%s(%d)",
			__func__, strerror(errno), errno);
	}
	int i;
	hash_list_t *ptr = *init_head;
	for (i = 0; i < MAX_HLIST_LENGTH; i++) {
		pthread_mutex_init(&(ptr[i].list_lock), NULL);
		ptr[i].entry_count = 0;
		INIT_HLIST_HEAD(&ptr[i].head);
	}
}
/* 计数(+) 未加锁*/
void get_entry_nolock(client_hentry_t *entry)
{
	entry->count++;
}
/* 计数(+) 加锁 */
void get_entry(client_hentry_t *entry)
{
	pthread_mutex_lock(&entry->lock);
	entry->count++;
	pthread_mutex_unlock(&entry->lock);
}
/* ================set 1 or set 0===================== */
void get_entry_lockself(client_hentry_t *entry)
{
	pthread_mutex_lock(&entry->lock);
	entry->count = 1;
	pthread_mutex_unlock(&entry->lock);
}
static void put_entry_lockself(client_hentry_t *entry)
{
	pthread_mutex_lock(&entry->lock);
	if (entry->count > 1)
		entry->count = 1;
	else
		entry->count = 0;
	pthread_mutex_unlock(&entry->lock);
}

/* 超时 put_entry */
void *check_put_entry_nolock(void *arg)
{
	client_hentry_t *entry = (client_hentry_t *)arg;
	while (1) {
		sleep(args_info.bandonline_itv_arg);
		put_entry_lockself(entry);
	}
}
/* ===End=============set 1 or set 0===================== */
/* 计数(-)(加锁)  (如果count == 0 表示已经下线，删除节点)*/
void put_entry(client_hentry_t *entry)
{
	pthread_mutex_lock(&(entry->hash_head[entry->hash_pos].list_lock));
	pthread_mutex_lock(&entry->lock);
	if(!(--(entry->count))) {
		hlist_del(&entry->node);
		pthread_mutex_unlock(&entry->lock);
		pthread_mutex_unlock(&(entry->hash_head[entry->hash_pos].list_lock));
		free(entry);
		return;
	}
	pthread_mutex_unlock(&entry->lock);
	pthread_mutex_unlock(&(entry->hash_head[entry->hash_pos].list_lock));
}

/* 将节点添加到哈希桶中对应的链表 */
static void hash_list_insert(hash_list_t *hash_head,
	int hash_pos, client_hentry_t *entry)
{
	hlist_add_head(&(entry->node), &hash_head[hash_pos].head);
}
/* 查找节点 不加锁*/
static client_hentry_t * _hash_search_nolock(hash_list_t *hash_head,
	char *ipin, int hash_pos)
{
	client_hentry_t *tpos = NULL, *entry = NULL;
	struct hlist_node *pos = NULL;
	hlist_for_each_entry(tpos, pos,
		&hash_head[hash_pos].head, node) {
		if (!memcmp(tpos->ipaddr, ipin, strlen(ipin))) {
			tpos->hash_pos = hash_pos;
			entry = tpos;
			break;
		}
	}
	return entry;
}

client_hentry_t *hash_list_search_nolock(hash_list_t *hash_head, char *ipin)
{
	if (!ipin) return NULL;
	int hash_pos;
	hash_pos = Hash_By_ip(ipin);
	client_hentry_t *entry = NULL;
	entry = _hash_search_nolock(hash_head, ipin, hash_pos);
	if (entry)
		get_entry(entry);
	return entry;
}

/* 查找节点 */
client_hentry_t *hash_list_search(hash_list_t *hash_head, char *ipin)
{
	if (!ipin) return NULL;
	int hash_pos;
	hash_pos = Hash_By_ip(ipin);
	client_hentry_t *entry = NULL;
	pthread_mutex_lock(&(hash_head[hash_pos].list_lock));
	entry = _hash_search_nolock(hash_head, ipin, hash_pos);
	if (entry)
		get_entry(entry);
	pthread_mutex_unlock(&(hash_head[hash_pos].list_lock));
	return entry;
}
/* 超时 put_entry */
static void devhash_timeout(void *arg)
{
	client_hentry_t *entry = arg;
	put_entry(entry);
}
/* 查找并添加新的节点 */
client_hentry_t *hash_list_search_new(hash_list_t *hash_head, char *ipin)
{
	int hash_pos;
	hash_pos = Hash_By_ip(ipin);
	client_hentry_t *entry = NULL;
	pthread_mutex_lock(&(hash_head[hash_pos].list_lock));
	entry = _hash_search_nolock(hash_head, ipin, hash_pos);
	if(!entry) {
		entry = (client_hentry_t *)malloc(sizeof(client_hentry_t));
		if (entry == NULL) {
			printf("Function:%s Line:%d, malloc error:%s(%d)",
				 __func__, __LINE__, strerror(errno), errno);
		}
		pthread_mutex_init(&entry->lock, NULL);
		entry->hash_pos = hash_pos;
		entry->count = 0;
		entry->hash_head = hash_head;
		entry->ipaddr = strdup(ipin);
		entry->sockfd = -1;
		get_entry_lockself(entry);
		hash_list_insert(hash_head, hash_pos, entry);
		safe_pthread_create(check_put_entry_nolock, (void*)entry);//创建线程检测设备在线状态
		hash_head[hash_pos].entry_count += 1; //哈希桶(hash_pos)节点数加一
	} else {
		/* search */
		get_entry(entry);
	}
	pthread_mutex_unlock(&(hash_head[hash_pos].list_lock));
	return entry;
}

/* 初始化哈希表 */
void hash_init()
{
	_hash_init(&hlist_clients_head);
}

/* ================== ============================ ================== */
/* ================== 将 baseband 信息放置在链表中 ================== */
/* ================== ============================ ================== */
/* 基带板 信息链表头 */
static struct list_head *band_list_head;

void change_set_client_count(void *arg) {
	if (!arg) return;
	band_entry_t *entry = (band_entry_t *)arg;
	/* if cell deactive return */
	if (entry->cellstate != 3) {
		goto the_end;
	}
	if (entry->client_find_count > 0) {
		entry->client_find_count = 0;
		entry->client_no_find_count = 0;
	} else {
		entry->client_find_count = 0;
		entry->client_no_find_count++;
	}
	if (entry->client_no_find_count >= 4) {
		entry->client_no_find_count = 0;
		printf("基带板(IP:%s)已经两个小时未上号,需要将其重启!\n",
			entry->ipaddr);
		send_reboot_request_to_baicell(entry->ipaddr);
	}
the_end:
		timer_add(30 * 60, 0, change_set_client_count, (void *)entry);
		return;
}

void band_list_head_init()
{
	band_list_head = (struct list_head*)calloc(1, sizeof(struct list_head));
	INIT_LIST_HEAD(band_list_head);
	if (pthread_mutex_init(&band_list_lock, NULL)) {
		printf("%s:init lock failed,%s(%d)\n",
			__func__, strerror(errno), errno);
		exit(-1);
	}
	band_entry_count = 0;
}
/* 外部获取 band_list_head 接口*/
struct list_head *return_band_list_head()
{
	return band_list_head;
}

// 创建新的节点,通过ipaddr
static band_entry_t *band_list_entry_new(char *ipaddr)
{
	band_entry_t *new_entry = (band_entry_t *)calloc(1, sizeof(band_entry_t));
	if (!new_entry) {
		printf("%s:malloc failed,%s(%d)\n",
			__func__, strerror(errno), errno);
		exit(-1);
	}
	if (pthread_mutex_init(&(new_entry->lock), NULL)) {
		printf("%s:init lock failed,%s(%d)\n",
			__func__, strerror(errno), errno);
		exit(-1);
	}
	new_entry->change_count++;
	new_entry->online = 1;
	new_entry->sockfd = -1;
	new_entry->client_find_count = 0;
	new_entry->client_no_find_count = 0;
	new_entry->ipaddr = strdup(ipaddr);
#ifndef VMWORK
#ifdef SYS86
	new_entry->pa_num = get_base_pa_addr_by_ip(ipaddr);
	new_entry->gpio_num = get_base_switch_gpio_by_ip(ipaddr);
#endif
#endif
	return new_entry;
}
/* 根据ipaddr 查找并返回节点地址 */
band_entry_t *band_list_entry_search(char *ipaddr)
{
	if (!ipaddr) return NULL;
	band_entry_t *pos;
	if (band_list_head == NULL) {
		printf("band_list_head is NULL\n");
	}
	list_for_each_entry(pos, band_list_head, node) {
		if (strcmp(pos->ipaddr, ipaddr) == 0) {
			return pos;
		}
	}
	return NULL;
}

/* 根据ipaddr 查找并返回节点地址若没有查找到则新建一个 */
band_entry_t *band_list_entry_search_add(char *ipaddr)
{
	assert(ipaddr);
	band_entry_t *entry = band_list_entry_search(ipaddr);
	if (!entry) {
		entry = band_list_entry_new(ipaddr);
		printf("new_entry->ipaddr:%s\n", entry->ipaddr);
		/* 将新建的节点插入到head中 */
		pthread_mutex_lock(&band_list_lock); //加总锁
		band_entry_count++;
		list_add_tail(&(entry->node), band_list_head);
		pthread_mutex_unlock(&band_list_lock);//解总锁
		timer_add(30 * 60, 0, change_set_client_count, (void *)entry);
	}
	return entry;
}
void band_list_delete(char *ipaddr)
{
	if (!ipaddr)
		return;
	band_entry_t *entry = band_list_entry_search(ipaddr);
	if (entry) {
		pthread_mutex_lock(&band_list_lock); //加总锁
		list_del(&(entry->node));
		pthread_mutex_unlock(&band_list_lock);//解总锁
		if (entry->sn) free(entry->sn);
		if (entry->ipaddr) free(entry->ipaddr);
		if (entry->macaddr) free(entry->macaddr);
		if (entry->s_version) free(entry->s_version);
		if (entry->dev_model) free(entry->dev_model);
		if (entry->h_version) free(entry->h_version);
		free(entry);
	}
	return;
}

/* 读取基带板信,组合成json字符串返回调用者
 * 要获取所有基带板数据, 将addr至为NULL
 * 要获取某个基带板数据, 将addr至为想要查询的基带板的IP地址
 */
/* WCDMA  (Not use)*/
char *get_wcdma_base_band_status()
{
	if (!band_entry_count) return NULL;
	cJSON *clients_object = cJSON_CreateObject();
	if (!clients_object) return NULL;
	cJSON *clients_array = cJSON_CreateArray();
	if (!clients_array) {
		cJSON_Delete(clients_object);
		return NULL;
	}
	cJSON_AddStringToObject(clients_object, "topic", my_topic);
	cJSON_AddItemToObjectCS(clients_object, "stations", clients_array);
	int need_send = 0;
	band_entry_t *tpos, *n;
	pthread_mutex_lock(&band_list_lock);
	list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
		if (tpos->work_mode == WCDMA_BASE_DEVICE) {
			cJSON *object_in_arry = cJSON_CreateObject();
			if (!object_in_arry) continue;
			need_send = 1;
			pthread_mutex_lock(&(tpos->lock));
			cJSON_AddStringToObject(object_in_arry,
				"online", (tpos->online)?"1":"0");
			cJSON_AddStringToObject(object_in_arry,
				"ip", (tpos->ipaddr)?tpos->ipaddr:"");
			cJSON_AddStringToObject(object_in_arry,
				"mac", (tpos->macaddr)?tpos->macaddr:"");
			cJSON_AddNumberToObject(object_in_arry,
				"pa_num", (int)tpos->pa_num);
			cJSON_AddStringToObject(object_in_arry,
				"sn", "N/A");
			cJSON_AddNumberToObject(object_in_arry,
				"work_mode", tpos->work_mode);
			cJSON_AddStringToObject(object_in_arry,
				"software_version", tpos->s_version);
			cJSON_AddStringToObject(object_in_arry,
				"hardware_version", "N/A");
			cJSON_AddStringToObject(object_in_arry,
				"device_model", "N/A");
			pthread_mutex_unlock(&(tpos->lock));
			cJSON_AddItemToArray(clients_array, object_in_arry);
		}
	}
	pthread_mutex_unlock(&band_list_lock);
	char *json_str = cJSON_PrintUnformatted(clients_object);
	cJSON_Delete(clients_object);
	if (!json_str) {
		printf("get WCDMA dev info json string error!\n");
		return NULL;
	}
	if (need_send) {
		return json_str;
	}
	free(json_str);
	return NULL;
}

/* GSM */
char *get_gsm_base_band_status()
{
	if (!band_entry_count) return NULL;
	cJSON *clients_object = cJSON_CreateObject();
	if (!clients_object) return NULL;
	cJSON *clients_array = cJSON_CreateArray();
	if (!clients_array) {
		cJSON_Delete(clients_object);
		return NULL;
	}
	cJSON_AddStringToObject(clients_object, "topic", my_topic);
	cJSON_AddItemToObjectCS(clients_object, "stations", clients_array);
	int need_send = 0;
	band_entry_t *tpos, *n;
	pthread_mutex_lock(&band_list_lock);
	list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
		if (tpos->work_mode != 2 && tpos->work_mode != 3) {
			continue;
		}
		if (tpos->macaddr == NULL) {
			char arp_scmd[128] = {0}, gsm_mac[18] = {0};
#ifdef VMWORK
			snprintf(arp_scmd, 128,
				"arp |grep '%s' |awk '{print $3}' |tr '[a-z]' '[A-Z]'",
				tpos->ipaddr);
#else
			/* in openwrt */
			snprintf(arp_scmd, 128,
				"arp |grep '%s' |awk '{print $4}' |tr '[a-z]' '[A-Z]'",
				tpos->ipaddr);
#endif
			FILE *pfile = popen(arp_scmd, "r");
			if (!pfile) {
				perror("popen error");
			}
			fscanf(pfile, "%s", gsm_mac);
			pclose(pfile);
			tpos->macaddr = strdup(gsm_mac);
		}
		cJSON *object_in_arry = cJSON_CreateObject();
		if (!object_in_arry) {
			continue;
		}
		need_send = 1;
		pthread_mutex_lock(&(tpos->lock));
		cJSON_AddStringToObject(object_in_arry,	"ip", tpos->ipaddr);
		cJSON_AddStringToObject(object_in_arry,	"online", (tpos->online)?"1":"0");
		cJSON_AddNumberToObject(object_in_arry,	"work_mode", tpos->work_mode);
		cJSON_AddStringToObject(object_in_arry,	"software_version", tpos->s_version);
		cJSON_AddStringToObject(object_in_arry,	"hardware_version", "N/A");
		cJSON_AddStringToObject(object_in_arry,	"device_model", "N/A");
		cJSON_AddStringToObject(object_in_arry,	"pa_num", "N/A");
		cJSON_AddStringToObject(object_in_arry,	"mac", (tpos->macaddr)?(tpos->macaddr):"N/A");
		cJSON_AddStringToObject(object_in_arry,	"sn", "N/A");
		pthread_mutex_unlock(&(tpos->lock));
		cJSON_AddItemToArray(clients_array, object_in_arry);
	}
	pthread_mutex_unlock(&band_list_lock);
	char *json_str = cJSON_PrintUnformatted(clients_object);
	cJSON_Delete(clients_object);
	if (!json_str) {
		printf("get GSM dev info json string error!\n");
		return NULL;
	}
	if (need_send) {
		return json_str;
	}
	printf("well ,needn't send anything(GSM)!\n");
	free(json_str);
	return NULL;
}
/* LTE */
char *get_lte_base_band_status(U8 get_type, S8 *addr)
{
	if (!band_entry_count)
		return NULL;
	cJSON *clients_object = cJSON_CreateObject();
	if (!clients_object)
		return NULL;
	cJSON_AddStringToObject(clients_object, "topic", my_topic);
	cJSON *clients_array = cJSON_CreateArray();
	if (!clients_array) {
		cJSON_Delete(clients_object);
		return NULL;
	}
	int need_send = 0;
	band_entry_t *tpos, *n;
	pthread_mutex_lock(&band_list_lock);
	list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
		if (tpos->work_mode != FDD && tpos->work_mode != TDD) {
			/* it is not Baicell LTE device */
			continue;
		}
		if (addr && strncmp(addr, tpos->ipaddr, strlen(addr))) {
			/* get once of by ip and this ipaddr is not ip */
			continue;
		}
		cJSON *object_in_arry = cJSON_CreateObject();
		if (!object_in_arry) {
			printf("creat json object in arry ERROR!\n");
			continue;
		}
		need_send = 1;
		pthread_mutex_lock(&(tpos->lock));
		if (get_type & GET_DEFAULT_CONFIG) {
			cJSON_AddStringToObject(object_in_arry,
				"online", (tpos->online)?"1":"0");
			cJSON_AddStringToObject(object_in_arry,
				"ip", (tpos->ipaddr)?tpos->ipaddr:"N/A");
			cJSON_AddStringToObject(object_in_arry,
				"mac", (tpos->macaddr)?(tpos->macaddr):"N/A");
			cJSON_AddNumberToObject(object_in_arry,
				"pa_num", (int)tpos->pa_num);
			cJSON_AddStringToObject(object_in_arry,
				"sn", (tpos->sn)?(tpos->sn):"N/A");
			cJSON_AddNumberToObject(object_in_arry,
				"work_mode", tpos->work_mode);
			cJSON_AddStringToObject(object_in_arry,
				"device_model", "Baicells");
			cJSON_AddStringToObject(object_in_arry,
				"software_version",
				(tpos->s_version)?(tpos->s_version):"N/A");
			cJSON_AddStringToObject(object_in_arry,
				"hardware_version",
				(tpos->h_version)?(tpos->h_version):"N/A");
			cJSON_AddStringToObject(object_in_arry,
				"uboot_version",
				(tpos->ubt_version)?(tpos->ubt_version):"N/A");
			cJSON_AddStringToObject(object_in_arry,
				"board_temp",
				(tpos->board_temp)?(tpos->board_temp):"N/A");
		}
		if (get_type & GET_STATUS_CONFIG) {
			cJSON *state_object = cJSON_CreateObject();
			cJSON_AddNumberToObject(state_object,
				"sync_type", (U32)tpos->u16RemMode);
			cJSON_AddNumberToObject(state_object,
				"sync_state", (U32)tpos->u16SyncState);
			cJSON_AddNumberToObject(state_object,
				"cell_state", tpos->cellstate);
			cJSON_AddNumberToObject(state_object,
				"regain", (U32)tpos->regain);
			cJSON_AddNumberToObject(state_object,
				"power_derease", (U32)tpos->powerderease);
			cJSON_AddItemToObject(object_in_arry,
				"state",state_object);
		}
		if (get_type & GET_FREQ_CONFIG) {
			cJSON *freq_object = cJSON_CreateObject();
			cJSON_AddNumberToObject(freq_object,
				"sysuiarfcn", (int)tpos->sysUlARFCN);
			cJSON_AddNumberToObject(freq_object,
				"sysdiarfcn", (int)tpos->sysDlARFCN);
			cJSON_AddStringToObject(freq_object,
				"plmn", (char *)tpos->PLMN);
			cJSON_AddNumberToObject(freq_object,
				"sysbandwidth", (int)tpos->sysBandwidth);
			cJSON_AddNumberToObject(freq_object,
				"sysband", (int)tpos->sysBand);
			cJSON_AddNumberToObject(freq_object,
				"pci", (int)tpos->PCI);
			cJSON_AddNumberToObject(freq_object,
				"tac", (int)tpos->TAC);
			cJSON_AddNumberToObject(freq_object,
				"cellid", (int)tpos->CellId);
			cJSON_AddNumberToObject(freq_object,
				"uepmax", (int)tpos->UePMax);
			cJSON_AddNumberToObject(freq_object,
				"enodebpmax", (int)tpos->EnodeBPMax);
			cJSON_AddItemToObject(object_in_arry,
				"freq_conf", freq_object);
		}
		pthread_mutex_unlock(&(tpos->lock));
		cJSON_AddItemToArray(clients_array, object_in_arry);
	} /* end list_for_each_entry_safe(... */
	pthread_mutex_unlock(&band_list_lock);

	cJSON_AddItemToObjectCS(clients_object, "stations", clients_array);
	char *json_str = cJSON_PrintUnformatted(clients_object);
	cJSON_Delete(clients_object);
	if (!json_str) {
		printf("get json string error!\n");
		return NULL;
	}
	if (need_send) {
		return json_str;
	}
	printf("well ,needn't send anything(LTE)!\n");
	free(json_str);
	return NULL;
}

/* =========================================================================== */
