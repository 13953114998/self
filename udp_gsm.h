/*
 * =====================================================================================
 *
 *       Filename:  udp_gsm.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年06月05日 17时20分23秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __UDP_GSM_H__
#define __UDP_GSM_H__
#include "config.h"
/*消息类型可变长度宏定义*/
#define GSM_VERSION_LEN           20
#define GSM_CFG_MODE_LEN	  1  //new add,2017.10.17
#define GSM_WORKMODE_LEN          1
#define GSM_FRE_ONE_LEN           2
#define GSM_FRE_TWO_LEN           2
#define GSM_FRE_OFFSET_LEN        2
#define GSM_CAPTURE_INTERVAL_LEN  4  //new add,2017.10.17
#define GSM_RUN_STATUS_LEN        4 /* new add, 2018.05.26 by yunsangc */

#define SEND_TO_GSM_LEN           8 + 11 + 11 + 11 + 11 + 11 + 11 + 11 + 4 + 4 + 5 + 5 + 5 + 5 + 5
#define GSM_HEADER_LEN            8
#define GSM_MSG_HEADER_LEN        3


/*与gsm通信的宏定义*/
#define O_SEND_TOGSM_GETCFGONE   (0xA001)   //查询移动频点配置
#define O_SEND_TOGSM_GETCFGTWO   (0xA002)   //查询联通频点配置
#define O_SEND_TOGSM_SETCFGONE   (0xA003)   //设置移动频点配置
#define O_SEND_TOGSM_SETCFGTWO   (0xA004)   //设置联通频点配置
#define O_GET_GSM_CAPTIME   	 (0xA005)   //查询重新捕获间隔时间
#define O_SET_GSM_CAPTIME   	 (0xA006)   //设置重新捕获间隔时间

/*消息编号*/
#define O_SEND_TO_GSM_QUERY       0x01   //查询
#define O_FROM_GSM_QUERY_ACK      0x11   //查询应答
#define O_SEND_TO_GSM_SET         0x02   //设置
#define O_FROM_GSM_SET_ACK        0x12   //设置应答
#define O_SEND_TO_GSM_NETSCAN     0x03   //网络扫描
#define O_FROM_GSM_NETSCAN_ACK    0x13   //网络扫描应答
#define O_SEND_TO_GSM_ON_RF       0x05   //开启射频
#define O_FROM_GSM_ON_RF_ACK      0x15   //开启射频应答
#define O_SEND_TO_GSM_OFF_RF      0x06   //关闭射频
#define O_FROM_GSM_OFF_RF_ACK     0x16   //关闭射频应答
#define O_SEND_TO_GSM_RES_SYS     0x07   //重启系统
#define O_FROM_GSM_RES_SYS_ACK    0x17   //重启系统应答
#define O_SEND_TO_GSM_CARRIER_RES 0x09   //载波复位
#define O_FROM_GSM_CARRIER_RES_ACK 0x19  //载波复位应答
#define O_FROM_GSM_MSG_REP        0x18   //上报
#define O_FROM_GSM_SCAN_REP       0x1E   //GSM扫描结果上报
#define O_FROM_GSM_IND            0x1F   //心跳

/*设备基本参数*/
#define O_GSM_VERSION             0x0001  //软件版本
#define O_GSM_RUN_STATE           0x0002  //运行状态
#define O_GSM_NTP_SERVER          0x0003  //NTP服务器
#define O_GSM_RUN_TIME            0x0004  //运行时间
#define O_GSM_DEV_TIME            0x0005  //设备时间
#define O_GSM_DEV_IP_ADDR         0x0010  //设备IP地址
#define O_GSM_DEV_MASK            0x0011  //设备IP地址掩码
#define O_GSM_GET_CATCH_TIME      0x010C  //重新捕获间隔时间

/*运营商参数及工作模式*/
#define O_GSM_MCC                 0x0101  //国家码（MCC）
#define O_GSM_MNC                 0x0102  //网络码（MNC）
#define O_GSM_LAC                 0x0103  //位置区号（LAC）GSM特有
#define O_GSM_CI                  0x0104  //小区号（CI）
#define O_GSM_CONFIG_MODE         0x010A  //配置模式
#define O_GSM_WORK_MODE           0x010B  //工作模式

/*CDMA小区特有参数*/
#define O_GSM_CDMA_SID            0x0111  //SID
#define O_GSM_CDMA_NID            0x0112  //NID
#define O_GSM_CDMA_BSID           0x0113  //BSID
#define O_GSM_CDMA_REG_CODE       0x0114  //登记区识别码（CDMA）
#define O_GSM_CDMA_WORK_MODE      0x0119  //工作模式(CDMA)

/*射频参数*/
#define O_GSM_CARRIER_FRE         0x0150  //载波频点 GSM特有
#define O_GSM_DOWN_ATTEN          0x0151  //下行衰减
#define O_GSM_UP_ATTEN            0x0152  //上行衰减
#define O_GSM_START_FRE_ONE       0x0156  //起始频点1
#define O_GSM_STOP_FRE_ONE        0x0157  //结束频点1
#define O_GSM_START_FRE_TWO       0x0158  //起始频点2
#define O_GSM_STOP_FRE_TWO        0x0159  //结束频点2
#define O_GSM_VCO                 0x015E  //VCO校准值
#define O_GSM_FRE_OFFSET          0x015F  //频率偏移

/*上报消息*/
#define O_GSM_REP_OPE             0x0200  //运营商
#define O_GSM_REP_IMSI            0x0211  //IMSI
#define O_GSM_REP_IMEI            0x0212  //IMEI
#define O_GSM_REP_TMSI            0x0213  //TMSI
#define O_GSM_REP_ESN             0x0214  //ESN
#define O_GSM_SIGNAL_INT          0x0215  //信号强度

#define O_GSM_CDMA_PN             0x0110  //PN码（CDMA）

#define GSM_CARRIER_TYPE_MOBILE (1)      /* GSM 1 载波 */
#define GSM_CARRIER_TYPE_UNICOM (1 << 1) /* GSM 2 载波 */
/* GSM 所有 载波 */
#define GSM_CARRIER_TYPE_ALL (GSM_CARRIER_TYPE_MOBILE | GSM_CARRIER_TYPE_UNICOM)

#pragma pack(1)
typedef struct gsm_msg{
	U8 gsm_msg_len;  //消息体长度
	U16 gsm_msg_type;//消息体信息编号
	U8 *gsm_msg;     //消息体内容（可变长度）
}gsm_msg_t;


typedef struct WrtogsmHeader{
	U32 msg_len;     //发送给gsm的信息总长度
	U8 msg_num;      //信息序号
	U8 msg_type;     //信息编号
	U8 carrier_type; //载波指示
	U8 msg_action;   //消息功能参数
}WrtogsmHeader_t;

typedef struct wrtogsmfreqcfg{
	WrtogsmHeader_t WrtogsmHeaderInfo;
	U8 bcc_len;
	U16 bcc_type;
	U8 bcc[8];
	U8 mcc_len;
	U16 mcc_type;
	U8 mcc[8];
	U8 mnc_len;
	U16 mnc_type;
	U8 mnc[8];
	U8 lac_len;
	U16 lac_type;
	U8 lac[8];
	U8 lowatt_len;
	U16 lowatt_type;
	U8 lowatt[8];
	U8 upatt_len;
	U16 upatt_type;
	U8 upatt[8];
	U8 cnum_len;
	U16 cnum_type;
	U8 cnum[8];
	U8 cfgmode_len;
	U16 cfgmode_type;
	U8 cfgmode;
	U8 workmode_len;
	U16 workmode_type;
	U8 workmode;
	U8 startfreq_1_len;
	U16 startfreq_1_type;
	S16 startfreq_1;
	U8 endfreq_1_len;
	U16 endfreq_1_type;
	S16 endfreq_1;
	U8 startfreq_2_len;
	U16 startfreq_2_type;
	S16 startfreq_2;
	U8 endfreq_2_len;
	U16 endfreq_2_type;
	S16 endfreq_2;
	U8 freqoffset_len;
	U16 freqoffset_type;
	S16 freqoffset;
}wrtogsmfreqcfg_t;

/*GSM上报信息*/
typedef struct gsm_rep_msg{
	U32 perator; //运营商
	U8 lac[8];   //位置区号（LAC）
	U8 imsi[20]; //IMSI
	U8 imei[20]; //IMEI
	U8 tmsi[20]; //TMSI
	S16 signal;  //信号强度
}gsm_rep_msg_t;

/*扫描上报信息结构体*/
typedef struct nb_cell_item{
	U16 wARFCN;   //小区频点
	U16 wBSIC;    //物理小区ID
	S8 cRSSI;     //信号功率
	U8 bReserved; //保留
	S8 cC1;       //C1测量值
	U8 bC2;       //C2测量值
}nb_cell_item_t;

typedef struct nb_cell_info_item{
	S32 bGlobalCellId;
	U8 bPLMNId[5]; //PLMN标志
	S8 cRSSI;      //信号功率
	U16 wLAC;      //追踪区域码
	U16 wBSIC;     //物理小区ID
	U16 wARFCN;    //小区频点
	S8 cC1;        //参考发射功率
	U8 bNbCellNum; //邻小区个数
	U8 bReserved;  //保留字段
	U8 bC2;        //C2测量值
	nb_cell_item_t stNbCell[32];
}nb_cell_info_item_t;
#pragma pack()

void handle_send_to_gsm(WrtogsmHeader_t *WrtogsmHeader, U32 carrier_t);
void handle_gsm_req(wrtogsmfreqcfg_t *pStr);
void handle_gsm_query_set_ack(char *recv_buf, int msg_len, U8 gsm_carrier_type);
void handle_from_gsm_ind(char *recv_buf);   //心跳信息处理
void handle_from_gsm_msg_rep(char *recv_buf, int msg_len);
void handle_from_gsm_scan_rep(char *recv_buf, int msg_len);//扫描结果信息处理
void handle_gsm_station(char *recv_buf);
void handle_recv_gsm(char *recvfrom_buf);
void *thread_send_gsm(void *arg); //循环查询GSM的基础信息(软件版本与工作模式)
void *thread_recv_gsm(void *arg);
void *thread_udp_to_gsm(void *arg);
int get_gsm_cap_time();
void set_gsm_cap_time(int cap);
void *thread_gsm_cap_time(void *arg);
void *read_send_gsm_json_object(void *arg);
void *read_andsend_gsm_scan_json_object(void *arg);
#endif /* __UDP_GSM_H__ */
