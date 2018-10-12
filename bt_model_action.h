/*
 * ============================================================================
 *
 *       Filename:  bt_model_action.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/02/2018 10:08:32 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */

#ifndef __BT_MODEL_ACTION_H__
#define __BT_MODEL_ACTION_H__
/* PA控制脚本名称 */
#define CONTROL_PA_CMD "/usr/sbin/control_pa"
/* 串口蓝牙模块设备 */
#define BT_DEV_NAME "/dev/ttyS0"
/* 心跳回应 */
#define DEVICE_HEARTBEATRES "{\"type\":\"ff\"}"
#define LOCALTION_ERROR_STR "{\"type\":\"04\", \"value\":\"%s\"}"
#define LEDINF_ARFCN_STATE_STR "{\"type\":\"06\",\"state\":\"%d\"}"
#define DEVICE_HEART_BEAT_REQ    (0)    /* 心跳检测请求 */
#define DEVICE_SCAN_ARFCN_REQ    (0x01) /* 扫频请求 */
#define DEVICE_AUTO_LOCATION_REQ (0x03) /* 自动定位请求 */
#define STOP_SEND_UE_INFOR_REQ   (0xff) /* 停止商报UE测量 */
#define DEVICE_SET_ARFCN_REQ     (0x07) /* 导入频点信息 */
#define IMSI_PROBE_INFO_SEND_REQ (0x09) /* 是否上报IMSI信息请求 */
#define SERIAL_READ_MAX (8) /* 串口数据长度(8字节) */
/* 手机-->设备 ***命令### */
#define CMD_START "***" 
#define CMD_STOP "###" 
/* 设备-->手机 ###信息*** */
#define REP_START "###"
#define REP_STOP "***\n"

/* YUGA NC5AT指令设备 */
#define MODEL_DEV_NAME "/dev/ttyUSB2"
/* 运营商识别码 */
#define CHINA_MOBILE (0)
#define CHINA_UNICOM (1)
#define CHINA_NET (11)
#define CHINA_MOBILE_LTE_PLMN  "46000"
#define CHINA_UNICOM_LTE_PLMN  "46001"
#define CHINA_NET_LTE_PLMN     "46011"
#define PLMN_LENGTH (5)
#define IMSI_LENGTH (15)
/* 扫频模块是否锁定plmn */
#define UNLOCK_PLMN  (0)
#define LOCK_PLMN  (1)
/* 扫频查询模式*/
#define GSM_ONLY (13)
#define TD_SCDMA_ONLY (15)
#define WCDMA_ONLY  (14)
#define CDMAX_EVDO   (22)
#define LTE_ONLY (38)
/* 扫频模块制式 */
#define GSM_MODE (0)
#define LTE_MODE  (3)

#define BUFSIZE (1024)

/* ============================================== */
typedef struct cell_information_response
{
	int cell_mod; /* 0-GSM, 1-TDS, 2-WCDMA, 3-LTE */
	int mmc;/* 移动国家号码460-中国 */
	int mnc;/* 移动网络号码：00-移动,01-联通,11-电信 */
	char tac[10]; /* 位置区更新 LTE下为TAC */
	char cellid[10]; /* 小区ID */
	int arfcn; /* 主频 */
	int pci; /* 物理小区ID */
	int rxlevel; /* 信号强度 */
} cell_info_t;

typedef struct cdma_info
{
  int sid;       /* 系统ID */
  int nid;       /* 网络ID */
  int bid;       /* Base Station ID基站号 */
  int channel;   /* 当前使用的信道号 */
  int pn;        /* 导频 */
  int rssi;      /* 信号强度 */
  int dev_long;  /* 经度 */
  int dev_lat;   /* 纬度 */
}cdma_info_t;

enum {
	CFG_STATUE_NOCONFIG = 0,
	CFG_STATUE_SET_WORK_MODE_OK,
	CFG_STATUS_SET_ARFCN_OK,
	CFG_STATUS_SET_TXPOWER_OK,
	CFG_STATUS_SET_UE_OK
};

typedef struct track_mode_config {
	unsigned int config_status;/* 0 未配置
				    * 1 设备工作模式配置成功(可能要重启)
				    * 2 小区参数配置成功(TDD配置频宽要重启)
				    * 3 功率衰减值配置成功(基本默认 40)
				    * 4 UE 测量值配置成功
				    * */
	unsigned short work_mode; /* 设备工作模式 0-TDD, 1-FDD */
	unsigned char ue_mode; /* UE工作模式 2-定位 */
	unsigned char imsi[17]; /* 定位IMSI号 */
	unsigned char plmn[7]; /* PLMN (mmc+mnc) */
	unsigned char meas_report_period; /* 测量上报周期 4 - 1024(ms) */
	unsigned char ue_maxpower_switch; /* 中端最大功率开关 0-关,1-开，FDD需要开 */
	unsigned int sysdarfcn; /* 需要更新的设备下行频点 */
	unsigned int cell_pci; /* 物理小区ID */
} track_config_t;

extern track_config_t *track_cfg;
void safe_pthread_create(void *(*start_routine) (void *), void *arg);
int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop);
void init_bluetooth_device_fd();
void set_bluetooth_fd(int fd);
int get_bluetooth_fd();
void *thread_read_bluetooth_cmd(void *arg);
void *thread_pare_bluetooth_cmd_buf(void *arg);
void *thread_read_bluetooth_cmd(void *arg);
char *read_model_response(FILE *file);
char *get_scan_cell_information(char *mnc, int mode, int act, int fd, FILE *file);
char *get_cdma_cell_info(int fd, FILE *file);
int write_to_blue_tooth(int fd, char *str);
int get_scan_model_fd();
void set_scan_model_fd(int fd);
int init_scan_model_device_fd();
int get_imsi_send_flag();
int get_ue_send_flag();
void set_ue_send_flag(int f);
int open_model_devid(FILE **file);
#endif /* __BT_MODEL_ACTION_H__ */
