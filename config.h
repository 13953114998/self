#ifndef __CONFIG_H__
#define __CONFIG_H__
#define DEBUG
/* #undef BIG_END */

typedef unsigned char 	   U8;
typedef unsigned short int U16;
typedef unsigned int 	   U32;
typedef unsigned long long U64;
typedef char          S8;
typedef short int     S16;
typedef int 	      S32;
typedef long long int S64;
typedef double	      F64;

extern F64 my_longitude; /* 经度 */
extern F64 my_latitude;  /* 纬度 */
extern F64 my_altitude;  /* 高度 */
#define C_MAX_IMSI_LEN 	17 /* IMSI 数据长度 */
#define C_MAX_IMEI_LEN 	17 /* IMEI 数据长度 */
#define C_MAX_REM_ARFCN_NUM 	10 /* 最大的扫频频点数目 */
#define C_MAX_INTRA_NEIGH_NUM 	32 /* 最大的同频邻区数目 */
#define C_MAX_DEFAULT_ARFCN_NUM 50 /* 小区自配置对应的默认扫频ARFCN总数 */
#define C_MAX_CONTROL_LIST_UE_NUM 200 /* 管控名单中可以含有的最大UE数 */
#define C_MAX_CONTROL_PROC_UE_NUM 10 /* 管控名单每次最大可以添加/删除的UE数 */
#define MAXDATASIZE 	 (1024)
#define MAX_RECEV_LENGTH (20480)
#define C_MAX_LOCATION_BLACKLIST_UE_NUM  50 /* 定位黑名单中可以含有的最大UE数 */
#define C_MAX_COLLTECTION_INTRA_CELL_NUM 8 /* 最大的同频采集小区数目 */

#define TRUE   1
#define FALSE  0

#define TDD    0
#define FDD    1

/* 运营商信息 */
#define OPERATOR_UNKNOW_O_ALL (0) /* 未知或多个运营商 */
#define OPERATOR_CHINA_MOBILE (1) /* 中国移动 */
#define OPERATOR_CHINA_UNICOM (2) /* 中国联通 */
#define OPERATOR_CHINA_NET (3) /* 中国电信 */

void safe_pthread_create(void *(*start_routine) (void *), void *arg);
U64 my_htonll_ntohll(U64 in);
U32 my_htonl_ntohl(U32 in);
U16 my_htons_ntohs(U16 in);
void my_btol_ltob(void *arg, int len);
/* 检测并创建目录 */
void create_directory(const char *path_name);
void send_string_udp(void *send_string, int send_length);
int init_udp_send_imsi_cli(char *ipaddr, int port);
void send_ue_string_udp(void *send_string, int send_length);
int init_ue_udp_send_cli(char *ipaddr, int port);
#endif /* __CONFIG_H__ */
