/*
 * ============================================================================
 *
 *       Filename:  cli_action.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年10月19日 17时59分59秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zlib.h>

#include "mosquit_sub.h"
#include "cmdline.h"
#include "arg.h"
#include "epoll_active.h"
#include "cJSON.h"
#include "config.h"
#include "hash_list_active.h"

#include "tcp_wcdma.h"
#include "cli_action.h"
#include "my_log.h"

/* set list */
static struct list_head *cli_reqest_list_head = NULL; //cli 请求链表头
pthread_mutex_t cli_reqest_list_lock;  //操作cli请求链表的锁

static void init_cli_req_list_lock()
{
	pthread_mutex_init(&cli_reqest_list_lock, NULL);
}

static void lock_cli_req_list_lock()
{
	pthread_mutex_lock(&cli_reqest_list_lock);
}

static void unlock_cli_req_list_lock()
{
	pthread_mutex_unlock(&cli_reqest_list_lock);
}

void add_entry_to_clireq_list(struct cli_req_if info)
{
	cli_req_t *entry = (cli_req_t *)calloc(1, sizeof(cli_req_t));
	if (!entry) {
		printf("creat new cli_req_t entry failed!\n");
		return;
	}
	entry->info.sockfd = info.sockfd;
	entry->info.msgtype = info.msgtype;
	strcpy((char *)(entry->info.ipaddr), (char *)(info.ipaddr));

	lock_cli_req_list_lock();
	list_add_tail(&(entry->node), cli_reqest_list_head);
	unlock_cli_req_list_lock();
	return;
}

cli_req_t *entry_to_clireq_list_search(struct cli_req_if info)
{
	cli_req_t *pos, *n;
	if (cli_reqest_list_head == NULL) {
		return NULL;
	}
	lock_cli_req_list_lock();
	list_for_each_entry_safe(pos, n, cli_reqest_list_head, node) {
		if ((info.msgtype == pos->info.msgtype)
			&& !(strcmp((char *)(info.ipaddr), (char *)(pos->info.ipaddr)))) {
			unlock_cli_req_list_lock();
			return pos;
		}
	}
	unlock_cli_req_list_lock();
//	printf("not find cli entry!\n");
	return NULL;
}

static void cli_reqest_list_head_init()
{
	cli_reqest_list_head =
		(struct list_head *)calloc(1, sizeof(struct list_head));
	INIT_LIST_HEAD(cli_reqest_list_head);
}

void del_entry_to_clireq_list(cli_req_t *entry)
{
	if (!entry) return;
	lock_cli_req_list_lock();
	list_del(&(entry->node));
	unlock_cli_req_list_lock();
	free(entry);
}

static int init_unix_socke(char *unix_domain)
{
	if (!unix_domain) {
		printf("not input unix_domain!\n");
		exit(-1);
	}
	int new_sockfd;
	struct sockaddr_un server_addr;
	unlink(unix_domain);
	new_sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (new_sockfd == -1) {
		perror("CLI server get tcp socket error:");
		exit(-1);
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, unix_domain,
		sizeof(server_addr.sun_path) - 1);
	/* bind socket */
	int ret = bind(new_sockfd,
		(struct sockaddr*)&server_addr, sizeof(server_addr));
	if(ret == -1) {
		perror("CLI server cannot bind server socket");
		close(new_sockfd);
		exit(-1);
	}
	return new_sockfd;
}

/* ====================== Unix socket accept cli connect ==================== */
void *thread_usocket_with_CLI(void *arg)
{
	char recv_buffer[1024] = {0};
	char no_info_str[128] = {0};
	if (!access(CLI_TO_BASE_MASTER_UNIX_DOMAIN, F_OK)) {
		remove(CLI_TO_BASE_MASTER_UNIX_DOMAIN);
	}
	int cli_server_sockfd =	init_unix_socke(CLI_TO_BASE_MASTER_UNIX_DOMAIN);
	int cli_fd;
	int length;
	struct sockaddr_un client_addr;

	init_cli_req_list_lock(); /* init lock */
	cli_reqest_list_head_init(); /* init list head */

	/* listen sockfd */
	int ret = listen(cli_server_sockfd, 10);
	if(ret == -1) {
		perror("cannot listen the client connect request");
		close(cli_server_sockfd);
		exit(-1);
	}
	while(1) {
		length = sizeof(client_addr);
		cli_fd = accept(cli_server_sockfd,
			(struct sockaddr*)&client_addr, (socklen_t *)&length);
		if (cli_fd < 0) {
			perror("cannot accept client connect request");
			continue;
		}
		/* read and printf sent client info */
		memset(recv_buffer, 0, 1024);
		int num = read(cli_fd, recv_buffer, 4);
		if( num < 0 ) {
			perror("CLI server read error:");
			continue;
		}

		if (!strncmp(recv_buffer, "AAAA", 4)) {
			num = read(cli_fd, recv_buffer, 1024);
			if( num < 0 ) {
				perror("CLI server read error:");
				continue;
			}
			printf("<<<<<<<<<<<<Get command from CLI\n");
			U16 type = 0;
			memcpy(&type, recv_buffer, 2);
			switch(type) {
			case CLI_TO_BASE_MASTER_GET_LTE_DEV_INFO:
				printf("<<<<<<<<<<<<get lte base device information\n");
				write_action_log("<<<<<<<<get command from CLI:",
					"get lte base device information");
				char *json_string =
					get_lte_base_band_status(
						GET_DEFAULT_CONFIG
						| GET_STATUS_CONFIG
						| GET_FREQ_CONFIG, NULL);
				if (json_string == NULL) {
					memset(no_info_str, 0, 128);
					snprintf(no_info_str, 128, "nothing");
					write(cli_fd, no_info_str,
						sizeof(no_info_str));
					break;
				}
				write(cli_fd, json_string,
					strlen(json_string) + 1);
				free(json_string);
				close(cli_fd);
				break;
			case CLI_TO_BASE_MASTER_GET_WCDMA_DEV_INFO:
				printf("<<<<<<<<<<<<get WCDMA base device information\n");
				write_action_log("<<<<<<<<get command from CLI:",
					"get WCDMA base device information");
				struct cli_req_if cli_info;
				cli_info.sockfd = cli_fd;
				cli_info.msgtype = type;
				memset(cli_info.ipaddr, 0, 20);
				band_entry_t *tpos = NULL;
				band_entry_t *n = NULL;
				pthread_mutex_lock(&band_list_lock);
				list_for_each_entry_safe(tpos, n,
					return_band_list_head(), node) {
					/* WCDMA base band device */
					if (tpos->work_mode ==	WCDMA_BASE_DEVICE) {
						strcpy((char *)(cli_info.ipaddr), tpos->ipaddr);
						break;
					}
				}
				pthread_mutex_unlock(&band_list_lock);
				if (!cli_info.ipaddr[0])
					goto send_and_break;
				add_entry_to_clireq_list(cli_info);
				cJSON *object = cJSON_CreateObject();
				if (!object) {
					goto send_and_break;
				}
				cJSON_AddStringToObject(object,
					"msgtype", "F082");
				cJSON_AddStringToObject(object,
					"ip", (char *)cli_info.ipaddr);
				char *pare_str = cJSON_PrintUnformatted(object);
				cJSON_Delete(object);
				if (!pare_str)
					goto send_and_break;
				pare_config_json_message(pare_str, cli_fd);
				break;
send_and_break:
				memset(no_info_str, 0, 128);
				snprintf(no_info_str, 128, "nothing");
				write(cli_fd, no_info_str,
					sizeof(no_info_str));
				close(cli_fd);
				break;
			case CLI_TO_BASE_MASTER_GET_GSM_DEV_INFO:
				printf("<<<<<<<<<<<<get GSM base device information\n");
				write_action_log("<<<<<<<<get command from CLI:",
					"get GSM base device information");
				char *send_str = get_gsm_base_band_status();
				if (send_str) {
					write(cli_fd, send_str, strlen(send_str));
					close(cli_fd);
					cli_fd = -1;
					free(send_str);
				} else {
					memset(no_info_str, 0, 128);
					snprintf(no_info_str, 128, "nothing");
					write(cli_fd, no_info_str, sizeof(no_info_str));
					close(cli_fd);
					cli_fd = -1;
				}
				break;
			default:
				write_action_log("<<<<<<<<get command from CLI:",
					recv_buffer);
				pare_config_json_message(recv_buffer, cli_fd);
				break;
			} /* end switch(type)... */
		} else {
			printf("str{%d}:%s\n", num, recv_buffer);
		} /* end if (!strncmp(recv_buffer, "AAAA", 4)) ... else ...*/
	} /* end while (1)... */
	close(cli_server_sockfd);
	return NULL;
}
