/*
 * =====================================================================================
 *
 *       Filename:  hash_list_active.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年04月18日 19时00分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __HASH_LIST_ACTIVE_H__
#define __HASH_LIST_ACTIVE_H__
#include <sys/socket.h>
#include<netinet/in.h>
#include "config.h"
#include "list.h"

#define IS_HAVE_BANDDEV_MAC 		(1)
#define IS_HAVE_BANDDEV_SN 		(1 << 1)
#define IS_HAVE_BANDDEV_SVERSION 	(1 << 2)
#define IS_HAVE_BANDDEV_HVERSION 	(1 << 3)
#define IS_HAVE_BANDDEV_DEV_MODEL 	(1 << 4)
#define IS_HAVE_BANDDEV_UBOOTVER        (1 << 5)
#define IS_HAVE_BANDDEV_BOARD_TEMP 	(1 << 6)
#define BASE_BAND_GENERIC_FULL (IS_HAVE_BANDDEV_MAC | IS_HAVE_BANDDEV_SN \
	| IS_HAVE_BANDDEV_SVERSION | IS_HAVE_BANDDEV_DEV_MODEL)

/* ============= 将 baseband 信息放置在链表中 ============= */
/* ======================================================== */
#pragma pack(1)
typedef struct base_band_information_node {
	struct list_head node; // pre and next
	pthread_mutex_t lock;  //节点锁
// net
	char *ipaddr; //基带设备的IP地址
	char *changed_ip;//基带设备遇到修改IP时目标修改的IP地址
	int sockfd;  //基带设备连接上来所用的fd
	int online; //基带设备在线情况
	int _auto; //判断是否发送获取基础信息的请求
	int change_count; //修改次数
// baseband info
	U8 is_have;
	char *macaddr;   //基带设备mac地址
	char *sn;        //基带串号
	char *s_version; //基带设备软件版本
	char *dev_model; //基带设备型号
	char *h_version; //基带设备硬件版本
	U16 work_mode;   //基带设备工作模式
	char *ubt_version; //基带板UBOOT版本
	char *board_temp; //板卡温度
// sync status
	U16 u16RemMode;   /*同 步 类 型 0:sniffer;1:gps*/
	U16 u16SyncState; /*0:GPS 同步成功;1:空口同步成功,2:未同步*/
// cell state
	U32 cellstate; //小区状态 0Idle,1REM,2激活中,3已激活,4去激活
// brand config information
	U8 control_type; //管控类型 0--黑名单 1--白名单(默认黑名单)
	U8 control_list[C_MAX_CONTROL_LIST_UE_NUM][C_MAX_IMSI_LEN]; // 管控名单
// txpower
	U8 regain;       // 接收增益 只保存数据库中的值 
	U8 powerderease; // 发射功率 只保存数据库中的值
// clients count these cliprobe
	char client_find_count;
	char client_no_find_count;
// freq config status
	U32 sysUlARFCN;	/*上行频点*/
	U32 sysDlARFCN;	/*下行频点*/
	U8 PLMN[7]; 	/*plmn str, eg: “46001”*/
	U8 sysBandwidth; /*wrFLBandwidth*/
	U32 sysBand;     /*频段:Band38/band39/band40*/
	U16 PCI;	 /*0~503*/
	U16 TAC;
	U32 CellId;
	U16 UePMax;
	U16 EnodeBPMax;
	/* UE 测量信息 */
	U8 ue_mode; /* UE模式 2-定位模式 */
	U8 ue_IMSI[C_MAX_IMSI_LEN]; /* UE定位模式目标IMSI */

	U8 scan_frequency_flag; //当此值为1/2时,代表正在进行扫频配置
} band_entry_t;
#pragma pack()
extern band_entry_t *entry;

#define OPERATOR_UNKNOW_OR_ALL (0)
#define OPERATOR_CHINA_MOBLIE (1)
#define OPERATOR_CHINA_UNICOM (2)
#define OPERATOR_CHINA_NET (3)

#define GET_DEFAULT_CONFIG (1)
#define GET_STATUS_CONFIG (1 << 1)
#define GET_FREQ_CONFIG (1 << 2)

#endif /* __HASH_LIST_ACTIVE_H__ */
