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
#include "mosquit_sub.h"
#include "http.h"
#include "timer.h"
#include "arg.h"
#include "epoll_active.h"
#ifdef SCACHE
#include "cache_file.h"
#endif
#include "tcp_wcdma.h"
#include "udp_gsm.h"
#include "hash_list_active.h"
#include "cJSON.h"
#include "my_log.h"
#ifdef SAVESQL
#include "save2sqlite.h"
#endif
#include "cli_action.h"
#include "fiberhome_lte.h"
#include "udp_wifi.h"
#include "imsi_list_acton.h"
#ifdef AUTH
#include "auth.h"
#endif
#ifndef VMWORK
#ifdef SYS86
#include "gpio_ctrl.h"
#include "gpio_serial.h"
#endif
#endif
#include "scan_fre.h"

char my_topic[30] = {0};/* MQTT-topic */
char my_id[23] = {0}; /* MQTT-id */

F64 my_longitude = 0; /* 经度 */
F64 my_latitude = 0;  /* 纬度 */
F64 my_altitude = 0;  /* 高度 */

static unsigned char dev_mac[6];/* unsigned char mac */
struct mosquitto *mosq = NULL; /* MQTT config */
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
	band_entry_t *tpos;
	band_entry_t *n;
	while(1) {
		sleep(args_info.bandonline_itv_arg / 2);
		list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
			if (tpos->change_count == 0)
				tpos->online = 0;
			else
				tpos->online = 1;
			if (tpos->change_count > 1)
				tpos->change_count = 1;
			else
				tpos->change_count = 0;
		} /* end list_for_each_entry_safe(... */
	} /* end while(... */
	return NULL;
}

/* 使用cJSON 组合成基带板设备的信息然后通过http发送至server */
static void *thread_baseband_status(void *arg)
{
	char *json_string = NULL;
	while (1) {
		sleep(args_info.banditv_arg);
		/* get && send LTE device information */
		json_string =
			get_lte_base_band_status(GET_DEFAULT_CONFIG |
				GET_STATUS_CONFIG, NULL);
		if (json_string) {
			printf("LTE device report str :%s\n", json_string);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.baseband_api_arg, json_string,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(json_string);
			json_string = NULL;
		}
#if 1
		/* get && send FiberHome LTE device information */
		json_string = get_all_fiber_lte_base_band_status();
		if (json_string) {
			printf("FiberHome LTE report str :%s\n", json_string);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.baseband_api_arg, json_string,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(json_string);
			json_string = NULL;
		}
		/* get && send GSM device information */
		json_string = get_gsm_base_band_status();
		if (json_string) {
			printf("GSM device report str is :%s\n", json_string);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.gsm_ind_api_arg, json_string,
				NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(json_string);
			json_string = NULL;
		}
#endif

		/* get && send WCDMA device information */
		/* Not use */
#if 0
		json_string = get_wcdma_base_band_status();
		if (json_string) {
			printf("GSM device report str is :%s\n", json_string);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.wcdma_dev_info_api_arg,
				json_string, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(json_string);
			json_string = NULL;
		}
#endif
	}
	return NULL;
}

/* 循环上报主控板状态信息 */
static void *thread_device_status(void *arg)
{
	char my_dev_info[256] = {0};
	char my_freq_type_str[128] = {0};
	int sleep_sec = (args_info.statitv_arg >= 30)?args_info.statitv_arg:30;
	while (1) {
		snprintf(my_dev_info, 256,
			"{\"mac\":\"%s\",\"topic\":\"%s\""
			",\"lng\":\"%02.10lf\""
			",\"lat\":\"%02.10lf\""
			",\"alt\":\"%02.2lf\"}",
			dev_mac_string, my_topic,
			my_longitude, my_latitude, my_altitude);
		pthread_mutex_lock(&http_send_lock);
		http_send(args_info.stat_api_arg, my_dev_info, NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
		snprintf(my_freq_type_str, 128,
			"{\"topic\":\"%s\",\"band_type\":\"%d\"}",
			my_topic, same_or_dif_state);

		pthread_mutex_lock(&http_send_lock);
		http_send(args_info.same_dif_fre_api_arg, my_freq_type_str,
			NULL, NULL);
		pthread_mutex_unlock(&http_send_lock);
		sleep(sleep_sec);
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

/* 初始化 mosquitto 配置 */
static void init_config(struct mosq_config *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->eol = true;
	cfg->max_inflight = 20;
	cfg->clean_session = true;
	cfg->keepalive = args_info.keepalive_arg;
	cfg->username = strdup(args_info.user_name_arg);
	cfg->password = strdup(args_info.user_passwd_arg);
	cfg->protocol_version = MQTT_PROTOCOL_V31;

	cfg->id = strdup(my_id);
	cfg->topic_count = 1;
	cfg->topics = calloc(1, sizeof(char *));
	cfg->topics[0] = strdup(my_topic);
	cfg->host = strdup(args_info.hostip_arg);
	if (args_info.hostport_arg > 65535) {
		fprintf(stderr, "Error: Invalid port given: %d\n", cfg->port);
		return;
	} else {
		cfg->port = args_info.hostport_arg;
	}
	if (args_info.debug_arg == 7) {
		cfg->debug = true;
	}
}

/*根据基带板功放配置文件控制基带板与功放的开关*/
void init_band_pa_state()
{
	char *errmsg;
	char **resultp;
	int n_row, n_cloum;
	int i, j;
	char sql_cmd[1024];

	if(sqlite3_open(args_info.band_pa_state_config_arg, &band_pa_state_db) != SQLITE_OK)
	{
		printf("open band and pa database err!\n");
		write_action_log("@nyb-Base Band and PA info:",
			"sql was open failed");
		return;
	}
	printf("open band and pa database success!\n");
	write_action_log("@nyb-Base Band and PA SQL info:",
		"sql was open successfully");
	/*  below info define(nyb,2018.03.31):
	 *  First:  Device cfg, 1 intral, 2 inter A, 3 inter B
	 *  Second: band0~band5 indicate six port voltage status
	 *  Third:  three PA status,0 disable, 1 enable
	 * */
	char *sql_create_cmd = "create table band_pa_state(\
                same_or_dif_state int  not null default 0,\
                band0_state       int  not null default 0,\
                band1_state       int  not null default 0,\
                band2_state       int  not null default 0,\
                band3_state       int  not null default 0,\
                band4_state       int  not null default 0,\
                band5_state       int  not null default 0,\
                pa0_state         int  not null default 0,\
                pa1_state         int  not null default 0,\
                pa2_state         int  not null default 0\
                );";

	//write_action_log("@nyb-Sql create cmd: ", sql_create_cmd);
	if(sqlite3_exec(band_pa_state_db, sql_create_cmd,  NULL,  NULL, &errmsg) ==  SQLITE_OK) {
		printf("sqlitr3 create table！\n");
		write_action_log("@nyb-sqlitr3 create table:", "sql was created successfully");
		pthread_mutex_lock(&set_band_pa_file_lock);

		//Default 1, work in intra frequence model, TDD work at B38/B39, FDD work at B3(unicom/telecom)
		same_or_dif_state = INTRA_WORK_MODEL;  //Default 1
		for(i = 0; i < BAND_NUM; i++) {//设置默认基带板与功放状态
			//we only have three PA, so here first process former PA
			if(i < PA_NUM) {
				state_band_pa.band_state[i] = GPIO_PORT_ENABLE;//0：关闭；1：打开
				state_band_pa.pa_state[i] = PA_ENABLE;
			} else {
				state_band_pa.band_state[i] = GPIO_PORT_ENABLE;
			}
		}
		//extend from four to six bands as we have six ports in our Power board
		sprintf(sql_cmd, "insert into band_pa_state values(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d);",
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

		if(sqlite3_exec(band_pa_state_db, sql_cmd,  NULL,  NULL, &errmsg) !=  SQLITE_OK) {
			printf("sqlite3 insert info error:%s\n", errmsg);
			write_action_log("@nyb-sqlite3 insert info error:", errmsg);
			sqlite3_free(errmsg);
			pthread_mutex_unlock(&set_band_pa_file_lock);
			return;
		}
		pthread_mutex_unlock(&set_band_pa_file_lock);
	} else {
		write_action_log("@nyb-sqlitr3 create table, err info:", errmsg);
		sqlite3_free(errmsg);
	}

	sprintf(sql_cmd, "select * from band_pa_state;");
	pthread_mutex_lock(&set_band_pa_file_lock);
	if (sqlite3_get_table(band_pa_state_db, sql_cmd, &resultp, &n_row, &n_cloum, &errmsg) != SQLITE_OK) {
		printf("sqlite3 get info err:%s\n", errmsg);
		write_action_log("@nyb-sqlite3 get info err: ", errmsg);
		sqlite3_free(errmsg);
		pthread_mutex_unlock(&set_band_pa_file_lock);
		return;
	}
	char **p;
	p = resultp + n_cloum;
	for (i = 0; i < n_cloum; i++)  {
		if(i == 0) {
			same_or_dif_state = atoi(p[i]);
			continue;
		}
		if(i <= BAND_NUM)
			state_band_pa.band_state[i-1] = atoi(p[i]);
		else
			state_band_pa.pa_state[i-BAND_NUM-1] = atoi(p[i]);
	}
#ifndef VMWORK
#ifdef SYS86
	for(i = 0;i < BAND_NUM; i++) {
		if(state_band_pa.band_state[i] == GPIO_PORT_DISABLE) {
			ctrl_pa_switch(i ,LEVEL_LOW);
		}
	}

	//Actually we only have three PA now, maybe will extend in future,2018.04.02
	for(i = 0; i < PA_NUM; i++) {
		if(state_band_pa.pa_state[i] == PA_DISABLE)
			send_serial_request(i, DATA_SEND_TYPE_SET_ANT_DOWN);
		else
			send_serial_request(i, DATA_SEND_TYPE_SET_ANT_UP);
	}
#endif
#endif
	sqlite3_free_table(resultp);
	pthread_mutex_unlock(&set_band_pa_file_lock);
}
/* 获取mac地址并初始化MQTT身份信息 */
void get_mac_and_init_about_config()
{
	/* 获取mac地址 */
	if (get_dev_mac(dev_mac, args_info.dev_iface_arg) == -1) {
		exit(0);
	}
	snprintf(dev_mac_string, 18, "%02X-%02X-%02X-%02X-%02X-%02X",
		dev_mac[0], dev_mac[1], dev_mac[2],
		dev_mac[3], dev_mac[4], dev_mac[5]);
	snprintf(my_topic, 30, "%s.%02X-%02X-%02X-%02X-%02X-%02X",
		args_info.sensorname_arg, dev_mac[0],dev_mac[1], dev_mac[2],
		dev_mac[3],dev_mac[4],dev_mac[5]);
	printf("My topic is :[%s]\n", my_topic);
	snprintf(my_id, 23, "mqttsub_%02x%02x%02x%02x%02x%02x",
		dev_mac[0], dev_mac[1], dev_mac[2],
		dev_mac[3], dev_mac[4], dev_mac[5]);
	printf("My mqttID is :[%s]\n", my_id);
}
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
/* =================循环读取Epoll==================*/
void *thread_looprecv(void *arg)
{
	struct epoll_event *ev = malloc(sizeof(struct epoll_event) * MAXWAIT);
	if(!ev) {
		printf("malloc epoll act failed\n");
		exit(-1);
	}
	struct cliinfo *cli;
	int nfds, i, ret;
	U8 rec_buffer[MAX_RECEV_LENGTH] = {0};
	while(1) {
		nfds = Epoll_wait(ev);
		for(i = 0; i < nfds; i++) {
			if (ev[i].data.fd == STDIN_FILENO)
				continue;
			cli = (struct cliinfo *)(ev[i].data.ptr);
			rec_buffer[0] = 0;
			ret = recv(cli->fd, rec_buffer, MAX_RECEV_LENGTH, 0);
			if (ret <= 0)
				continue;
			band_entry_t *entry = band_list_entry_search(cli->addr);
			if (!entry)
				continue;
			if (entry->work_mode == FIBERHOME_BASE_BAND_WMODE) {
				/* FiberHome */
				handle_recev_fiber_package(rec_buffer, entry);
			} else if (entry->work_mode == WCDMA_BASE_DEVICE) {
				/* WCDMA */
				handle_package_wcdma(rec_buffer, cli);
			} else {
				/* BaiCells*/
				//	handle_recev_package(rec_buffer, entry);
			}
		} /* end for(i=0... */
	} /* end while(1)... */
	return NULL;
}

void thread_read_send_station_json_object()
{
	/* Baicell */
	safe_pthread_create(thread_read_send_baicell_json_object, NULL);
	/* FiberHome */
	safe_pthread_create(thread_read_send_fiberhome_json_object, NULL);
	/* WCDMA */
	safe_pthread_create(thread_read_send_wcdma_json_object, NULL);
	/* GSM */
	safe_pthread_create(read_send_gsm_json_object, NULL);
}

int main(int argc, char *argv[])
{
	proc_args(argc, argv); //初始化配置参数
	one_instance(); // 确保单一进程
	get_mac_and_init_about_config(); //获取mac地址并初始化topic
#ifdef AUTH
	if (!auth()) {
		printf("Sorry, this device is unauthorized!\n");
		exit(1);
	}
	printf("This device is authorized!\n");
#endif
	/* 是否开启打印 */
	if (args_info.debug_arg <= 4) {
		daemon(0, 0);
	}
	/* 初始化http 发送锁 */
	if (pthread_mutex_init(&http_send_lock, NULL) != 0) {
		printf("init http_send_lock error!\n");
		return 0;
	}
	timer_init(); // init timer
	/* init UDP client to send imsi infor and send UE info */
	init_udp_send_imsi_cli(args_info.uhost_arg, args_info.uport_arg);
	init_ue_udp_send_cli(args_info.uehost_arg, args_info.ueport_arg);
	band_list_head_init(); //init baseband list head
	/* create directory */
	create_directory(args_info.def_basepath_arg); //保存动态默认配置
	create_directory(args_info.bsavepath_arg); //保存基带板信息
	create_directory(SCAN_DIR); //保存扫频信息
	init_action_log();//初始化日志文件
#ifdef SAVESQL
	create_directory(args_info.dbpath_arg); //保存imsi的数据库
	init_dbfile_save_imsi(); //init db file:all.db
	safe_pthread_create(thread_check_dbdirectory_file, NULL);//初始化数据库(按时间)
#endif
#ifdef SCACHE
    	init_cutnet_config();//初始化断网配置文件
	cache_filelock_init(); // init lock that check cache file
	check_cache_file(); // check cache file
	/* 初始化http服务器的链表 */
	init_http_send_client_server_list();
#endif
#ifndef VMWORK
#ifdef SYS86
	init_serial_gpio_switch(); // init serial and GPIO
	init_band_pa_state(); // get base band and power amplifier status
#endif
#endif
	Epoll_init(); // init Epoll select
	/* creat pthread */
	safe_pthread_create(thread_looprecv, NULL);//接收Epoll监管信息
	safe_pthread_create(thread_device_status, NULL);//发送主控板状态
	safe_pthread_create(thread_socket_to_baicell, NULL);//与Baicell基带板交互
	safe_pthread_create(thread_tcp_to_baseband_wcdma, NULL);//与WCDMA基带板交互
	safe_pthread_create(thread_tcp_to_fiberbaseband_lte, NULL);//与烽火LTE基带板交互
	safe_pthread_create(thread_udp_to_gsm, NULL); //与GSM交互
	safe_pthread_create(thread_recv_wifi_and_send, NULL);//接收并组合wifi采集信息
#ifndef VMWORK
#ifdef SYS86
	safe_pthread_create(thread_pa_status, NULL); //获取功放信息状态并发送
#endif
#endif
	safe_pthread_create(thread_scan_frequency, NULL); //每过一段时间，自动扫频并上报
	safe_pthread_create(thread_pare_imsi_action_list, NULL);//解析baicell IMSI链表
	safe_pthread_create(thread_change_baseband_state, NULL);//监听基带板状态
	safe_pthread_create(thread_baseband_status, NULL);//发送基带板状态
#ifdef SCACHE
	safe_pthread_create(thread_checknet_readfile, NULL);//检查网络读取缓存
#endif
#ifdef CLI
	safe_pthread_create(thread_usocket_with_CLI, NULL);//与cli进行交互
#endif
	thread_read_send_station_json_object();//发送采集数据(baicell need add wcdma and gsm and fiber)

	struct mosq_config cfg; // mosquitto config 
	init_config(&cfg); // init mosquitto config
start_mosqutto:
	sleep(1);
	mosquitto_lib_init();
	mosq = mosquitto_new(cfg.id, cfg.clean_session, &cfg);
	if(!mosq) {
		printf("New mosq Error.\n");
		goto error_restart;
	}
	if (client_opts_set(mosq, &cfg)) {
		goto error_restart;
	}
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);

	int rc = client_connect(mosq, &cfg);
	if (rc) goto error_restart;
	mosquitto_loop_forever(mosq, -1, 1);
	/* if error and return from mosquitto_loop_forever */
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	goto start_mosqutto;
error_restart:
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	goto start_mosqutto;
	return 1;
}
