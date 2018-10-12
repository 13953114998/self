/*
 * ============================================================================
 *
 *       Filename:  imsi_list_acton.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年05月30日 17时29分06秒
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
#include <unistd.h>
#include <errno.h>


#include "config.h"
#include "arg.h"
#include "my_log.h"
#include "imsi_list_acton.h"
#include "cJSON.h"
#include "hash_list_active.h"
#include "baicell_net.h"

/* 保存IMSI信息的链表(交替使用) */
struct list_head *imsi_head_one = NULL;
struct list_head *imsi_head_two = NULL;
struct list_head *imsi_head_allow_write; /* 当前允许写入的链表 */
struct list_head *imsi_head_allow_read; /* 当前正在读取的链表 */
static U32 count_in_write = 0;

pthread_mutex_t imsi_list_write_lock;

static void imsi_write_lock_init()
{
	pthread_mutex_init(&imsi_list_write_lock, NULL);
}
void imsi_write_lock()
{
	pthread_mutex_lock(&imsi_list_write_lock);
}
void imsi_write_unlock()
{
	pthread_mutex_unlock(&imsi_list_write_lock);
}

static S32 write_flag = 0; /* 表示当前正在写imsi链表编号 */

#define WRITTING_LIST_ONE (1)
#define WRITTING_LIST_TWO (2)


/* 初始化两个用于缓存imsi数据包的链表头，用户不同的线程处理imsi数据包 */
S32 init_imsi_list_heads()
{
	imsi_head_one = (struct list_head*)calloc(1, sizeof(struct list_head));
	if (!imsi_head_one) return -1;
	imsi_head_two = (struct list_head*)calloc(1, sizeof(struct list_head));
	if (!imsi_head_one) {
		free(imsi_head_one);
		return 0;
	}
	INIT_LIST_HEAD(imsi_head_one);
	INIT_LIST_HEAD(imsi_head_two);
	imsi_head_allow_write = imsi_head_one;
	imsi_head_allow_read = imsi_head_two;
	write_flag = 1;
	return 1;
}
/* 清空读取完成的链表,以备线程添加信息 */
void clear_imsi_action_list(struct list_head *del_head)
{
	imsi_list_t *tpos, *n;
	list_for_each_entry_safe(tpos, n, del_head, node) {
		list_del(&(tpos->node));
		free(tpos);
	}
	return;
}
/* 开启线程循环读取解析imsi链表中的包，并添加到json数据集合中 */
void *thread_pare_imsi_action_list(void *arg)
{
	imsi_list_t *tpos;
	S8 time_tmp[20] = {0};
	S32 len = 0;
	struct tm *timeinfo;
	S8 *send_str = NULL;
	S32 count = 0;
	cJSON *array = NULL;
	S8 rssi_t[5] = {0};
	do {
		if (!count_in_write) continue;
		imsi_write_lock();
		imsi_head_allow_read = imsi_head_allow_write;
		imsi_head_allow_write =
			(imsi_head_allow_write == imsi_head_one)? \
			imsi_head_two : imsi_head_one;
		write_flag = (write_flag == WRITTING_LIST_ONE)? \
			     WRITTING_LIST_TWO : WRITTING_LIST_ONE;
		imsi_write_unlock();
		bc_sta_json_obj_lock();
		/* imsi链表1正在写,本线程读取链表2,反之读取链表1
		 * 读取完成后将read链表和write链表进行调换,1秒钟之后再进行读取
		 * */
		array = get_bc_sta_array();
		list_for_each_entry(tpos, imsi_head_allow_read, node) {
			cJSON *object = cJSON_CreateObject();
			if (!object) continue;
			memset(time_tmp, 0, len);
			len = snprintf(time_tmp, 20, "%lu",
				(unsigned long)tpos->rtime);
			cJSON_AddStringToObject(object,	"timestamp", time_tmp);
			cJSON_AddStringToObject(object, "imsi",
				(char *)((tpos->ptr).IMSI));
			cJSON_AddStringToObject(object, "ip", tpos->ip);
			cJSON_AddStringToObject(object, "imei",
				(char *)((tpos->ptr).IMEI));
			snprintf(rssi_t, 5, "%u", (tpos->ptr).Rssi);
			cJSON_AddStringToObject(object, "rssi", rssi_t);
			/* add to imsi json array */
			cJSON_AddItemToArray(array, object);
			count++;
			printf("<----BAICELL_IMSI---->"
				"ip:%s, IMSI:%s, Rssi:%u, timestamp:%s\n",
				tpos->ip, (tpos->ptr).IMSI,
				(tpos->ptr).Rssi, time_tmp);
		}
		bc_sta_count_sum(count);
		count = 0;
#if 0
		/* 当json集合中的imsi信息数量达到上限,发送至后台服务器 */
		if (get_bc_sta_count() > args_info.max_stacache_arg) { 
			send_str = cJSON_PrintUnformatted(get_bc_sta_object());
			if (send_str) {
				printf("Send imsi info :%s\n", send_str);
				free(send_str);
				bc_sta_json_delete();
				bc_sta_json_init();
			}
		}
#endif
		bc_sta_json_obj_unlock();
		clear_imsi_action_list(imsi_head_allow_read);
	} while(1);
}
/* 将包含imsi结构信息的节点加入到write 链表中,以备线程读取信息 */
void *thread_add_point_to_imsi_list(void *arg)
{
	imsi_list_t *i_point = (imsi_list_t *)arg;
	imsi_write_lock();
	list_add_tail(&(i_point->node), imsi_head_allow_write);	
	count_in_write++;
	imsi_write_unlock();
	return NULL;
}

