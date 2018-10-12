/*
 * ============================================================================
 *
 *   Project Name:  base_master
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年04月05日 16时32分28秒
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
#include <stdint.h>
#include <string.h>
#include <sys/file.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <mosquitto.h>
#include <syslog.h>                                                             
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "timer.h"
#include "arg.h"
#include "epoll_active.h"
#include "hash_list_active.h"
#include "cJSON.h"
#include "my_log.h"
#include "imsi_list_acton.h"
#include "bt_model_action.h"
#include "tcp_devid.h"

//char my_topic[30] = {0};/* MQTT-topic */
//char my_id[23] = {0}; /* MQTT-id */

F64 my_longitude = 0; /* 经度 */
F64 my_latitude = 0;  /* 纬度 */
F64 my_altitude = 0;  /* 高度 */

static unsigned char dev_mac[6];/* unsigned char mac */
char dev_mac_string[18] = {0}; /* char * mac */

pthread_mutex_t get_config_lock; // the lock for get config
/* U64 数字大小端转换 */
U64 my_htonll_ntohll(U64 in)
{
	U8 tmp_1[8] = {0}, tmp_2[8]={0};
	memcpy(tmp_1, &in, 8);
	U8 t;
	for (t = 0; t < 8; t++) {
		tmp_2[t] = tmp_1[8 - 1 - t];
	}
	U64 out;
	memcpy(&out, tmp_2, 8);
	return out;
}
/* U32 数字大小端转换 */
U32 my_htonl_ntohl(U32 in)
{
	U8 tmp_1[4] = {0}, tmp_2[4]={0};
	memcpy(tmp_1, &in, 4);
	U8 t;
	for (t = 0; t < 4; t++) {
		tmp_2[t] = tmp_1[4 - 1 - t];
	}
	U32 out;
	memcpy(&out, tmp_2, 4);
	return out;
}
/* U16 数字大小端转换 */
U16 my_htons_ntohs(U16 in)
{
	U8 tmp_1[2] = {0}, tmp_2[2]={0};
	memcpy(tmp_1, &in, 2);
	U8 t;
	for (t = 0; t < 2; t++) {
		tmp_2[t] = tmp_1[2 - 1 - t];
	}
	U16 out;
	memcpy(&out, tmp_2, 2);
	return out;
}

void my_btol_ltob(void *arg, int len)
{
	U8 *tmp_1 = malloc(len);
	U8 *tmp_2 = malloc(len);
	memcpy(tmp_1, arg, len);
	U8 t = 0;
	for (t = 0; t < len; t++) {
		tmp_2[t] = tmp_1[len - 1 - t];
	}
	memcpy(arg, tmp_2, len);
	if(tmp_1) free(tmp_1);
	if(tmp_2) free(tmp_2);
}

/* 检测并创建目录  creat directory */
void create_directory(const char *path_name)
{
	char dir_name[256] = {'\0'};
	strcpy(dir_name, path_name);
	int i, len = strlen(dir_name);
	if (dir_name[len - 1] != '/') {
		strcat(dir_name, "/");
	}
	len = strlen(dir_name);
	for (i = 1; i < len; i++) {
		if (dir_name[i] == '/') {
			dir_name[i] = 0;
			if (access(dir_name, 0) != 0) {
				if (mkdir(dir_name, 0755) == -1) {
					return;
				}
			}
			dir_name[i] = '/';
		}
	}
}
/* 获得网口的mac地址(eg:AA-BB-CC-DD-EE-FF) */
static int get_dev_mac(unsigned char* mac, char *ifname)
{
	struct ifreq tmp;
	int sock_mac, i;
	char mac_addr[30];
	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	if ( sock_mac == -1) {
		perror("create socket fail\n");
		printf("get device mac error!iface:[%s]\n", ifname);
		return -1;
	}
	memset(&tmp,0,sizeof(tmp));
	strncpy(tmp.ifr_name, ifname, sizeof(tmp.ifr_name)-1);
	if ((ioctl( sock_mac, SIOCGIFHWADDR, &tmp)) < 0 ) {
		close(sock_mac);
		printf("mac ioctl error\n");
		printf("get device mac error!iface:[%s]\n", ifname);
		return -1;
	}
	sprintf(mac_addr, "%02X-%02X-%02X-%02X-%02X-%02X",
		(unsigned char)tmp.ifr_hwaddr.sa_data[0],
		(unsigned char)tmp.ifr_hwaddr.sa_data[1],
		(unsigned char)tmp.ifr_hwaddr.sa_data[2],
		(unsigned char)tmp.ifr_hwaddr.sa_data[3],
		(unsigned char)tmp.ifr_hwaddr.sa_data[4],
		(unsigned char)tmp.ifr_hwaddr.sa_data[5]
	       );
	for (i = 0; i < 6; i++) {
		mac[i] = (unsigned char)tmp.ifr_hwaddr.sa_data[i];
	}
	close(sock_mac);
	return 0;
}

/* 定时修改基带设备的状态信息 */
static void *thread_change_baseband_state(void *arg)
{
	while(1) {
		sleep(args_info.bandonline_itv_arg / 2);
		if (!entry) continue;
		if (entry->change_count == 0)
			entry->online = 0;
		else
			entry->online = 1;
		if (entry->change_count > 1)
			entry->change_count = 1;
		else
			entry->change_count = 0;
		printf("基带板信息:\n");
		printf("     Online:%s\n", (entry->online)?"Online":"Offline");
		printf("  Work_mode:%s\n", (entry->work_mode ==TDD)?"TDD":"FDD");
		printf("Cell status:(%u)%s\n", entry->cellstate,
			(entry->cellstate == CELL_ACTIVE_YET)?"已激活":"未激活");
		printf(" Txpoweratt:%u\n", entry->powerderease);
		printf("     Regain:%u\n", entry->regain);
		printf("   up arfcn:%u\n", entry->sysUlARFCN);
		printf(" down arfcn:%u\n", entry->sysDlARFCN);
		printf("       plmn:%s\n", (S8*)entry->PLMN);
		printf("    sysBand:%u\n", entry->sysBand);
		printf("        PCI:%u\n", entry->PCI);
	} /* end while(... */
	return NULL;
}

/* 避免多个进程*/
static void one_instance()
{
	int pid_file = open("/var/run/mqttc.pid", O_CREAT | O_RDWR, 0666);
	int flags = fcntl(pid_file, F_GETFD);
	flags |= FD_CLOEXEC;
	fcntl(pid_file, F_SETFD, flags);
	int rc = flock(pid_file, LOCK_EX | LOCK_NB);
	if(rc) {
		if(EWOULDBLOCK == errno) {
			printf("Another base_master is running!\n");
			exit(-1);
		}
	}
}
#if 0
/* 获取mac地址并初始化MQTT身份信息 */
void get_mac_and_init_about_config()
{
	/* 获取mac地址 */
	if (get_dev_mac(dev_mac, "eth0") == -1) {
		exit(0);
	}
	snprintf(dev_mac_string, 18, "%02X-%02X-%02X-%02X-%02X-%02X",
		dev_mac[0], dev_mac[1], dev_mac[2],
		dev_mac[3], dev_mac[4], dev_mac[5]);
	snprintf(my_topic, 30, "%s.%02X-%02X-%02X-%02X-%02X-%02X",
		"sensor", dev_mac[0],dev_mac[1], dev_mac[2],
		dev_mac[3],dev_mac[4],dev_mac[5]);
	printf("My topic is :[%s]\n", my_topic);
	snprintf(my_id, 23, "mqttsub_%02x%02x%02x%02x%02x%02x",
		dev_mac[0], dev_mac[1], dev_mac[2],
		dev_mac[3], dev_mac[4], dev_mac[5]);
	printf("My mqttID is :[%s]\n", my_id);
}
#endif
/* 创建线程 */
void safe_pthread_create(void *(*start_routine) (void *), void *arg)
{
	pthread_attr_t attr; 
	pthread_t mythread;

	if(pthread_attr_init(&attr)) {
		printf("pthread_attr_init faild: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
	if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		printf("pthread_setdetach faild: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
	if(pthread_create(&mythread, &attr, start_routine, arg)) {
		printf("pthread_create faild: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
}

int main(int argc, char *argv[])
{
	proc_args(argc, argv); //初始化配置参数
	one_instance(); // 确保单一进程
	/* 是否开启打印 */
	//	if (args_info.debug_arg <= 4) {
	//		daemon(0, 0);
	//	}

	timer_init(); // init timer
	init_action_log();//初始化日志文件
	init_tcp_sockfd();//初始化tcp
	init_epoll();//初始化epoll
	//safe_pthread_create(thread_pare_imsi_action_list, NULL);//解析baicell IMSI链表
	//safe_pthread_create(thread_change_baseband_state, NULL);//监听基带板状态
	//safe_pthread_create(thread_read_send_baicell_json_object, NULL); /* 上传IMSI采集信息 */
	printf("tcp_state_arg:%d\n", args_info.tcp_state_arg);
	if(args_info.tcp_state_arg)
	{ 
		safe_pthread_create(thread_read_bluetooth_cmd, NULL); /* 与模块交互扫频 */
		safe_pthread_create(thread_connect_client, NULL);/*tcp server*/
		sleep(1);
		/*开始扫频*/
		char *buffer = (char *)calloc(1, 256);
		snprintf(buffer, 256, 
			"%s{\"type\":\"%x\",\"sweep\":\"%d\"}", 
			CMD_START, DEVICE_SCAN_ARFCN_REQ, 1);
		thread_pare_bluetooth_cmd_buf((void *)buffer);
	}
	else
		thread_recv_form_server(NULL);
	//thread_socket_to_baicell(NULL);//与Baicell基带板交互
	return 1;
}

