/*
 * ============================================================================
 *
 *       Filename:  tcp_wcdma.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年09月25日 09时13分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  weichao lu (Chao), lwc_yx@163.com
 *   Organization:  
 *
 * ============================================================================
 */

#ifndef __TCP_WCDMA_H__
#define __TCP_WCDMA_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hash_list_active.h"
#include "cJSON.h"
#include "cache_file.h"
#include "http.h"
#include "cmdline.h"
#include "arg.h"
#include "epoll_active.h"
#include "mosquit_sub.h"
#include "config.h"
#ifdef SAVESQL
#include "save2sqlite.h"
#endif
#include "baicell_net.h"
#define WCDMA_TYPE_LEN       2
#define WCDMA_NUM_LEN        2

#define WCDMA_MSG_LEN        2

#define WCDMA_TYPE_FFFF      4

#define WCDMA_WORK_MODE      4      //代表WCDMA的工作模式

/*消息类型宏*/
#define WCDMA_RF             "00"
#define WCDMA_INFO           "03"
#define WCDMA_VERSION        "08"
#define WCDMA_RUN_PARAMETER  "09"
#define WCDMA_MODE           "06"

#define WCDMA_GET_VERSION_CMD  "FFFF#4#0804"
#define WCDMA_UPGRADE_CMD      "FFFF#4#0815"

/*上行消息代码宏*/
/*消息类型00*/
#define WCDMA_START_RF_RESPONSE      "02"
#define WCDMA_STOP_RF_RESPONSE       "05"

/*消息类型03*/
#define WCDMA_WORK_INFO_RESPONSE     "02"
#define WCDMA_SET_POWER_RESPONSE     "13"
#define WCDMA_WORK_STATE             "09"
#define WCDMA_RESTART_RESPONSE       "23"
#define WCDMA_USER_INFO              "24"

#define WCDMA_SET_IP_RESPONSE        "21"

#define WCDMA_GET_TIME               "44"
#define WCDMA_SET_TIME_RES           "19"

/*消息类型08*/
#define WCDMA_VERSION_RESPONSE       "05"
#define WCDMA_FTP_NUM_RESPONSE       "13"
#define WCDMA_FTP_DOWNLOAD_RESPONSE  "14"
#define WCDMA_FTP_UPGRADE_RESPONSE   "16"
#define WCDMA_GET_DAC_RESPONSE       "09"
#define WCDMA_SET_FTP_INFO_RESPONSE  "22"

#define WCDMA_SET_FRE_RATIO          "01"
#define WCDMA_SET_LAC_RAC_TIME_RES   "03"
#define WCDMA_SET_DAC_OFFSET_RES     "07"
#define WCDMA_SET_PA_POWER_RES       "18"
#define WCDMA_STOP_LAC_RAC_TIME_RES  "11"

/*消息类型09*/
#define WCDMA_DEV_INFO_RESPONSE      "02"

/*消息类型06*/
#define WCDMA_SET_2G_INFO_RESPONSE   "15"
#define WCDMA_SET_TO_2G_RESPONSE     "17"

//Just WCDMA msg type define(0xCxxx)
#define MQTT_WCDMA_SET_IPADDR 		  (0xC01B) /* 6.10 设置基带板的IP地址 */
#define MQTT_WCDMA_SET_RATIO_OFPILOT_SIG  (0xC080) /* 6.15 设置导频比率 -*/
#define MQTT_WCDMA_SET_TIME_OF_LAC_RAC    (0xC081) /* 6.17 设置LAC/RAC更新时间 */
#define MQTT_WCDMA_GET_STATUS_OF_BASE     (0xC082) /* 6.19 获取设备运行参数 */
#define MQTT_WCDMA_GET_DAC          (0xC083) /* 6.23 查询DAC */
#define MQTT_WCDMA_SET_DAC          (0xC084) /* 6.25 设置DAC (使用初始频偏设置)*/
/* 6.27 当前运行的PSC/LAC/RAC/CELLID值和驱赶模式值 */
#define BASE_WCDMA_STATUS_OF_REJ    (0xC085)
#define MQTT_WCDMA_STOP_SET_TIME_OF_LAC_RAC (0xC086) /* 6.28 停止自更改LAC/RAC值*/
#define MQTT_WCDMA_SET_DRIVE_TO_2G_INFO     (0xC087) /* 6.33 设置驱赶到2G的信息*/
#define MQTT_WCDMA_SET_DRIVE_MOD    (0xC088) /* 6.35 设置基站驱赶模式 */
#define MQTT_WCDMA_SET_FTP_DL_INFO  (0xC089) /* 6.37 设置ftp下载文件参数*/
#define MQTT_WCDMA_SET_PA_POWER     (0xC08A) /* 6.42 设置基站功放功率值 */
#define MQTT_WCDMA_SYNC_TIME        (0xC08B) /* 6.44 请求控制台同步时间 */
#define MQTT_WCDMA_SET_FTP_UL_INFO  (0xC08C) /* 6.45 设置基站文件上传ftp相关信息*/
#define MQTT_WCDMA_SET_WORK_PARAM   (0xC08D) /* 6.1 设定特种基站工作参数请求 */

#define MQTT_WCDMA_TURN_ONOFF (0xF00D) /* 激活与去激活小区 */
#define MQTT_WCDMA_SET_TXPOWER  (0xF015) /* 设置发射功率 */
#define MQTT_WCDMA_SET_SYS_TINE (0xF01F) /* 设置系统时间 */
#define MQTT_WCDMA_REBOOT_DEV (0xF00B) /* 重启WCDMA设备 */

#pragma pack(1)
typedef struct wcdma_header {
	char wcdma_type[3];
	char wcdma_num[3];
} wcdma_header_t;

typedef struct wcdma_user_info {
	char *imsi;
	char *imei;
	char *domains;
} wcdma_user_info_t;
#pragma pack()


void handle_wcdma_rf_res(char *wcdma_buf, int msg_len, struct cliinfo *info);
void handle_wcdma_version(char *wcdma_buf, int msg_len, struct cliinfo *info);
void handle_recv_wcdma(char *wcdma_buf, int buf_len, struct cliinfo *info);
void handle_wcdma_info(char *wcdma_buf, int msg_len, struct cliinfo *info);
void handle_wcdma_user_info(char *msg, int msg_len, struct cliinfo *info);
void handle_work_state(char *msg, int msg_len, struct cliinfo *info);
void handle_wcdma_run_parameter(char *wcdma_buf, int msg_len, struct cliinfo *info);
void handle_wcdma_dev_info(char *msg, int msg_len, struct cliinfo *info);
void handle_wcdma_mode(char *wcdma_buf, int msg_len);
void *thread_tcp_to_baseband_wcdma(void *arg);
void *thread_read_send_wcdma_json_object(void *arg);
void wcdma_pare_config_json_message(S8 *mqtt_msg, band_entry_t *entry);
void handle_package_wcdma(U8 *recBuf, struct cliinfo *info); 

#endif
