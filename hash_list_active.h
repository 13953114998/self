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

#ifndef MAX_HLIST_LENGTH
#define MAX_HLIST_LENGTH (255)
#endif

#define WCDMA_BASE_DEVICE            4
#define WCDMA_REQUEST_PKGTYPE_LENGTH (2 + 2)

typedef struct hash_info_list {
	struct hlist_head head;
	pthread_mutex_t list_lock;
	int entry_count;
} hash_list_t;

/* 基带设备信息保存哈希表节点结构体 */
typedef struct clients_info_node {
	hash_list_t *hash_head;//所属哈希头指针
	int hash_pos;          //哈希桶位置
	struct hlist_node node;//哈希节点
	pthread_mutex_t lock;  //节点锁
	int count; // get or put entry
/* 网络通信信息 */
	char *ipaddr;//基带设备的IP地址
	int sockfd;  //基带设备连接上来所用的fd
/* 基带板基本信息 */
	char *dev_model;//基带设备型号
	char *h_version;//基带设备硬件版本
	char *s_version;//基带设备软件版本
	char *sn;       //基带串号
	char *macaddr;  //基带设备mac地址
	U16 work_mode; //基带设备工作模式
	U32 sys_band;  //设备所处的频段
	U8 plmn[7]; //plmn
/* 小区状态信息 */
	U32 cellstate; //小区状态 0Idle,1REM,2激活中,3已激活,4去激活
/* 管控类型以及管控名单新 */
	U8 control_type; //管控类型 0--黑名单 1--白名单(默认黑名单)
	U8 control_list[C_MAX_CONTROL_LIST_UE_NUM][C_MAX_IMSI_LEN];// 管控名单(黑白一体)
/* 发射功率信息 */
	U8 is_saved;
} client_hentry_t;

/* 采集信息的哈希表节点结构体 */
typedef struct station_info_node {
	// hash
	hash_list_t *hash_head;//所属哈希头指针
	int hash_pos;          //哈希桶位置
	struct hlist_node node;//哈希节点
	pthread_mutex_t lock;  //节点锁
	//statsion 采集信息
	U8 imsi[17];// imsi
	U8 imei[17];// imei
	time_t fist_time; //第一次捕获的时间
	int find_count; //去重周期内获取的次数
} station_hentry_t;

hash_list_t *hlist_clients_head; //储存基带板的基础信息的哈希表头
hash_list_t *hlist_station_head; //储存采集信息的哈希表头(主要:imsi 信息)

void get_entry_lockself(client_hentry_t *entry);
void *check_put_entry_nolock(void *arg);

/* 初始化哈希表 */
void hash_init();
/* 计数(+) 未加锁*/
void get_entry_nolock(client_hentry_t *entry);
/* 计数(+) 加锁 */
void get_entry(client_hentry_t *entry);
/* 计数(-)(加锁)  (如果count == 0 表示已经下线，删除节点)*/
void put_entry(client_hentry_t *entry);
/* 查找节点 */
client_hentry_t *hash_list_search_nolock(hash_list_t *hash_head, char *ipin);
client_hentry_t *hash_list_search(hash_list_t *hash_head, char *ipin);
/* 查找并添加新的节点 */
client_hentry_t *hash_list_search_new(hash_list_t *hash_head, char *ipin);

/* ========================================================================== */
/* ========================================================================== */
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
/* 功放地址与电源板控制GPIO编号 */
	U8 pa_num;
	S32 gpio_num;
	U8 gsm_carrier_type;   //GSM载波指示
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

	U8 scan_frequency_flag; //当此值为1/2时,代表正在进行扫频配置
	/* only Fiberhome about */
	S8 device_id[22];
	S8 *fiber_bands; /* bands38,39,... one more */
	U8 lte_mode; /* fiberhome LTE mode 1-TDD,2-FDD*/
	U16 oper; /* 运营商 0-移动,1-联通,2-电信 */
	U32 collectperiod; /* 采集周期 */
} band_entry_t;
#pragma pack()

#define OPERATOR_UNKNOW_OR_ALL (0)
#define OPERATOR_CHINA_MOBLIE (1)
#define OPERATOR_CHINA_UNICOM (2)
#define OPERATOR_CHINA_NET (3)

#define GET_DEFAULT_CONFIG (1)
#define GET_STATUS_CONFIG (1 << 1)
#define GET_FREQ_CONFIG (1 << 2)

int band_entry_count;//链表中节点个数
pthread_mutex_t band_list_lock; //链表总锁

void band_list_head_init();
struct list_head *return_band_list_head();
band_entry_t *band_list_entry_search(char *ipaddr);
band_entry_t *band_list_entry_search_add(char *ipaddr);
char *get_lte_base_band_status(U8 get_type, S8 *addr);
char *get_gsm_base_band_status();

#endif /* __HASH_LIST_ACTIVE_H__ */
