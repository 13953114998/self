/*
 * =====================================================================================
 *
 *       Filename:  mosquit_sub.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年04月22日 16时50分14秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __MOSQUIT_SUB_H__
#define __MOSQUIT_SUB_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>

#define FAULT_CLIENT_SOCKET_FD (-1)

#define O_CHANGE_SERVER_IP_CFG 			(0xE002)

//ANT Config Command type
#define O_SERVER_TO_BASE_MASTER_ANT_CONFIG      (0xE003)
/* {"msgtype":"0xE003", "ip":"192.168.2.36", "command":"0"}
 * command:
 * 0  查询功放状态信息
 * 1  打开功放开关
 * 2  关闭功放开关
 * */

//设置同频异频
#define O_SERVER_TO_BAND_AND_PA_CMD        (0xE004)
/*{"msgtype":"0xE004","command":"0"}
command:
0：查询同频异频状态
1：同频
2：异频A
3: 异频B*/

/* 设置主控板升级或者运行脚本等指令请求 */
#define O_SERVER_TO_BAND_UPDATE_FILE        (0xE005)
#if 0
{
 "msgtype" : "0xE005",
  "f_name" : "base_master_1.0.0-1_i386_pentium4.ipk",
   "f_md5" : "790a5522726b0ec6c53697bcc7eaaa11",
   "uname" : "dm",
  "passwd" : "admin",
   "d_url" : "ftp://58.59.6.130",
  "f_type" : "0",
  "d_type" : "1"
}
--------------------------
f_type: 下载文件的类型
0：ipk包安装文件(使用opkg install 安装),     file_name 后缀名: .ipk
1：img/bin固件文件(使用 sysupgrade 升级),    file_name 后缀名: .img/.bin
2：压缩文件(使用tar -zxvf解压到根('/')目录), file_name 后缀名: .tar.gz
3：shell脚本文件(添加可执行权限并直接运行),  file_name 后缀名: .sh
--------------------------
d_type:下载链接的协议类型
0：http协议
1：ftp协议
--------------------------
#endif
/* 设置不发送采集信息的服务器IP */
#define O_SET_NOT_SEND_CLIENT_IP   (0xE006)
/* 后台控制主控板使用GPIO对基带板进行硬件重启 */
#define O_SERVER_TO_BASE_MASTER_GPIO_REBOOT_BAND_CMD  (0XF099)
#if 0
{"msgtype":"0xE006", "list":["58.59.6.130", "60.240.15.92"]}
#endif
//!todo: below four Macro defination have been replaced by enum
#define GET_SAME_OR_DIF_STATE 0//控制同频异频命令 0：获取状态；1：同频；2：异频
#define SET_STATE_SAME 1
#define SET_STATE_DIF_A 2
#define SET_STATE_DIF_B 3

#define BAND_NUM    (6)//基带板数量
#define PA_NUM      (3)//功放的数量

pthread_mutex_t set_band_pa_file_lock;//操作基带板功放状态配置文件锁
FILE *band_and_pa_state_fd; // 基带板功放状态配置文件
typedef struct band_pa_state{//基带板功放状态结构体
	int band_state[BAND_NUM];
	int pa_state[PA_NUM];
}band_pa_state_t;

band_pa_state_t state_band_pa;
/*同频异频信息*/
sqlite3 *band_pa_state_db;//基带板功放状态配置数据库

int same_or_dif_state; //同频异频状态

#define SCAN_DIR "/mnt/sd/scan_info/"
#define SCAN_LTE_FILE "/mnt/sd/scan_info/lte_scan"
#define SCAN_GSM_FILE "/mnt/sd/scan_info/gsm_scan"


struct mosq_config {
	char *id;
	char *id_prefix;
	int protocol_version;
	int keepalive;
	char *host;
	int port;
	int qos;
	bool retain;
	int pub_mode; /* pub */
	char *file_input; /* pub */
	char *message; /* pub */
	long msglen; /* pub */
	char *topic; /* pub */
	char *bind_address;
	bool debug;
	bool quiet;
	unsigned int max_inflight;
	char *username;
	char *password;
	char *will_topic;
	char *will_payload;
	long will_payloadlen;
	int will_qos;
	bool will_retain;
	bool clean_session; /* sub */
	char **topics; /* sub */
	int topic_count; /* sub */
	bool no_retain; /* sub */
	char **filter_outs; /* sub */
	int filter_out_count; /* sub */
	bool verbose; /* sub */
	bool eol; /* sub */
	int msg_count; /* sub */
};

void client_config_cleanup(struct mosq_config *cfg);
int client_opts_set(struct mosquitto *mosq, struct mosq_config *cfg);
int client_connect(struct mosquitto *mosq, struct mosq_config *cfg);

/* mosquitto connect callback function */
void my_connect_callback(struct mosquitto *mosq, void *obj, int result);
/* mosquitto receive message callback function */
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
void pare_config_json_message(char *my_msg, int cli_sfd);

#endif /* __MOSQUIT_SUB_H__ */
