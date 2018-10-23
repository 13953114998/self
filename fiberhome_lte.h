/*
 * =====================================================================================
 *
 *       Filename:  fiberhome_lte.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年02月27日 14时15分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __FIBERHOME_LTE_H__
#define __FIBERHOME_LTE_H__
#include "config.h"
#include "epoll_active.h"
#include "hash_list_active.h"
#include "cJSON.h"
/* fiberhome base_band work_mode */
#define FIBERHOME_BASE_BAND_WMODE (5)

/* 烽火基站配置报文类型 */
/* 采集配置请求/回应 */
#define TOBASE_PROBE_CONFIG_REQUEST (301)
#define FROMBASE_PROBE_CONFIG_RESPONSE (302)
/* 设备运行状态上报 */
#define FROMBASE_DEVICE_STATUS_REPORT (303)
/* IMSI信息上报 */
#define FROMBASE_IMSI_REPORT (304)
/* 功率档位设置请求/回应 */
#define TOBASE_TXPOWER_CONFIG_REQUEST (305)
#define FROMBASE_TXPOWER_CONFIG_RESPONSE (306)
/* 获取采集信息请求/回应 */
#define TOBASE_GET_PROBE_CONDIF_REQUEST (307)
#define FROMBASE_GET_PROBE_CONDIF_RESPONSE (308)
/* 设置基站IP请求/回应 */
#define TOBASE_DEVICE_IP_CONFIG_REQUEST (309)
#define FROMBASE_DEVICE_IP_CONFIG_RESPONSE (309)
/* 复位基站请求/回应 */
#define TOBASE_RESET_DEVICE_REQUEST (311)
#define FROMBASE_RESET_DEVICE_RESPONSE (312)
/* 设置基站时间请求/回应 */
#define TOBASE_TIME_CONFIG_REQUEST (313)
#define FROMBASE_TIME_CONFIG_RESPONSE (314)
/* 设备信息查询请求/回应 */
#define TOBASE_GET_DEVICE_INFO_REQUEST (315)
#define FROMBASE_GET_DEVICE_INFO_RESPONSE (316)
/* 基站升级操作请求/回应/结束指示/升级进度指示 */
#define TOBASE_VERSION_UPDATE_REQUEST (317)
#define FROMBASE_VERSION_UPDATE_RESPONSE (318)
#define FROMBASE_VERSION_UPDATE_FINISH_REPORT (319)
#define FROMBASE_VERSION_UPDATE_RATE_REPORT (320)
/* 基站位置查询请求/回应 */
#define TOBASE_GET_DEVICE_SITE_REQUEST (321)
#define FROMBASE_GET_DEVICE_SITE_RESPONSE (322)
/* IMSI 数据上传请求/相应/结束指示 */
#define TOBASE_GET_IMSI_INFO_REQUEST (323)
#define FROMBASE_GET_IMSI_INFO_RESPONSE (324)
#define FROMBASE_IMSI_UP_FINISH_REPORT (325)
/* 设备信息上报 */
#define FROMBASE_DEVICE_INFOMATION_REPORT (326)
/* 设备心跳 */
#define FROMBASE_HARTBEAT_REPORT (327)
/* 基站时间查询请求/回应 */
#define TOBASE_GET_TIME_REQUEST (328)
#define FROMBASE_GET_TIME_RESPONSE (329)
/* 基站IP查询请求/回应 */
#define TOBASE_GET_IP_REQUEST (330)
#define FROMBASE_GET_IP_RESPONSE (331)
#if 1
/* 文件下载请求/响应 */
#define TOBASE_DOWNLOADFILE_REQUEST (332)
#define FROMBASE_DOWNLOADFILE_RESPONSE (333)
/* 文件上传请求/响应 */
#define TOBASE_UPLOADFILE_REQUEST (334)
#define FROMBASE_UPLOADFILE_RESPONSE (335)
#define FROM_REPORT_FILE_FINISH_REPORT (336)
/* 空口同步测量请求/回应/结果上报 */
#define TOBASE_AIR_SYNC_GAUGE_CONFIG_REQUEST (337)
#define FROMBASE_AIR_SYNC_GAUGE_CONFIG_REQUEST (338)
#define FROMBASE_AIR_SYNC_GAUGE_RES_REPORT (339)
/* 时偏值配置请求/回应 */
#define TOBASE_AIR_SYNC_TIME_OFFSET_CONFIG_REQUEST (340)
#define FROMBASE_AIR_SYNC_TIME_OFFSET_CONFIG_REQUEST (341)
/* 时偏值查询请求/回应 */
#define TOBASE_GET_AIR_SYNC_TIME_OFFSET_REQUEST (342)
#define FROMBASE_GET_AIR_SYNC_TIME_OFFSET_REQUEST (343)
#endif
/* 告警上报消息 */
#define FROMBASE_WARING_REPORT (0x0809)
#define TOBASE_ACTIVE_WARING_REUEST (0x080A)
#define FROMBASE_ACTIVE_WARING_RESPONSE (0x080B)

/* 3.7.2 小区运行模式 */
#define TOBASE_CELL_ACTIVE_MODE_CONFIG_REQUEST (401)
#define FROMBASE_CELL_ACTIVE_MODE_CONFIG_RESPONSE (402)
/* 3.7.3 小区运行模式查询 */
#define TOBASE_CELL_ACTIVE_MODE_QUERY_REQUEST (403)
#define FROMBASE_CELL_ACTIVE_MODE_QUERY_RESPONSE (404)
/* 3.7.5 REM扫描配置 */
#define TOBASE_REM_SCAN_CONFIG_REQUEST (405)
#define FROMBASE_REM_SCAN_CONFIG_RESPONSE (406)
/* 3.7.6 REM扫描查询 */
#define TOBASE_REM_SCAN_QUERY_REQUEST (407)
#define FROMBASE_REM_SCAN_QUERY_RESPONSE (408)
/* 2.7.8 状态上报扩展部分*/
#define FROMBASE_DEVICE_STATUS_REPORT_ADDED (409)
/* 3.7.11 IMSI黑白名单配置 */
#define TOBASE_IMSI_CONTROL_LIST_CONFIG_REQUEST (410)
#define FROMBASE_IMSI_CONTROL_LIST_CONFIG_RESPONSE (411)
/* 3.7.12 IMSI黑白名单查询 */
#define TOBASE_IMSI_CONTROL_LIST_QUERY_REQUEST (412)
#define FROMBASE_IMSI_CONTROL_LIST_QUERY_RESPONSE (413)
/* 3.7.15 邻区扫描结果查询 */
#define TOBASE_ADAREA_SCAN_QUERY_REQUEST (414)
#define FROMBASE_ADAREA_SCAN_QUERY_RESPONSE (415)
/* 3.7.26 采集开关 */
#define TOBASE_PROBE_SWITCH_CONFIG_REQUEST (416)
#define FROMBASE_PROBE_SWITCH_CONFIG_REQUEST (417)
/* 3.7.27 定时重启 */
#define TOBASE_TIMING_REBOOT_CONFIG_REQUEST (418)
#define FROMBASE_TIMING_REBOOT_CONFIG_REQUEST (419)
/* 3.7.28 一键删除邻区列表 */
#define TOBASE_DELETE_ADAREA_SCAN_LIST_REQUEST (420)
#define FROMBASE_DELETE_ADAREA_SCAN_LIST_RESPONSE (421)
/* 3.7.29 邻区表查询 */
#define TOBASE_ADAREA_LIST_QUERY_REQUEST (422)
#define FROMBASE_ADAREA_LIST_QUERY_RESPONSE (423)
/* 3.7.30 频率表查询 */
#define TOBASE_FREQUENCY_LIST_QUERY_REQUEST (424)
#define FROMBASE_FREQUENCY_LIST_QUERY_RESPONSE (425)
/* 3.7.31  APK版本升级 */

/* 烽火基站报文 */
#pragma pack(1)
/* Header */
typedef struct fiberhome_package_header {
	U32 length;  /* 消息长度 */
	U16 msgtype; /* 消息类型 */
	U16 transid; /* 消息流水号 */
	U8 version;  /* 接口版本 22 */
	U8 reserv1;  /* 保留字段 */
	U16 reserv2;  /* 保留字段 */
} _fiberhome_header;
/* 通用配置请求*/
typedef struct fiberhome_generic_config_request {
	_fiberhome_header header; /* 消息头 */
	S8 device_id[21];         /* 设备ID */
} _fiber_genneric_set_req;
/* 通用配置回应*/
typedef struct fiberhome_generic_config_response {
	_fiberhome_header header; /* 消息头 */
	S8 device_id[21];         /* 设备ID */
	U16 result; /* 结果:0-成功,1-失败 */
	U16 cause;  /* 错误代码 如果result == 0,此为0 */
} _fiber_genneric_set_res;
/* 通用信息指示 */
typedef _fiber_genneric_set_req _fiber_genneric_inf_rep;

/* 2.8空口同步相关 */
/* 2.8.1 空口同步测量请求*/
typedef struct fiberhome_air_sync_gauge_request {
	_fiber_genneric_set_req req;
	U32 meas_freq_band; /* TDD频带 38,39,40,41...*/
	U16 meas_freq; /* 公网 4G 工作频点,取值范围如下. 
			* 当 FreqBandIndicator 为 38 时,(37750~38249)
			* 当 FreqBandIndicator 为 39 时,(38250~38599)
			* 当 FreqBandIndicator 为 40 是,(38650~39349)
			* 当 FreqBandIndicator 为 41 是,(39650~41589)
			* */
} _fiber_airsync_gauge_req;
/* 2.8.3 空口同步测量结果上报 */
typedef struct fiberhome_air_sync_gauge_result_report {
	_fiber_genneric_inf_rep rep;
	U32 meas_freq_band; /* TDD频带 38,39,40,41...*/
	U16 meas_freq; /* 公网 4G 工作频点,取值范围如下. 
			* 当 FreqBandIndicator 为 38 时,(37750~38249)
			* 当 FreqBandIndicator 为 39 时,(38250~38599)
			* 当 FreqBandIndicator 为 40 是,(38650~39349)
			* 当 FreqBandIndicator 为 41 是,(39650~41589)
			* */
	U16 meas_result; /* 该频段的空口同步测量结果。取值范围:
			  * 0--不测量.
			  * 1--待测量,该频段需要进行测量,尚未进行.
			  * 2--测量完成,测量值见 meas_time_offset
			  * 3--测量失败.
			  * */
	S8 meas_time_offset[16]; /* 公网小区的时偏值.
				  * 取值范围: -5000000~5000000,单位: ns.
				  * 字符串类型,字符串长度为 16 字节.
				  * 内容格式示例 1: "-12345" 
				  * 内容格式示例 2: "65433"
				  * */
} _fiber_airsync_gauge_res_report;
/* 2.8.4 时偏值配置请求 */
typedef struct fiberhome_air_sync_time_offset_config_request {
	_fiber_genneric_set_req req;
	U32 meas_freq_band; /* TDD频带 38,39,40,41...*/
	S8 meas_time_offset[16]; /* 公网小区的时偏值.
				  * 取值范围: -5000000~5000000,单位: ns.
				  * 字符串类型,字符串长度为 16 字节.
				  * 内容格式示例 1: "-12345" 
				  * 内容格式示例 2: "65433"
				  * */
} _fiber_airsync_timeoffset_req;
/* 2.8.7 时偏值查询响应 */
typedef struct fiberhome_air_sync_time_offset_get_response {
	_fiber_genneric_set_req res;
	U32 meas_freq_band; /* TDD频带 38,39,40,41...*/
	S8 meas_time_offset[16]; /* 公网小区的时偏值.
				  * 取值范围: -5000000~5000000,单位: ns.
				  * 字符串类型,字符串长度为 16 字节.
				  * 内容格式示例 1: "-12345" 
				  * 内容格式示例 2: "65433"
				  * */
} _fiber_airsync_timeoffseet_get_res;
/* 2.9.1 告警信息 */
typedef struct fiberhome_warning_report {
	_fiber_genneric_inf_rep rep;
	U32 alarm_id; /* 告警编号 */
	U32 alarm_restore_flag; /* 告警恢复标志:0-清除 1-产生 */
	S8 param[100];/* 告警参数 */
} _fiber_warning_report;
/* 2.9.2 活跃告警激活请求 */
/* 2.9.3 活跃告警激活响应 generic*/


/* ==============================New word=================================== */
/* 3.7.2.1小区运行模式配置请求 */
typedef struct fiberhome_cell_active_mode_config_request {
	_fiber_genneric_set_req req;
	U8 cmd_type;/* 小区建站模式:0-自动建站,1-手动建站 */
	U8 trans_type; /* 是否开启透传模式,0-关闭,1-开启 */
	U8 reboot; /* 0:无操作;1:设备重启 */
	U8 powersave; /* 电源模式 0:正常模式;1:休眠模式 */
} _fiber_cell_act_cfg_req;
/* 3.7.2.2小区运行模式配置回应 generic res */
/* 3.7.3.1 小区运行模式查询回复 generic req*/
/* 3.7.3.2 小区运行模式查询回复 */
typedef struct fiberhome_cell_active_mode_query_response {
	_fiber_genneric_set_res res;
	U8 cmd_type;/* 小区建站模式:0-自动建站,1-手动建站 */
	U8 trans_type; /* 是否开启透传模式,0-关闭,1-开启 */
	U8 powersave; /* 电源模式 0:正常模式;1:休眠模式 */
} _fiber_cell_act_query_res;
/* 3.7.4.1 建站配置请求 */
typedef struct fiberhome_probe_config_request {
	_fiber_genneric_set_req req;
	U8 cmd_type; /* 命令类型 1-开启 2-关闭 3-重启*/
	U16 oper; /* 运营商类型 0:移动,1:联通, 2:电信*/
	U32 preten_cellid; /* 伪装小区ID */
	U16 workfcn; /* 下行频点 */
	U32 plmn; /* 运营商网路ID */
	U32 pci; /* 物理小区表示 */
	U32 band; /* 工作频带 */
	U32 tac; /* 区域跟踪码 */
	U32 powerrank; /* 发射功率 */
	U32 collectperiod; /* 采集周期 */
} _fiber_probe_req;
/* 3.7.4 建站配置 回应 generic res */
/* 3.7.5.1 REM描配置 请求 */
typedef struct fiberhome_rem_scan_config_request {
	_fiber_genneric_set_req req;
	U8 cmd_type;/* 命令类型,1-开启扫描,2-关闭扫描.
		       自动建站模式下,此值必须为1 */
	U8 mode;/* 0-快速扫描模式 1-常规扫描模式. */
	S8 scanfcn[64];/* 小区扫描频点 */
	S8 plmn[64];/* 运营商网络 */
	U32 band;/* 未使用 */
	U32 thrsrp;/* 邻区扫描门限 */
} _fiber_rem_scan_cfg_req;
/* 3.7.5.2 REM描配置回复 generic res */
/* 3.7.6.1 REM扫描配置查询请求 generic req */
/* 3.7.6.2 REM扫描配置查询回应 */
typedef struct fiberhome_rem_scan_query_response {
	_fiber_genneric_set_res res;
	U8 cmd_type;/* 小区建站模式:0-自动建站,1-手动建站 */
	U8 mode; /* 0:快速扫描模式  1:常规扫描模式.
		  * 快速扫描意味着只获取PCI,频点信息,
		  * 不进行SIB1,SIB3,SIB5的解析.
		  * */
	S8 scanfcn[64]; /* 小区的扫描频点,可多个:eg.39148,38950,38400 */
	S8 plmn[64]; /* 运营商ID:移动=46000,联通=46001,电信=46011 */
	U32 band; /* 预留 */
	U32 thrrsrp; /* 邻区扫描门限,当邻区信号小于此值,不上报结果.单位dbm */
} _fiber_rem_scan_query_res;
/* 3.7.7 状态上报消息 */
typedef struct fiberhome_device_status_report {
	_fiber_genneric_inf_rep rep;
	U8 status;/* 设备状态
		     1-running 小区建立成功，正在采集 IMSI
		     2-Ready运行中,设备就绪,等待后台下发配置
		     3-Not Ready,设备未就绪,设备未启动,或启动未完成
		     4-Abnormal,设备状态异常,不能正常工作,需复位
		     5-Sync Measure,空口同步状态 */
	U8 cpu_rate; /* 设备 CPU 占用率,范围: 0~100 */
	U8 mem_rate; /* 设备内存使用率,范围: 0~100 */
} _fiber_dev_status_report;
/* 3.7.8 态上报扩展部分 */
typedef struct fiberhome_dev_status_added_report {
	_fiber_genneric_inf_rep rep;
	U16 battle; /* 电池电量 */
	U8 _sync; /* 同步状态,0:同步失败;1:空口同步成功;2:GPS同步成功 */
} _fiber_dev_status_add_rep;
/* 3.7.9 心跳检测消息 generic */
/* 3.7.10 IMSI上报 */
typedef struct fiberhome_imsi_info {
	_fiber_genneric_inf_rep rep;
	U32 num; /* MAX36 */
	U64 imsi[36];
}_fiber_imsi_rep;
/* 3.7.11.1 IMSI黑白名单配置请求 */
typedef struct imsi_control_list_config_request {
	_fiber_genneric_set_req req;
	U32 num;/* 数量 */
	U8 type; /* 0-无限制;1-白名单模式;2-黑名单模式 */
	U64 imsi[32]; /* IMSI数组 */
} _fiber_control_list_cfg_req;
/* 3.7.11.2 IMSI黑白名单配置回应 generic res */
/* 3.7.12.1 IMSI黑白名单查询请求 generic req */
/* 3.7.12.2 IMSI黑白名单查询回应 */
typedef struct imsi_control_list_query_response{
	_fiber_genneric_set_res res;
	U32 num;/* 数量 */
	U64 imsi[32]; /* IMSI数组 */
} _fiber_control_list_query_res;
/* 3.7.13.1 功率档位配置消息请求 */
typedef struct fiberhome_txpower_config_request {
	_fiber_genneric_set_req req;
	U32 powerrank; /* 功率档位,范围为1~4档,1档功率最小,4档最大.*/
} _fiber_txpower_set_req;
/* 3.7.13.2 功率档位配置消息回应 generic res */
/* 3.7.14.1 获取建站采集参数信息请求 generic req */
/* 3.7.14.2 获取建站采集参数信息回应 */
typedef struct fiberhome_get_probe_config_response{
	_fiber_genneric_set_res res;
	U8 cmd_type; /* 命令类型 1-开启 2-关闭 3-重启*/
	U16 oper; /* 运营商类型 0:移动,1:联通, 2:电信*/
	U32 preten_cellid; /* 伪装小区ID */
	U16 workfcn; /* 下行频点 */
	U32 plmn; /* 运营商网路ID */
	U32 pci; /* 物理小区表示 */
	U32 band; /* 工作频带 */
	U32 tac; /* 区域跟踪码 */
	U32 powerrank; /* 发射功率 */
	U32 collectperiod; /* 采集周期 */
} _fiber_getprobe_res;
/* 3.7.15 邻区扫描结果查询 no more support */
/* 3.7.16 设置基站IP No active */
/* 3.7.17.1 基站IP查询请求 generic req */
/* 3.7.17.2 基带板IP查询回应 */
typedef struct fiberhome_device_ipaddr_query_response {
	_fiber_genneric_set_res res;
	U8 ip_ver; /* IP类型,0-IPv4, 1-IPv6(不支持) */
	U8 ipaddr[16];/* IP地址(非字符串,不含点) */
	U8 net_mask; /* 子网掩码(数字类型eg:24) */
	U8 gateway[16]; /* IP网关(类似ipaddr) */
	U8 mbapk_ipaddr[16];/* 后台Mobile APK的IP地址(类似ipaddr) */
	U16 mbapk_port;/* 后台Mobile APK的端口号 */
	U16 ftp_ser_port; /* FTP server端口号:缺省值-21 */
} _fiber_ipaddr_query_res;
/* 3.7.18.1 基站复位请求 generic req */
/* 3.7.18.1 基站复位回应 generic res */
/* 3.7.19.1 基站时间设置消息请求 */
typedef struct fiberhome_device_time_config_request {
	_fiber_genneric_set_req req;
	S8 _date[16]; /* date 日期(格式 YYYY-MM-DD) */
	S8 _time[16]; /* time 时间(格式 HH:MM:SS) */
} _fiber_time_cfg_req;
/* 3.7.19.1 基站时间设置消息回应 generic res */
/* 3.7.20.1 基站时间查询请求 generic req */
/* 3.7.20.2 基站时间查询请求 */
typedef struct fiberhome_get_time_response {
	_fiber_genneric_set_res res;
	S8 _date[16];/* date YYYY-MM-DD */
	S8 _time[16];/* time HH-MM-SS */
} _fiber_get_time_res;
/* 3.7.21.1 设备信息查询请求 generic req */
/* 3.7.21.2 设备信息查询回应 */
typedef struct fiberhome_device_query_response{
	_fiber_genneric_set_res res;
	S8 version[100]; /* 版本 */
	U8 lte_mode; /* 设备模式1-TDD,2-FDD */
	S8 bands[30];/* 工作频带(多个频带之间使用‘,’分隔) */
	U8 status;/* 设备状态
		     1-running 小区建立成功，正在采集 IMSI
		     2-Ready运行中,设备就绪,等待后台下发配置
		     3-Not Ready,设备未就绪,设备未启动,或启动未完成
		     4-Abnormal,设备状态异常,不能正常工作,需复位
		     5-Sync Measure,空口同步状态 */
	U8 cpu_rate; /* 设备 CPU 占用率,范围: 0~100 */
	U8 mem_rate; /* 设备内存使用率,范围: 0~100 */
	U32 cpu_temp; /* CPU温度 单位:摄氏度, -273 ~ 300*/
	U32 pa_temp; /* 功放温度 单位:摄氏度, -273 ~ 300*/
} _fiber_dev_query_res;
/* 3.7.22.1 版本升级请求 */
typedef struct fiberhome_update_version_request {
	_fiber_genneric_set_req req;
	U8 ip_ver; /* IP版本:0-IPv4,1-IPv6(暂不支持) */
	U8 ftp_server_ip[16];/* FTP 服务器的 IP 地址.
			      * 如果是 IPv4 地址,仅使用前 4个字节.
			      * FTP 服务器的监听端口默认为 21
			      * */
	S8 ftp_user[40]; /* Ftp 连接用户名 */
	S8 ftp_password[40]; /* Ftp 连接密码 */
	S8 filename[256]; /* 升级文件名称 */
} _fiber_update_req;
/* 3.7.22.2 版本升级响应消息 generic res */
/* 3.7.22.3 版本升级结束指示消息 generic rep */
/* 3.7.22.4 版本升级进度上报消息 */
typedef struct fiberhome_update_rate_report {
	_fiber_genneric_inf_rep rep;
	U8 rate; /* 版本升级只是(0~100) */
} _fiber_update_rate_rep;
/* 3.7.23 基站位置查询 no more support*/
/* 3.7.23.1 基站位置查询请求 generic req */
/* 3.7.23.2 基站位置查询响应 */
typedef struct fiberhome_get_site_response {
	_fiber_genneric_set_res res;
	S8 longitude[16]; /* 经度 */
	S8 latitude[16]; /* 纬度 */
	S8 height[16]; /* 高度 */
} _fiber_get_site_res;
/* 3.7.24.1 IMSI 数据上传请求 */
typedef struct fiberhome_get_imsi_ftp_request {
	_fiber_genneric_set_req req;
	U8 ip_ver; /* IP版本:0-IPv4,1-IPv6(暂不支持) */
	U8 ftp_server_ip[16];/* FTP 服务器的 IP 地址.
			      * 如果是 IPv4 地址,仅使用前 4个字节.
			      * FTP 服务器的监听端口默认为 21
			      * */
	S8 ftp_user[40]; /* Ftp 连接用户名 */
	S8 ftp_password[40]; /* Ftp 连接密码 */
} _fiber_imsi_ftp_req;
/* 3.7.24.2 IMSI 数据上传响应 */
typedef struct fiberhome_get_imsi_ftp_response {
	_fiber_genneric_set_res res;
	S8 filename[256]; /* 上传文件名 */
} _fiber_imsi_ftp_res;
/* 3.7.24.3 IMSI 数据上传结束指示 generic rep */
/* 3.7.25 设备信息上报消息 */
typedef struct fiberhome_device_information_report {
	_fiber_genneric_inf_rep rep;
	S8 version[100]; /* 版本 */
	U8 lte_mode; /* 设备模式1-TDD,2-FDD */
	S8 bands[30];/* 工作频带(多个频带之间使用‘,’分隔) */
	U8 status;/* 设备状态
		     1-running 小区建立成功，正在采集 IMSI
		     2-Ready运行中,设备就绪,等待后台下发配置
		     3-Not Ready,设备未就绪,设备未启动,或启动未完成
		     4-Abnormal,设备状态异常,不能正常工作,需复位
		     5-Sync Measure,空口同步状态 */
	U8 cpu_rate; /* 设备 CPU 占用率,范围: 0~100 */
	U8 mem_rate; /* 设备内存使用率,范围: 0~100 */
	U32 cpu_temp; /* CPU温度 单位:摄氏度, -273 ~ 300*/
	U32 pa_temp; /* 功放温度 单位:摄氏度, -273 ~ 300*/
} _fiber_dev_info_report;
/* 3.7.26.1 采集开关请求 */
typedef struct fiberhome_probe_switch_config_request {
	_fiber_genneric_set_req req;
	U8 cmd_type;/* 命令类型:1-开启采集,2-关闭采集,3-重启采集 */
} _fiber_probe_switch_req;
/* 3.7.26.2 采集开关回应 generic res */
/* 3.7.27.1 定时重启请求 */
typedef struct fiberhome_timing_reboot_config_request {
	_fiber_genneric_set_req req;
	S8 time[16];/* 设备定时重启时间,格式hh:mm:ss */
} _fiber_timing_reboot_req;
/* 3.7.27.2 定时重启回应 generic res */
/* 3.7.28.1 键删除邻区列表请求 */
typedef struct fiberhome_delete_adarea_scan_list_request{
	_fiber_genneric_set_req req;
	U8 delete_enable;/* 1开启开关,0关闭开关
			  * 自动建站之前,需先删除邻区列表信息
			  * */
} _fiber_del_adarea_scan_req;
/* 3.7.28.2 键删除邻区列表回应 generic res */
/* 3.7.29.1 邻区表查询请求 generic req */
/* 3.7.29.2 邻区表查询回应 */
typedef struct fiberhome_nbear_poin {
	U32 nbearfcn; /* 邻区频点 */
	U32 nbpci;/* 邻区频点 */
	U16 nbdev; /* 邻区设备号 */
	U32 nbrsrp;/* 邻区基站RSRP */
	U8 nbreselectsib3;/* 重选优先级(SIB3) */
} _fiber_nbear;
typedef struct fiberhome_nb_ear_list_query_response {
	_fiber_genneric_set_res res;
	U16 num;
	_fiber_nbear ary[16];
} _fiber_nbear_list_res;
/* 3.7.30.1 频率表查询请求 generic req */
/* 3.7.30.2 频率表查询回应 */
typedef struct fiberhome_frequency_poin {
	U32 nbearfcn; /* 邻区频点 */
	U8 nbreselectsib5;/* 重选优先级(SIB5) */
} _fiber_freq;
typedef struct fiberhome_frequency_list_query_response {
	_fiber_genneric_set_res res;
	U16 num;
	_fiber_freq ary[16];
} _fiber_freq_list_res;
/* ========================================================================= */
#pragma pack()

void fiber_sta_json_lock();
void fiber_sta_json_unlock();
cJSON *get_fiber_sta_object();
void fiber_sta_json_init();
void fiber_sta_json_delete();
S32 get_fiber_sta_count();
void fiber_sta_count_sum(S32 num);

void *thread_tcp_to_fiberbaseband_lte(void *arg);
void get_fiber_lte_base_band_status(band_entry_t *point);
S8 *get_all_fiber_lte_base_band_status();
void handle_recev_fiber_package(U8 *recBuf, band_entry_t *entry);
void send_generic_request_to_fiberhome(U16 msg_type, band_entry_t *entry);
void fiberhome_pare_config_json_message(S8 *mqtt_msg, band_entry_t *entry);
void *thread_read_send_fiberhome_json_object(void *arg);
#endif /* #ifndef __FIBERHOME_LTE_H__ */
