/*
 * =====================================================================================
 *
 *       Filename:  imsi_list_acton.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年05月30日 17时29分14秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __IMSI_LIST_ACTION__
#define __IMSI_LIST_ACTION__
#include "config.h"
#include "list.h"
#include "baicell_net.h"

typedef struct imsi_action_list {
	struct list_head node;
	S8 *ip;
	wrFLEnbToLmtUeInfoRpt_t ptr;
	time_t rtime;
} imsi_list_t;

void imsi_write_lock();
void imsi_write_unlock();

/* 初始化两个用于缓存imsi数据包的链表头，用户不同的线程处理imsi数据包 */
S32 init_imsi_list_heads();
/* 开启线程循环读取解析imsi链表中的包，并添加到json数据集合中 */
void *thread_pare_imsi_action_list(void *arg);
/* 将包含imsi结构信息的节点加入到write 链表中,以备线程读取 */
void *thread_add_point_to_imsi_list(void *arg);

#endif /* __IMSI_LIST_ACTION__ */
