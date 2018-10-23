/*
 * =====================================================================================
 *
 *       Filename:  cli_action.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年10月19日 18时01分51秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __CLI_ACTION_H__
#define __CLI_ACTION_H__
#include "hash_list_active.h"

#define CLI_TO_BASE_MASTER_GET_LTE_DEV_INFO   (0xB001) /* get LTE BaseBand device information */
#define CLI_TO_BASE_MASTER_GET_WCDMA_DEV_INFO (0xB002) /* get WCDMA BaseBand device information */
#define CLI_TO_BASE_MASTER_GET_GSM_DEV_INFO   (0xB003) /* get GSM BaseBand device information */
#define CLI_TO_BASE_MASTER_UNIX_DOMAIN "/tmp/CLI.usock"

#pragma pack(1)
struct cli_req_if {
	int sockfd;
	U8 ipaddr[20];
	U16 msgtype;
};
typedef struct cli_reqest_list_struct {
	struct list_head node; // pre and next
	struct cli_req_if info;
} cli_req_t;
#pragma pack()


void add_entry_to_clireq_list(struct cli_req_if info);
cli_req_t *entry_to_clireq_list_search(struct cli_req_if info);
void del_entry_to_clireq_list(cli_req_t *entry);

void *thread_usocket_with_CLI(void *arg);


#endif /* #ifdef __CLI_ACTION_H__ */
