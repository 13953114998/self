/*
 * =====================================================================================
 *
 *       Filename:  baicell_com.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年05月30日 15时10分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __BAICELL_NET_H__
#define __BAICELL_NET_H__
#include <sqlite3.h>
#include <pthread.h>

#include "config.h"
#include "hash_list_active.h"
#include "cJSON.h"


#define BAICELL_PROTOCOL_TCP (1)
#define BAICELL_PROTOCOL_UDP (0)

//define some macro
#define MSG_FRAMEHEAD_DEF  0x5555AAAA
#define SYS_WORK_MODE_TDD  0xFF
#define SYS_WORK_MODE_FDD  0xFF00
#define SYS_CODE_DEFAULT   0x0000

//Baicell (or all)Msg 
#define O_FL_LMT_TO_ENB_SYS_MODE_CFG 		(0xF001)
#define O_FL_ENB_TO_LMT_SYS_MODE_ACK 		(0xF002)
#define O_FL_LMT_TO_ENB_SYS_ARFCN_CFG 		(0xF003)
#define O_FL_ENB_TO_LMT_SYS_ARFCN_ACK 		(0xF004)
#define O_FL_ENB_TO_LMT_UE_INFO_RPT 		(0xF005)
#define O_FL_LMT_TO_ENB_MEAS_UE_CFG 		(0xF006)
#define O_FL_ENB_TO_LMT_MEAS_UE_ACK 		(0xF007)
#define O_FL_ENB_TO_LMT_MEAS_INFO_RPT 		(0xF008)
#define O_FL_LMT_TO_ENB_REM_CFG 		(0xF009)
#define O_FL_ENB_TO_LMT_REM_INFO_RPT 		(0xF00A)
#define O_FL_LMT_TO_ENB_REBOOT_CFG 		(0xF00B)
#define O_FL_ENB_TO_LMT_REBOOT_ACK 		(0xF00C)
#define O_FL_LMT_TO_ENB_SET_ADMIN_STATE_CFG 	(0xF00D)
#define O_FL_ENB_TO_LMT_SET_ADMIN_STATE_ACK 	(0xF00E)
#define O_FL_ENB_TO_LMT_SYS_INIT_SUCC_IND 	(0xF010)
#define O_FL_LMT_TO_ENB_SYS_INIT_SUCC_RSP 	(0xF011)
#define O_FL_LMT_TO_ENB_SYS_RxGAIN_CFG 		(0xF013)
#define O_FL_ENB_TO_LMT_SYS_RxGAIN_ACK 		(0xF014)
#define O_FL_LMT_TO_ENB_SYS_PWR1_DEREASE_CFG 	(0xF015)
#define O_FL_ENB_TO_LMT_SYS_PWR1_DEREASE_ACK 	(0xF016)
#define O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG 	(0xF017)
#define O_FL_ENB_TO_LMT_REDIRECT_INFO_ACK 	(0xF018)
#define O_FL_LMT_TO_ENB_GET_ENB_STATE 		(0xF01A)
#define O_FL_ENB_TO_LMT_ENB_STATE_IND 		(0xF019)
#define O_FL_LMT_TO_ENB_IP_CFG 			(0xF01B)
#define O_FL_ENB_TO_LMT_IP_CFG_ACK 		(0xF01C)
#define O_FL_LMT_TO_ENB_RESET_CFG 		(0xF01D)
#define O_FL_ENB_TO_LMT_RESET_ACK 		(0xF01E)
#define O_FL_LMT_TO_ENB_SET_SYS_TMR 		(0xF01F)
#define O_FL_ENB_TO_LMT_SET_SYS_TMR_ACK 	(0xF020)
#define O_FL_LMT_TO_ENB_SET_QRXLEVMIN 		(0xF021)
#define O_FL_ENB_TO_LMT_SET_QRXLEVMIN_ACK 	(0xF022)
#define O_FL_LMT_TO_ENB_REM_MODE_CFG 		(0xF023)
#define O_FL_ENB_TO_LMT_REM_MODE_CFG_ACK 	(0xF024)
#define O_FL_LMT_TO_ENB_LMTIP_CFG 		(0xF025)
#define O_FL_ENB_TO_LMT_LMTIP_CFG_ACK 		(0xF026)
#define O_FL_LMT_TO_ENB_GET_ARFCN 		(0xF027)
#define O_FL_ENB_TO_LMT_ARFCN_IND 		(0xF028)
#define O_FL_LMT_TO_ENB_GPS_PP1S_CFG 		(0xF029)
#define O_FL_ENB_TO_LMT_GPS_PP1S_ACK 		(0xF02A)  //need to confirm
#define O_FL_LMT_TO_ENB_BASE_INFO_QUERY 	(0xF02B)
#define O_FL_ENB_TO_LMT_BASE_INFO_QUERY_ACK 	(0xF02C)
#define O_FL_LMT_TO_ENB_SYNC_INFO_QUERY 	(0xF02D)
#define O_FL_ENB_TO_LMT_SYNC_INFO_QUERY_ACK 	(0xF02E)
#define O_FL_LMT_TO_ENB_CELL_STATE_INFO_QUERY 		 (0xF02F)
#define O_FL_ENB_TO_LMT_CELL_STATE_INFO_QUERY_ACK 	 (0xF030)
#define O_FL_LMT_TO_ENB_RXGAIN_POWER_DEREASE_QUERY 	 (0xF031)
#define O_FL_ENB_TO_LMT_RXGAIN_POWER_DEREASE_QUERY_ACK 	 (0xF032)
#define O_FL_LMT_TO_ENB_ENB_IP_QUERY 			 (0xF033)
#define O_FL_ENB_TO_LMT_ENB_IP_QUERY_ACK 		 (0xF034)
#define O_FL_LMT_TO_ENB_QRXLEVMIN_VALUE_QUERY 		 (0xF035)
#define O_FL_ENB_TO_LMT_QRXLEVMIN_VALUE_QUERY_ACK 	 (0xF036)
#define O_FL_LMT_TO_ENB_REM_CFG_QUERY 			 (0xF037)
#define O_FL_ENB_TO_LMT_REM_CFG_QUERY_ACK 		 (0xF038)
#define O_FL_LMT_TO_ENB_CONTROL_UE_LIST_CFG 		 (0xF039)
#define O_FL_ENB_TO_LMT_CONTROL_UE_LIST_CFG_ACK 	 (0xF03A)
#define O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON 		 (0xF03B)
#define O_FL_ENB_TO_LMT_SELF_ACTIVE_CFG_PWR_ON_ACK 	 (0xF03C)
#define O_FL_LMT_TO_ENB_MEAS_UE_CFG_QUERY 		 (0xF03D)
#define O_FL_ENB_TO_LMT_MEAS_UE_CFG_QUERY_ACK 		 (0xF03E)
#define O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG_QUERY 	 (0xF03F)
#define O_FL_ENB_TO_LMT_REDIRECT_INFO_CFG_QUERY_ACK 	 (0xF040)
#define O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON_QUERY 	 (0xF041)
#define O_FL_ENB_TO_LMT_SELF_ACTIVE_CFG_PWR_ON_QUERY_ACK (0xF042)
#define O_FL_LMT_TO_ENB_CONTROL_LIST_QUERY 		 (0xF043)
#define O_FL_ENB_TO_LMT_CONTROL_LIST_QUERY_ACK 		 (0xF044)

#define O_FL_ENB_TO_LMT_CUTNET_CONFIG_QUERY			(0xF045)
#define O_FL_LMT_TO_ENB_CUTNET_CONFIG_SETUP			(0xF046)
#if 0
#define O_FL_LMT_TO_ENB_SET_PA_SWITCH_REQUEST 	 	 (0xF045)
#define O_FL_ENB_TO_LMT_SET_PA_SWITCH_REQUEST_ACK 	 (0xF046)
#define O_FL_LMT_TO_ENB_PA_STATE_QUERY  		 (0xF047)
#define O_FL_ENB_TO_LMT_PA_STATE_QUERY_ACK 		 (0xF048)
#endif
/* Log级别设置 */
#define O_FL_LMT_TO_ENB_SYS_LOG_LEVL_SET	(0xF045)
#define O_FL_ENB_TO_LMT_SYS_LOG_LEVL_SET_ACK 	(0xF046)
/* Log级别查询 */
#define O_FL_LMT_TO_ENB_SYS_LOG_LEVL_QUERY  	(0xF047)
#define O_FL_ENB_TO_LMT_SYS_LOG_LEVL_QUERY_ACK	(0xF048)
/* 设置 TDD 子帧配置 */
#define O_FL_LMT_TO_ENB_TDD_SUBFRAME_ASSIGNMENT_SET       (0xF049) 
#define O_FL_ENB_TO_LMT_TDD_SUBFRAME_ASSIGNMENT_SET_ACK   (0xF04A)

#define O_FL_LMT_TO_ENB_TDD_SUBFRAME_ASSIGNMENT_QUERY  	  (0xF04B) 
#define O_FL_ENB_TO_LMT_TDD_SUBFRAME_ASSIGNMENT_QUERY_ACK (0xF04C)
/* 频点自配置后台频点列表查询 */
#define O_FL_LMT_TO_ENB_SELFCFG_ARFCN_QUERY       (0xF04D)
#define O_FL_ENB_TO_LMT_SELFCFG_ARFCN_QUERY_ACK   (0xF04E)
/* 小区自配置请求 */
#define O_FL_LMT_TO_ENB_SELFCFG_CELLPARA_REQ 	  (0xF04F)
#define O_FL_LMT_TO_ENB_SELFCFG_CELLPARA_REQ_ACK  (0xF050)
/* 频点自配置后台频点添加/删除  */
#define O_FL_LMT_TO_ENB_SELFCFG_ARFCN_CFG_REQ 	  (0xF051)
#define O_FL_ENB_TO_LMT_SELFCFG_ARFCN_CFG_REQ_ACK (0xF052)
/* 位置区更新拒绝原因值配置 */
#define O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_CFG     (0xF057)
#define O_FL_ENB_TO_LMT_TAU_ATTACH_REJECT_CAUSE_CFG_ACK (0xF058)
/* 初始频偏配置 */
#define O_FL_LMT_TO_ENB_FREQ_OFFSET_CFG 	(0xF059)
#define O_FL_ENB_TO_LMT_FREQ_OFFSET_CFG_ACK 	(0xF05A)
/* 告警指示 */
#define O_FL_ENB_TO_LMT_ALARMING_TYPE_IND       (0xF05B)
/* 经纬高度查询 */
#define O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY     	(0xF05C)
#define O_FL_ENB_TO_LMT_GPS_LOCATION_QUERY_ACK	(0xF05D)
/* 异频同步频点及 band 配置 */
#define O_FL_LMT_TO_ENB_SYNCARFCN_CFG 	    (0xF05E)
#define O_FL_LMT_TO_ENB_SYNCARFCN_CFG_ACK   (0xF05F)
/* 随机接入成功率问询 */
#define O_FL_LMT_TO_ENB_RA_ACCESS_QUERY     (0xF065)
#define O_FL_ENB_TO_LMT_RA_ACCESS_QUERY_ACK (0xF066)
/* 随机接入成功率清空请求 */
#define O_FL_LMT_TO_ENB_RA_ACCESS_EMPTY_REQ 	(0xF067)
#define O_FL_ENB_TO_LMT_RA_ACCESS_EMPTY_REQ_ACK (0xF068) 
/* TAC 手动修改配置下发 */
#define O_FL_LMT_TO_ENB_TAC_MODIFY_REQ     (0xF069)
#define O_FL_ENB_TO_LMT_TAC_MODIFY_REQ_ACK (0xF06A)
/* 辅 PLMN 列表配置 */
#define O_FL_LMT_TO_ENB_SECONDARY_PLMNS_SET 	  (0xF060)
#define O_FL_ENB_TO_LMT_SECONDARY_PLMNS_SET_ACK   (0xF061)
/* 辅 PLMN 列表查询 */
#define O_FL_LMT_TO_ENB_SECONDARY_PLMNS_QUERY 	  (0xF062)
#define O_FL_ENB_TO_LMT_SECONDARY_PLMNS_QUERY_ACK (0xF063)
/* 位置区更新拒绝原因查询 */
#define O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_QUERY     (0xF06B)
#define O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_QUERY_ACK (0xF06C)
/* GPS 信息复位配置 */
#define O_FL_LMT_TO_ENB_GPS_INFO_RESET 	   (0xF06D)
#define O_FL_ENB_TO_LMT_GPS_INFO_RESET_ACK (0xF06E)
/* 基站版本升级配置 */
#define O_FL_LMT_TO_ENB_UPDATE_SOFT_VERSION_CFG     (0xF06F)
#define O_FL_ENB_TO_LMT_UPDATE_SOFT_VERSION_CFG_ACK (0xF070)
/* 获取基站 log */
#define O_FL_LMT_TO_ENB_GET_ENB_LOG   (0xF071)
#define O_ENB_TO_LMT_GET_ENB_LOG_ACK  (0xF072)
/* Gps同步偏移量查询 */
#define O_FL_LMT_TO_ENB_GPS1PPS_QUERY      (0xF073)
#define O_FL_ENB_TO_LMT_GSP1PPS_QUERY_ACK  (0xF074)
/* AGC 配置 */
#define O_FL_LMT_TO_ENB_AGC_SET     (0xF079)
#define O_FL_ENB_TO_LMT_AGC_SET_ACK (0xF07A)
/* 扫频配置 */
#define O_FL_LMT_TO_ENB_REM_ANT_CFG      (0xF07D)
#define O_FL_ENB_TO_LMT_REM_ANT_CFG_ACK  (0xF07E)
/*小区频点动态修改*/
#define O_FL_LMT_TO_ENB_SYS_ARFCN_MOD	    (0xF080)
#define O_FL_ENB_TO_LMT_SYS_ARFCN_MOD_ACK   (0xF081)
/*选频配置*/
#define O_FL_LMT_TO_ENB_SELECT_FREQ_CFG	    (0xF082)
#define O_FL_ENB_TO_LMT_SELECT_FREQ_CFG_ACK (0xF083)
/* 4.7.19 NTP 服务器IP配置 */
#define O_FL_LMT_TO_ENB_NTP_SERVER_IP_CFG (0xF075)
#define O_FL_ENB_TO_LMT_NTP_SERVER_IP_CFG_ACK (0xF076)
/* 4.7.20 定点重启配置*/
#define O_FL_LMT_TO_ENB_TIME_TO_RESET_CFG     (0xF086)
#define O_FL_ENB_TO_LMT_TIME_TO_RESET_CFG_ACK (0xF087)
/* IMEI捕获功能配置 */
#define O_FL_LMT_TO_ENB_IMEI_REQUEST_CFG     (0xF08A)
#define O_FL_ENB_TO_LMT_IMEI_REQUEST_CFG_ACK (0xF08B)

#define MAX_INTER_FREQ_NGH 32
#define MAX_INTER_FREQ_LST 16

//user report info type define
typedef enum ReportType
{
	IMSI_TYPE = 0,
	IMEI_TYPE = 1,
	IMSI_IMEI_TYPE = 2,
	MAX_TYPE = 3
} ReportType_t;
#pragma pack(1)
//General message header
typedef struct wrMsgHeader
{
	U32 u32FrameHeader;/*0x5555AAAA*/
	U16 u16MsgType;    /*定义消息的类型名称*/
	U16 u16MsgLength;  /*定义消息的长度*/
	U16 u16frame;      /*系统工作模式,FDD: 0xFF00, TDD: 0x00FF*/
	U16 u16SubSysCode; /*定义消息的产生的子系统编号*/
}wrMsgHeader_t;

//common Msg Body
//for some MSG, we only need to set it's msgType to inform eNodeB who we are
typedef struct wrFLLmtToEnbComMsgBody
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
}wrFLLmtToEnbComMsgBody_t;

//common Msg Response
typedef struct wrFLEnbToLmtComMsgRes
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
} wrFLEnbToLmtComMsgRes_t;

//3.1 系统工作模式配置,O_FL_LMT_TO_ENB_SYS_MODE_CFG
typedef struct wrFLLmtToEnbSysModeCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 sysMode;/*工作模式:0:TDD 1:FDD*/
}wrFLLmtToEnbSysModeCfg_t;

//3.2 系统工作模式配置应答,O_FL_ENB_TO_LMT_SYS_MODE_ACK
typedef struct wrFLEnbToLmtSysModeAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtSysModeAck_t;

//3.3. 频点配置,O_FL_LMT_TO_ENB_SYS_ARFCN_CFG
typedef enum wrFLBandwidth
{
	BW_RB_6 = 6,
	/*not support*/
	BW_RB_15 = 15, /*not support*/
	BW_RB_25 = 25,
	/*5M, only fdd support*/
	BW_RB_50 = 50,
	/*10M, tdd+fdd support*/
	BW_RB_75 = 75,
	/*15M. only fdd support*/
	BW_RB_100 = 100 /*20M, tdd+fdd support*/
} wrFLBandwidth_t; /*系统带宽枚举*/

typedef struct wrFLLmtToEnbSysArfcnCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 sysUlARFCN;	/*上行频点*/
	U32 sysDlARFCN;	/*下行频点*/
	U8 PLMN[7]; 	/*plmn str, eg: “46001”*/
	U8 sysBandwidth;/*wrFLBandwidth*/
	U32 sysBand;	/*频段:Band38/band39/band40*/
	U16 PCI;	/*0~503*/
	U16 TAC;
	U32 CellId;
	U16 UePMax;/*<=23dBm*/
	U16 EnodeBPMax;
}wrFLLmtToEnbSysArfcnCfg_T;/*射频前端配置参数*/

//3.4. 频点配置应答,O_FL_ENB_TO_LMT_SYS_ARFCN_ACK
typedef  struct wrFLEnbToLmtSysArfcnAck
{
	wrMsgHeader_t WrmsgHeaderInfo;	/*消息头定义*/
	U32 CfgResult;			/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtSysArfcnAck_t;

//3.5. 采集用户信息上报,O_FL_ENB_TO_LMT_UE_INFO_RPT
typedef struct wrFLEnbToLmtUeInfoRpt
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 UeIdType;/*0:IMSI 1:IMEI 2:BOTH*/
	U8 IMSI[C_MAX_IMSI_LEN]; /*IMSI*/
	U8 IMEI[C_MAX_IMEI_LEN];/*IMEI*/
	U8 Rssi; /* 信号强度 */
	U8 u8Res[2];/*补充字节*/
}wrFLEnbToLmtUeInfoRpt_t;

//3.6. 系统扫频配置,O_FL_LMT_TO_ENB_REM_CFG
typedef struct wrFLLmtToEnbRemcfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 wholeBandRem;/*频段*/
	U32 sysArfcnNum;/*默认为 1*/
	U32 sysArfcn[C_MAX_REM_ARFCN_NUM];/*频点,表示实际频点*10*/
}wrFLLmtToEnbRemcfg_t;

//3.7. 扫频信息上报,O_FL_ENB_TO_LMT_REM_INFO_RPT
typedef struct wrFLCellInfo
{
	U32 Arfcn;/* 频点，表示实际频点*10*/
	U16 pci;
	U16 Tac;
	U16 Rssi;
	U16 SFassign; /* 上下行子帧配置*/
	U32 cellid;
	U32 Priority;/* 本小区频点优先级*/
	U8 RSRP;
	U8 RSRQ;
	U8 Bandwidth;
	U8 SpecSFassign;/* 特殊子帧配置*/
#if 0
	U32 Arfcn;/*频点,表示实际频点*10*/
	U16 pci;
	U16 Tac;
	U16 Rssi;
	U16 spare1;
	U32 cellid;
	U32 Csgid; /*for CI*/
	U8  RSRP;
	U8  RSRQ;
	U8  Bandwidth;
	U8  spare;
#endif
}wrFLCellInfo_t;
/*ZMDW system REM report for neigh info*/
typedef struct wrFLNeighCellInfo
{
	U32 Arfcn; /*频点,表示实际频点*10*/
	U16 pci;
	U16 QoffsetCell;/*( QoffsetCell -24)表示实际 dB 值*/
}wrFLNeighCellInfo_t;
typedef struct wrFLInterNeighCellInfo
{
	U16 pci;/*PCI*/
	U16 QoffsetCell;/*( QoffsetCell -24)表示实际 dB 值*/
}wrFLInterNeighCellInfo_t;

typedef struct stFlLteIntreFreqLst
{
	U32 dlARFCN;
	U8 cellReselectPriotry;
	U8 Q_offsetFreq;
	U16 measBandWidth; /*允许测量带宽*/
	U32 interFreqNghNum;
	/* inter Freq Ngh Num*/
	wrFLInterNeighCellInfo_t stInterFreq[MAX_INTER_FREQ_NGH];
}stFlLteIntreFreqLst_t;
typedef struct wrFLCollectionCellInfo
{
	wrFLCellInfo_t stCellInfo;
	U32 NeighNum;
	wrFLNeighCellInfo_t stNeighCellInfo[C_MAX_INTRA_NEIGH_NUM];
	U32 NumOfInterFreq;
	stFlLteIntreFreqLst_t stInterFreqLstInfo[MAX_INTER_FREQ_LST];
}wrFLCollectionCellInfo_t;
/*FL system REM report*/
typedef struct wrFLEnbToLmtScanCellInfoRpt
{
	wrMsgHeader_t WrmsgHeaderInfo;/* 消息头定义 */
	U16 collectionCellNum; /* 采集小区数量 */
	U16 collectionEndFlag; /* 信息类型标识 0-扫频信息, 1-同步信息 */
	wrFLCollectionCellInfo_t stCollCellInfo[C_MAX_COLLTECTION_INTRA_CELL_NUM]; /* 基站采集信息 */
}wrFLEnbToLmtScanCellInfoRpt_t;

//3.8. 重启指示,O_FL_LMT_TO_ENB_REBOOT_CFG
typedef struct wrFLLmtToEnbRebootcfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
} wrFLLmtToEnbRebootcfg_t;

//3.10. 小区激活去激活配置,O_FL_LMT_TO_ENB_SET_ADMIN_STATE_CFG
typedef struct wrFLLmtToEnbSetAdminStateCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 workAdminState;/*工作状态:1:激活小区,0:去激活小区*/
}wrFLLmtToEnbSetAdminStateCfg_t;

//3.12. 上位机与基带板间心跳指示,O_FL_ENB_TO_LMT_SYS_INIT_SUCC_IND
//Heart Beat Request
typedef struct wrFLEnbToLmtSysInitInformInd
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
#if 0
	/* in 8.0 version */
	U16 CellState; /* 小区状态 */
	U16 Band; /* BAND */
	U32 ulEarfcn; /* 上行频点 */
	U32 dlEarfcn; /* 下行频点 */
	U8 PLMN[7]; /* PLMN */
	U8 Bandwidth; /* 系统带宽 */
	U16 PCI; /* 物理小区ID */
	U16 TAC; /* tac */
#endif

}wrFLEnbToLmtSysInitInformInd_t;

//3.13. 心跳应答,O_FL_LMT_TO_ENB_SYS_INIT_SUCC_RSP
//Heart Beat Response
typedef struct wrFLEnbToLmtSysInitInformRsp
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
}wrFLEnbToLmtSysInitInformRsp_t;

//3.14. 接收增益配置,O_FL_LMT_TO_ENB_SYS_RxGAIN_CFG
typedef struct wrFLLmtToEnbSysRxGainCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 Rxgain;/* 接收增益:0~127
		    * 注:表示将接收到的来自 UE 的信号放大多少倍
		    * 一般 TDD 默认填写 52,表示 52db;FDD 写 20i
		    * */
	U8 RxGainSaveFlag;/* 是否保存配置 */
	U8 Padding[3];/* 未知 */
}wrFLLmtToEnbSysRxGainCfg_t;

//3.16. 发射功率配置,O_FL_LMT_TO_ENB_SYS_PWR1_DEREASE_CFG
typedef struct wrFLLmtToEnbSysPwr1DegreeCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/* 注:衰减值每加 4,单板输出功率增加 1dB 衰减
	 * 无衰减时,即衰减值为 0x00 时,
	 * 单板输出功率大概在-1dbm~-2dbm.
	 * 例如,当衰减值设置为 0x04,输出功率大概在-2dbm~-3dbm;
	 * 当衰减值设置为 0x28,输出功率大概在-11dbm~12dbm.
	 * */
	U32 Pwr1Derease;/*功率衰减:0x00~FF*/
	U8 IsSave;
	/* 1: 动态生效,且保存衰减值到设备配置文件,动态生效且重启时也生效;
	 * 0: 只动态生效,设备重启不生效*/
	U8 Res[3]; /*保留字节*/
}wrFLLmtToEnbSysPwr1DegreeCfg_t;

//3.17. 发射功率配置应答,O_FL_ENB_TO_LMT_SYS_PWR1_DEREASE_ACK
typedef struct wrFLEnbToLmtSysPwr1DegreeAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtSysPwr1DegreeAck_t;

//3.18. 重定向信息配置,O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG
enum
{
	REDIR_TYPE_4G=0,
	REDIR_TYPE_3G=1,
	REDIR_TYPE_2G=2,
};
typedef struct wrFLLmtToEnbRedirectInfoCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 OnOff;/*是否开启重定向功能开关:0:打开,1:关闭*/
	U32 EARFCN;/*重定向的频点*/
	U32 RedirectType;/*0:4G;1:3G;2:2G*/
}wrFLLmtToEnbRedirectInfoCfg_t;

//3.20. 获取小区最后一次操作执行的状态请求,O_FL_LMT_TO_ENB_GET_ENB_STATE.
typedef struct wrFLLmtToEnbGetEnbState
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
}wrFLLmtToEnbGetEnbState_t;

//3.21. 基站状态实时上报,O_FL_ENB_TO_LMT_ENB_STATE_IND
typedef enum
{
	WR_FL_ENB_STATE_AIR_SYNC_SUCC =0,//空口同步成功
	WR_FL_ENB_STATE_AIR_SYNC_FAIL = 1,//空口同步失败
	WR_FL_ENB_STATE_GPS_SYNC_SUCC = 2,//GPS 同步成功
	WR_FL_ENB_STATE_GPS_SYNC_FAIL=3,//GPS 同步失败
	WR_FL_ENB_STATE_SCAN_SUCC=4, //扫频成功
	WR_FL_ENB_STATE_SCAN_FAIL=5, //扫频失败
	WR_FL_ENB_STATE_CELL_SETUP_SUCC=6, //小区建立成功
	WR_FL_ENB_STATE_CELL_SETUP_FAIL=7, //小区建立失败
	WR_FL_ENB_STATE_CELL_INACTIVE=8, //小区去激活
	WR_FL_ENB_STATE_AIR_SYNC_ON_GOING=9, //空口同步中
	WR_FL_ENB_STATE_GPS_SYNC_ON_GOING= 10, // GPS 同步中
	WR_FL_ENB_STATE_SCAN_ON_GOING = 11, //扫频中
	WR_FL_ENB_STATE_CELL_SETUP_ON_GOING = 12,//小区建立中
	WR_FL_ENB_STATE_INACTIVE_ON_GOING = 13, //小区去激活中
	WR_FL_ENB_STATE_INVALID = 0xFFFF, //无效状态
}WR_FL_ENB_STATE_t;

typedef struct wrFLEnbToLmtEnbStateInd
{
	wrMsgHeader_t WrmsgHeaderInfo;
	U32 u32FlEnbStateInd; /*WR_FL_ENB_STATE_t*/
}wrFLEnbToLmtEnbStateInd_t;

//3.22. IP 配置请求,O_FL_LMT_TO_ENB_IP_CFG
typedef struct wrFLLmtToEnbIpCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 eNBIPStr[50]; /*字符串,eg: “192.168.1.51#255.255.255.0#192.168.1.1#”*/
}wrFLLmtToEnbIpCfg_t;

//3.23. IP 配置应答,O_FL_ENB_TO_LMT_IP_CFG_ACK
typedef struct wrFLEnbToLmtIpCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtIpCfgAck_t;

//3.25. 复位指示应答,O_FL_ENB_TO_LMT_RESET_ACK
typedef struct wrFLEnbToLmtResetAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
} wrFLEnbToLmtResetAck_t;

//3.26. 设置基站系统时间请求,O_FL_LMT_TO_ENB_SET_SYS_TMR
typedef struct wrFLLmtToEnbSetSysTmr
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 TmrStr[20]; /*“2015.01.20-10:10:10”*/
} wrFLLmtToEnbSetSysTmr_t;

//3.27. 设置基站系统时间应答,O_FL_ENB_TO_LMT_SET_SYS_TMR_ACK
typedef struct wrFLEnbToLmtSetSysTmrAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtSetSysTmrAck_t;

//3.28. 设置小区选择接收最低门限,O_FL_LMT_TO_ENB_SET_QRXLEVMIN
typedef struct wrFLLmtToEnbSetQRxLevMin
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	S8 QRxLevMin; /*[-70,-22]*/
}wrFLLmtToEnbSetQRxLevMin_t;

//3.29. 设置小区选择接收最低门限应答,O_FL_ENB_TO_LMT_SET_QRXLEVMIN_ACK
typedef struct wrFLEnbToLmtSetQRxLevMinAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtSetQRxLevMinAck_t;

//3.30. 设置基站同步方式,O_FL_LMT_TO_ENB_REM_MODE_CFG
typedef enum syncMode
{
	AIR_SYNC_MODE = 0,
	GPS_SYNC_MODE = 1,
	SYNC_1588_MODE = 2
}syncMode_t;
typedef struct wrFLLmtToEnbRemModeCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/*同步方式:
	0:空口同步;
	1:GPS 同步;
	2:1588 同步(不支持*/
	U32 Remmode;/*同步方式:0~2*/
}wrFLLmtToEnbRemModeCfg_t;

//3.31. 设置基站同步方式应答,O_FL_ENB_TO_LMT_REM_MODE_CFG_ACK
typedef struct wrFLEnbToLmtRemModeCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtRemModeCfgAck_t;

//3.32. 主控板 IP 和端口配置,O_FL_LMT_TO_ENB_LMTIP_CFG
typedef struct wrFLLmtToLmtIpCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 LMTIPStr[32]; /*字符串,eg: “192.168.1.53#3345”*/
}wrFLLmtToEnbLmtIpCfg_t;

//3.33. 主控板 IP 和端口配置应答,O_FL_ENB_TO_LMT_LMTIP_CFG_ACK
typedef struct wrFLEnbToLmtLMTIpCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLEnbToLmtLMTIpCfgAck_t;

//3.34. 小区频点配置相关参数查询,O_FL_LMT_TO_ENB_GET_ARFCN
typedef struct wrFLEnbToLmtSysArfcnCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
}wrFLEnbToLmtSysArfcnCfg_t;

//3.35. 小区频点配置相关参数指示,O_FL_ENB_TO_LMT_ARFCN_IND

//3.36. GPS 同步偏移调节配置,O_FL_LMT_TO_ENB_GPS_PP1S_CFG
typedef struct wrFLgpsPp1sCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/* 注:band39,band40 需要进行 GPS 同步偏移量调节,一般做-700 微秒左右数据帧头偏移(正值说明时域相对
	原始值向后移动,负值说明是时域对应原始值向前移动)。Gpsppsls 值与微秒的对应关系如下:
	Bandwidth (5M)-->1 微秒*7.68
	Bandwidth (10M)-->1 微秒*15.36
	Bandwidth (20M)-->1 微秒*30.72*/
	S32 Gpsppsls;
}wrFLgpsPp1sCfg_t;

//3.37. GPS 同步偏移调节配置应答,O_FL_ENB_TO_LMT_GPS_PP1S_ACK
typedef struct wrFLgpsPp1sCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult;/*配置结果:0:成功>0:错误编号*/
}wrFLgpsPp1sCfgAck_t;

//3.38. 上电自动建小区配置,O_FL_LMT_TO_ENB_SELF_ACTIVE_CFG_PWR_ON
typedef struct wrFLLmtToEnbSelfActivePwrOnCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/*0:上电自动走建小区流程;
	  1:上电不自动走建小区流程*/
	U32 SelfActiveCfg;
}wrFLLmtToEnbSelfActivePwrOnCfg_t;

//3.40. 基站基本信息查询,O_FL_LMT_TO_ENB_BASE_INFO_QUERY,(0xF02B)
typedef struct wrFLLmtToEnbBaseInfoQuery
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/*查询信息的类型,
	0:设备型号
	1:硬件版本
	2:软件版本
	3:序列号
	4:MAC 地址*/
	U32 u32EnbBaseInfoType;
}wrFLLmtToEnbBaseInfoQuery_t;

//3.41. 基站基本信息查询应答,O_FL_ENB_TO_LMT_BASE_INFO_QUERY_ACK,(0xF02C)
typedef struct wrFLEnbToLmtBaseInfoQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/*查询信息的类型,0:设备型号 1:硬件版本 2:软件版本 3:序列号 4:MAC 地址*/
	U32 u32EnbBaseInfoType;
	U8 u8EnbbaseInfo[100]; /*上报格式为字符串,以’\0’结束*/
}wrFLEnbToLmtBaseInfoQueryAck_t;

//3.42. 基站同步信息查询,O_FL_LMT_TO_ENB_SYNC_INFO_QUERY,(0xF02D),use common Msg Body
typedef enum synMode
{
	GPS_SYNC_OK,
	SNIFF_SYNC_OK,
	SYNC_FAIL
}synMode_t;

//3.43. 基站同步信息查询应答,O_FL_ENB_TO_LMT_SYNC_INFO_QUERY_ACK,(0xF02E)
typedef struct wrFLEnbToLmtSyncInfoQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U16 u16RemMode;   /*同 步 类 型 0:sniffer;1:gps*/
	U16 u16SyncState; /*0:GPS 同步成功;1:空口同步成功,2:未同步*/
}wrFLEnbToLmtSyncInfoQueryAck_t;

//3.44. 小区状态信息查询,O_FL_LMT_TO_ENB_CELL_STATE_INFO_QUERY, (0xF02F),use common Msg Body

//3.45. 小区状态信息查询应答,O_FL_ENB_TO_LMT_CELL_STATE_INFO_QUERY_ACK,(0xF030)
enum
{
	IDLE_STATE = 0, //0:Idle 态
	REM_STATE = 1,  //1:REM 执行中
	CELL_READY_ACTIVE = 2, //2:小区激活中
	CELL_ACTIVE_YET = 3,   //3:小区已激活
	CELL_READY_DEACTIVE =4 //4:小区去激活中
};
typedef struct wrFLEnbToLmtCellStateInfoQureyAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/*  小区状态:
		0:Idle 态
		1:REM 执行中
		2:小区激活中
		3:小区已激活
		4:小区去激活中*/
	U32 u32CellState;
}wrFLEnbToLmtCellStateInfoQureyAck_t;

//3.47. 接收增益和发射功率查询应答,O_FL_ENB_TO_LMT_RXGAIN_POWER_DEREASE_QUERY_ACK
typedef struct wrFLEnbToLmtEnbRxGainPowerDereaseQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;
	U8 u8RxGainValueFromReg; //寄存器中的值,动态生效的值
	U8 u8RxGainValueFromMib; //数据库中的值,建小区的默认值
	U8 u8PowerDereaseValueFromReg; //寄存器中的值,动态生效的值
	U8 u8PowerDereaseValueFromMib; //数据库中的值,建小区的默认值
	U8 u8AgcFlag;/* AGC开关(0:关闭,1:打开) */
	U8 u8Padding[3];/* 保留 */
}wrFLEnbToLmtEnbRxGainPowerDereaseQueryAck_t;

//3.48. 基站 IP 查询,O_FL_LMT_TO_ENB_ENB_IP_QUERY,(0xF033),use common Msg Body

//3.49. 基站 IP 查询应答,O_FL_ENB_TO_LMT_ENB_IP_QUERY_ACK,(0xF034)
typedef struct wrFLEnbToLmtEnbIPQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 u8EnbIp[4]; //IPV4,整型
}wrFLEnbToLmtEnbIPQueryAck_t;

//3.51. 小区选择 QRxLevMin 查询应答,O_FL_ENB_TO_LMT_QRXLEVMIN_VALUE_QUERY_ACK
typedef struct wrFLEnbToLmtQrxlevminValueQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	S32 s32QrxlevminVlaue; //小区选择参数
}wrFLEnbToLmtQrxlevminValueQueryAck_t;

//3.52. 扫频参数配置查询,O_FL_LMT_TO_ENB_REM_CFG_QUERY,(0xF037),use common Msg Body

//3.53. 扫频参数配置查询应答,O_FL_ENB_TO_LMT_REM_CFG_QUERY_ACK,0xF038
typedef struct wrFLEnbToLmtRemCfgQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 wholeBandRem; /*是否开启全频段扫频 0:不开启1:开启*/
	U32 sysArfcnNum; /*扫频频点数目*/
	U32 sysArfcn[C_MAX_REM_ARFCN_NUM]; /*频点*/
}wrFLEnbToLmtRemCfgQueryAck_t;

//3.54. 重定向参数配置查询,O_FL_LMT_TO_ENB_REDIRECT_INFO_CFG_QUERY,(0xF03F), use common Msg Body

//3.55. 重定向参数配置查询应答,O_FL_ENB_TO_LMT_REDIRECT_INFO_CFG_QUERY_ACK,(0xF040)
typedef struct wrFLEnbToLmtRedirectInfoCfgQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 OnOff;  /*是否开启重定向功能开关:0:打开,1:关闭*/
	U32 EARFCN; /*重定向频点*/
	U32 RedirectType; /*重定向类型 0:4G;1:3G;2:2G*/
}wrFLEnbToLmtRedirectInfoCfgQueryAck_t;

//3.56. 上电小区自激活配置查询, use common Msg struct, MsgType: 0xF041

//3.57 上电小区自激活配置查询应答,O_FL_ENB_TO_LMT_SELF_ACTIVE_CFG_PWR_ON_QUERY_ACK
typedef struct wrFLEnbToLmtSelfActiveCfgPwrOnQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 SelfActiveCfg; /*0:上电自动走建小区流程;1:上电不自动走建小区流程*/
}wrFLEnbToLmtSelfActiveCfgPwrOnQueryAck_t;

/* 3.58 Log打印级别配置 O_FL_LMT_TO_ENB_SYS_LOG_LEVL_SET (0xF045) */
typedef struct wrFLSetSysLogLevelInfo
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 isSetStkLogLev;/* 是否设置StkLog级别 */
	U8 stkLogLevel; /* Log级别
			* 0:不打印 log
			* 1:critical
			* 2:error
			* 3:warning
			* 4:info
			* 5:debug
			* 6:Max_Lev 
			* */
	U8 isSetDbgLogLev; /* 是否设置dbglog级别 */
	U8 DbgLogLevel; /* log级别 
			* 0:不打印 log
			* 1:fatal 
			* 2:error
			* 3:warning
			* 4:info
			* 5:debug
			* 6:Max_Lev 
			* */
	U8 isSetOamLogLev; /* 是否配置Oamlog级别 */
	U8 oamLogLevel; /* log级别 
			 * 0:不打印 log
			 * 1:exception
			 * 2:call_stack
			 * 3:fatal
			 * 4:critical
			 * 5:warning
			 * 6:trace_info
			 * 7:trace_verbose
			 * 8:Max_lev
			 * */
	U8 Reserved[3];/* 预留 */

} wrFLSetSysLogLevelInfo_t;
/* 3.61. 设置 Log 打印级别应答 O_FL_ENB_TO_LMT_SYS_LOG_LEVL_SET_ACK (0xF046) */
typedef struct wrFLSetSysLogLevelInfoAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 isSetStkLogLevOk; /* 配置结果 */
	U8 isSetDbgLogLevOk; /* 0:配置失败 */
	U8 isSetOamLogLevOk; /* 1:配置成功 */
	U8 Reserved; /* 预留 */
} wrFLSetSysLogLevelInfoAck_t;

/* 3.63 O_FL_ENB_TO_LMT_SYS_LOG_LEVL_QUERY_ACK (0xF048)*/
typedef struct wrFLSysLogLevelInfoAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 queryResult; /*查询状态(0--失败,1--成功,成功后有系列信息) */
	U8 stkLogLevel; /* stkLog级别 */
	U8 dbgLogLevel; /* dbgLog级别 */
	U8 oamLogLev;   /* oamLog级别 */
} wrFLSysLogLevelInfoAck_t;


/* 常用配置状态回应结构体定义
 * 3.9. 重启指示应答,O_FL_ENB_TO_LMT_REBOOT_ACK(0xF00C)
 * 3.11. 小区激活去激活配置应答,O_FL_ENB_TO_LMT_SET_ADMIN_STATE_ACK(0xF00E)
 * 3.15. 接收增益配置应答,O_FL_ENB_TO_LMT_SYS_RxGAIN_ACK(0xF014)
 * 3.19. 重定向信息配置应答,O_FL_ENB_TO_LMT_REDIRECT_INFO_ACK(0xF018)
 * 3.39. 上电自动建小区配置应答,O_FL_ENB_TO_LMT_SELF_ACTIVE_CFG_PWR_ON_ACK(0xF03C)
 * 3.65. 设置TDD子帧配置应答,O_FL_ENB_TO_LMT_TDD_SUBFRAME_ASSIGNMENT_SET_ACK(0xF04A)
 * 3.17. 发射功率配置应答,O_FL_ENB_TO_LMT_SYS_PWR1_DEREASE_ACK(0xF016)
 * 3.71. 初始偏移配置应答,O_FL_ENB_TO_LMT_FREQ_OFFSET_CFG_ACK(0xF05A)
 * 3.78. Gps 信息复位配置应答,O_FL_ENB_TO_LMT_GPS_INFO_RESET_ACK(0xF06E) 
 * 3.86. 小区自配置请求应答,O_FL_LMT_TO_ENB_SELFCFG_CELLPARA_REQ_ACK(0xF050)
 * 3.90. 频点自配置频点添/删应答,0-成功,O_FL_ENB_TO_LMT_SELFCFG_ARFCN_CFG_REQ_ACK(0xF052)
 * 3.92. AGC 配置应答 O_FL_ENB_TO_LMT_AGC_SET_ACK(0xF07A)
 * 4.2. 测量 UE 配置应答,O_FL_ENB_TO_LMT_MEAS_UE_ACK (0xF007)
 * 7.4. 随机接入成功率清空请求响应 O_FL_ENB_TO_LMT_RA_ACCESS_EMPTY_REQ_ACK(0xF068)
 * 7.6 TAC 手动修改配置下发应答O_FL_ENB_TO_LMT_TAC_MODIFY_REQ_ACK(0xF06A) 
 * 7.8 辅 PLMN 列表配置应答O_FL_ENB_TO_LMT_SECONDARY_PLMNS_SET_ACK(0xF061)
 * */
typedef struct wrFLGenericConfigSetAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult; /* 3.9. 重启指示应答, 0-成功,>0-错误编号.
			* 3.11. 小区激活去激活配置应答, 0-成功,>0-错误编号.
			* 3.15. 接收增益配置应答, 0-成功,>0-错误编号.
			* 3.19. 重定向信息配置应答, 0-成功,>0-错误编号.
			* 3.39. 上电自动建小区配置应答, 0-成功,>0-错误编号.
			* 3.65. TDD子帧配置结果,0-成功,1-失败.
			* 3.17. 发射功率配置应答, 0-成功,>0-错误编号.
			* 3.69 初始偏移配置应答, 0-成功,>0-错误编号.
			* 3.78. Gps 信息复位配置应答,0-成功,>0-错误编号.
			* 3.86. 小区自配置请求应答,
			* 0-成功,1-失败,2-自配置后台频点列表中未含指定频段的频点.
			* 3.90. 频点自配置后台频点添加/删除应答,0-成功,
			* 1-添加频点重复,2-添加频点溢出,3-删除不存在的频点,4-频点值无效.
			* 3.92. AGC 配置应答, 0-成功,>0-错误编号.
			* 3.94.  小区频点动态修改应答
			* 3.102. IMEI  捕获功能配置应答
			* 4.2. 测量 UE 配置应答, 0-成功,>0-错误编号.
			* 7.4. 随机接入成功率清空请求响应,0-成功,1-失败.
			* 7.6 TAC 手动修改配置下发应答,0-成功,1-失败.
			* 7.8 辅 PLMN 列表配置应答,0-成功,1-含有无效值,配置失败.
			* */
} wrFLGenericConfigSetAck_t;

/* 通用配置以及查询命令结构体 */
typedef struct wrFLGenericConfigSet
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/

} wrFLGenericConfigSet_t;

//3.62. 设置 TDD 子帧配置(重启生效),O_FL_LMT_TO_ENB_TDD_SUBFRAME_ASSIGNMENT_SET (0xF049) 
//O_FL_ENB_TO_LMT_TDD_SUBFRAME_ASSIGNMENT_QUERY_ACK (0xF04C)
typedef struct wrFLLmtOrEnbSendYDDSubframeSetAndGet
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 u8TddSfAssignment; /* TDD子频 0~6 */
	U8 u8TddSpecialSfPatterns; /* TDD特殊模式 0~8 */
	U8 Reserved[2]; /* not know */
} wrFLLmtOrEnbSendYDDSubframeSetAndGet_t;

//3.66 位置区更新拒绝原因值配置,O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_CFG
typedef struct wrFLLmtToEnbTauAttachRejectCauseCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 RejectCause; //拒绝值设置:
	// 0:#cause15(追踪区不允许接入)
	// 1:#cause12(追踪区无合适小区)
	// 2:#cause3(无效终端)
	// 3:#cause13
	// 4:#cause22
} wrFLLmtToEnbTauAttachRejectCauseCfg_t;

// 3.67 位置更新拒绝原因值配置应答, O_FL_ENB_TO_LMT_TAU_ATTACH_REJECT_CAUSE_CFG_ACK
typedef struct wrFLEnbToLmtTauAttachRejectCauseCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult; //配置状况 1:配置失败  0:配置成功
} wrFLEnbToLmtTauAttachRejectCauseCfg_t;

// 3.68 初始偏移配置 O_FL_LMT_TO_ENB_FREQ_OFFSET_CFG (0xF059)
typedef struct wrFLLmtToEnbFreqOffsetCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 FreqOffset; /* 偏移值 */
}wrFLLmtToEnbFreqOffsetCfg_t;

//3.71 GPS经纬度高度查询, O_FL_LMT_TO_ENB_GPS_LOCATION_QUERY
typedef struct wrFLLmtToEnbGPSLocationQuery
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
} wrFLLmtToEnbGPSLocationQuery_t;
// 3.72 GPS经纬度高度查询应答, O_FL_ENB_TO_LMT_GPS_LOCATION_QUERY_ACK
typedef struct wrFLEnbToLmtGPSLocationQueryAck
{	/* F64 指 64 位浮点数 */
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 Paraoff1;  /* 无效 */
	F64 Longitude; /* 经度 */
	F64 Latitude;  /* 纬度 */
	F64 Altitude;  /* 高度 */
	U32 RateOfPro; /* GPS 经纬高度获取进度 0~100*/
	U32 Paraoff2;  /* 无效 */
} wrFLEnbToLmtGPSLocationQueryAck_t;

// 3.72. 告警指示 O_FL_ENB_TO_LMT_ALARMING_TYPE_IND (0xF05B)
typedef struct wrFLEnbToLmtAlarmTypeInd
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 AlarmingType; /* 告警类别:
		  	   * 0:高温告警>=70 度
		  	   * 1:失步告警
		  	   * 2:基站小区不可用告警
		  	   * 3:时钟源同步失败告警
		  	   * 4:驻波比过大告警
			   * 5:低温告警<=-20
			   */
	U32 IndFlag; /* 告警动作: 0:产生告警指示 1:取消告警指示 */
} wrFLEnbToLmtAlarmTypeInd_t;

// 3.73异频同步频点及 band 配置 O_FL_LMT_TO_ENB_SYNCARFCN_CFG 		(0xF05E)
typedef struct FLLmtToEnbSyncarfcnCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 SyncArfcn; /* 异频同步频点 */
	U32 SyncBand; /* 异频同步 band */
} FLLmtToEnbSyncarfcnCfg_t;
// 3.74异频同步频点及 band 配置 O_FL_LMT_TO_ENB_SYNCARFCN_CFG_ACK 	(0xF05F)
typedef struct FLLmtToEnbSyncarfcnCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 CfgResult; //配置状况 1:配置失败  0:配置成功
} FLLmtToEnbSyncarfcnCfgAck_t;

// 3.75 位置区域更新拒绝原因查询请求,O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_QUERY
// 只含有消息头 wrMsgHeader_t WrmsgHeaderInfo;
// 3.76 位置区域更新拒绝原因查询应答,O_FL_LMT_TO_ENB_TAU_ATTACH_REJECT_CAUSE_QUERY_ACK
typedef struct FLLmtToEnbTauAttachRejectCauseQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 U32RejectCause; /* 拒绝值设置 */
	// 0:#cause15(追踪区不允许接入)
	// 1:#cause12(追踪区无合适小区)
	// 2:#cause3(无效终端)
	// 3:#cause13
	// 4:#cause22
} FLLmtToEnbTauAttachRejectCauseQueryAck_t;

// 3.79基站版本升级配置O_FL_LMT_TO_ENB_UPDATE_SOFT_VERSION_CFG (0xF06F)
typedef struct wrFLLmtToEnbUpdateSoftVersionCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 updateType; /* 升级模式：0-基站 Image 版本, 1-uboot */
	U8 updateFileName[102]; /* 升级文件名称 */
	U8 isReservedCfg;  /* 是否保留配置:0不保留，1保留 */
	U8 isCfgFtpServer; /* 是否配置:0-不配置,1-配置 */
	U8 FtpServerIp[16];/* FTP服务器IP */
	U8 Reserved[3]; /* 预留 */ 
	U32 FtpServerPort; /* FTP服务器端口号 */
	U8 FtpLoginNam[20];/* 登录用户名 */
	U8 FtpPassword[10];/* 登录密码 */
	U8 FtpServerFilePath[66]; /* 升级文件保存路径(非中文,'/'结尾) */
} wrFLLmtToEnbUpdateSoftVersionCfg_t;

// 3.80基站版本升级配置应答O_FL_ENB_TO_LMT_UPDATE_SOFT_VERSION_CFG_ACK (0xF070)
typedef struct wrFLEnbToLmtUpdateSoftVersionCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 CfgResult; /* 配置结果 0-成功,>0-错误编号 */
	U8 failCause[23];/* 错误原因 */
} wrFLEnbToLmtUpdateSoftVersionCfgAck_t;

// 3.81. 获取基站Log O_FL_LMT_TO_ENB_GET_ENB_LOG (0xF071)
typedef struct wrFLLmtToEnbGetEnbLog
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 isCfgFtpServer;  /* 配置选项 0-不配置,1-配置 */
	U8 FtpServerIp[16]; /* FTP服务器IP */
	U8 Reserved[3];     /* 预留 */
	U32 FtpServerPort;  /* FTP服务器端口号 */
	U8 FtpLoginNam[20]; /* FTP登录用户名 */
	U8 FtpPassword[10]; /* FTP登录用户密码 */
	U8 FtpServerFilePath[66]; /* 上传文件保存目录(不支持中文,以'/'结尾) */
} wrFLLmtToEnbGetEnbLog_t;
// 3.82. 获取基站Log应答,O_ENB_TO_LMT_GET_ENB_LOG_ACK (0xF072)
typedef struct wrFLEnbToLmtFetEnbLogAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 CfgResult; /* 配置结果0-成功,1-失败 */
	U8 failCause[23];/* 失败原因 */
} wrFLEnbToLmtFetEnbLogAck_t;



//3.84. Gps 同步偏移量查询应答 O_FL_ENB_TO_LMT_GSP1PPS_QUERY_ACK (0xF074)
typedef struct FLEnbToLmtGps1ppsQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	S32 Gpsppsls; /* 偏移量 */
} FLEnbToLmtGps1ppsQueryAck_t;
// 3.85. 小区自配置请求 O_FL_LMT_TO_ENB_SELFCFG_CELLPARA_REQ (0xF04F)
typedef struct FLLmtToEnbSelfcfgCellparaReq
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 SelfBand; /* 指定频段 支持255*/

}FLLmtToEnbSelfcfgCellparaReq_t;
// 3.88. 频点自配置后台频点列表查询上报O_FL_ENB_TO_LMT_SELFCFG_ARFCN_QUERY_ACK(0xF04E)
typedef struct FLEnbToLmtSelfcfgArfcnQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 DefaultArfcnNum; /* 频点值系列表内数据个数 */
	U32 ArfcnValue[C_MAX_DEFAULT_ARFCN_NUM]; /* 频点值系列表 */

} FLEnbToLmtSelfcfgArfcnQueryAck_t;
// 3.89. 频点自配置后台频点添加/删除O_FL_LMT_TO_ENB_SELFCFG_ARFCN_CFG_REQ
typedef struct wrFLLmtToEnbSelfcfgArfcnCfgReq
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 Cfgtype; /* 动作类型 0-增加, 1-删除*/
	U32 ArfcnValue; /* 频点值 */
} wrFLLmtToEnbSelfcfgArfcnCfgReq_t;

// 3.91 AGC 配置 仅FDD有效 O_FL_LMT_TO_ENB_AGC_SET (0xF079)
typedef struct wrFLLmtToEnbAGCSet
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 AgcFlag;/* 0--不使能, 1--使能 */ 
} wrFLLmtToEnbAGCSet_t;


// 3.93.  小区频点动态修改
typedef struct wrFLLmtToEnbSysArfcnMod
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 sysUlARFCN;/*上行频点*/
	U32 sysDlARFCN;/*下行频点*/
	U8 PLMN[7]; /*plmn str, eg: “46001”*/
	U8 sysBand;
	U32 CellId;
	U32 UePMax;/*<=23dBm*/
}wrFLLmtToEnbSysArfcnMod_t;


#define MAX_REPORT_PERIOD_NUM 13
/* 终端测量值的上报周期枚举 */
typedef enum
{
	RPT_PERIOD_120MS = 0,
	RPT_PERIOD_240MS,
	RPT_PERIOD_480MS,
	RPT_PERIOD_640MS,
	RPT_PERIOD_1024MS, /* 4: 推荐值 */
	RPT_PERIOD_2048MS,
	RPT_PERIOD_5120MS,
	RPT_PERIOD_10240MS,
	RPT_PERIOD_1MIN,
	RPT_PERIOD_6MIN,
	RPT_PERIOD_12MIN,
	RPT_PERIOD_30MIN,
	RPT_PERIOD_60MIN,
}WrFLReportPeriod;
/* 基站测量UE配置, O_FL_LMT_TO_ENB_MEAS_UE_CFG
 * UE测量查询回应, O_FL_ENB_TO_LMT_MEAS_UE_CFG_QUERY_ACK
 * */
typedef struct wrFLLmtToEnbMeasUecfg
{
	wrMsgHeader_t WrmsgHeaderInfo;
	U8 u8WorkMode; /*工作模式*/
	/* 0:强干扰模式;1:周期抓号模式 2:定位模式 3:管控模式 4:重定向模式 */
	U8 u8SubMode; /* 保留1 */
	U16 u16CapturePeriod;     /* 周期模式参数, 针对同一个IMSI 的抓号周期 */
	/* 5~65535 单位:分钟 */
	U8 IMSI[C_MAX_IMSI_LEN];  /* 定位模式参数, 定位目标IMSI号 */
	/* string type */
	U8 u8MeasReportPeriod;   /* 定位模式参数,终端测量值的上报周期详见枚举 */
	U8 SchdUeMaxPowerTxFlag; /* 定位模式参数,定位终端最大功率发射开关 */
	/* 0: enable(Need)  * 1: disable */
	U8 SchdUeMaxPowerValue;   /* 定位模式参数, UE 最大发射功率,
				   * 最大值不超过wrFLLmtToEnbSysArfcnCfg
				   * 配置的 UePMax, 建议设置为 23dBm
				   * 0~23dBm
				   * */
	U8 SchdUeUlFixedPrbSwitch;/* 定位模式参数,
				   * 由于目前都采用 SRS 方案配合单兵,
				   * 因此此值需要设置为disable,
				   * 否则大用户量时有定位终端掉线概率.
				   * 0: disable(Need)
				   * 1: enable
				   */
	U8 CampOnAllowedFlag;     /* 定位模式参数,
				   * 是否把非定位终端踢回公网的配置,
				   * 建议设置为 0
				   * 0: 把非定位终端踢回公网
				   * 1: 非定位终端继续被本小区吸附
				   * */
	U8 res2[2]; /* 保留2 */
	U8 u8ControlSubMode;      /* 管控模式参数,
				   * 0:黑名单子模式
				   * 1:白名单子模式
				   * */
	U8 res3[3]; /* 保留3 */
}wrFLLmtToEnbMeasUecfg_t;

/* 定位UE测量值上报 O_FL_ENB_TO_LMT_MEAS_INFO_RPT */
typedef struct wrFLEnbToLmtMeasInfoRpt{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 UeMeasValue;/* 0..97 */
	U8 IMSI[C_MAX_IMSI_LEN]; /* IMSI */
	U16 TA;
} wrFLEnbToLmtMeasInfoRpt_t;

//4.3. UE 测量配置查询,O_FL_LMT_TO_ENB_MEAS_UE_CFG_QUERY,(0xF03D)
typedef struct wrFLLmtToEnbMeasUecfgQuery
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
}wrFLLmtToEnbMeasUecfgQuery_t;

//4.4. UE 测量配置查询应答,0xF03E
typedef struct wrFLEnbToLmtMeasUecfgQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 u8WorkMode; /*0:连续抓号模式,1:周期抓号模式,3:管控抓号模式*/
	U8 u8SubMode;  /*在周期抓号和管控抓号模式下有效。0:无干扰模式 1:重定向模式*/
	U16 u16CapturePeriod; /*capture period,只在周期抓号模式下有效*/
	U8 u8ControlSubMode; /*仅在管控抓号模式下有效 0:黑名单子模式 1:白名单子模式*/
}wrFLEnbToLmtMeasUecfgQueryAck_t;

//4.5. 管控名单配置,0xF039,O_FL_LMT_TO_ENB_CONTROL_UE_LIST_CFG
typedef struct wrFLLmtToEnbControlUeListCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/* 0: 在 管 控 名 单 中 删除用户
	 * 1: 在 管 控 名 单 中 添加用户
	 * */
	U8 ControlMovement;
	/*添加/删除 UE 数目*/
	U8 ControlUENum;
	/* 0:添加/删除黑名单用户
	 * 1:添加/删除白名单用户
	 * */
	U8 ControlUEProperty;
	/*UE标示,默认为IMSI.添加字符串,如:"460011111111111"非有效 UE ID 位注意置为'\0'(该字段为固定长度)*/
	U8 ControlUEIdentity[C_MAX_CONTROL_PROC_UE_NUM][C_MAX_IMSI_LEN];
}wrFLLmtToEnbControlUeListCfg_t;

//4.6. 管控名单配置应答,O_FL_ENB_TO_LMT_CONTROL_UE_LIST_CFG_ACK,0xF03A
typedef struct wrFLEnbToLmtControlUeListCfgAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/*3:管控名单配置中含有无效值;
	  2:存在用户未添加成功;
	  1:存在用户未删除成功;
	  0:配置成功*/
	U8 CfgResult;
	/*未操作成功 UE 数目*/
	U8 IgnoreUENum;
	U8 IgnoreUEList[C_MAX_CONTROL_PROC_UE_NUM][C_MAX_IMSI_LEN];
}wrFLEnbToLmtControlUeListCfgAck_t;

//4.7. 管控名单查询,O_FL_LMT_TO_ENB_CONTROL_LIST_QUERY
typedef struct wrFLLmtToEnbControlListQuery
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 ControlListType; /*0:查询黑名单 1:查询白名单*/
}wrFLLmtToEnbControlListQuery_t;
//4.7.19 NTP服务器IP配置
typedef struct wrFLLmtToEnbNtpServerCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 ntpServerIp[20]; /* NTP 服务器IP地址 eg:192.168.2.11 */
}wrFLLmtToEnbNtpServerCfg_t;
//4.7.20 定点重启配置请求
typedef struct wrFLLmtToEnbTimeToResetCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 ResetSwitch; //定点重启开关 0-关闭, 1-打开
	U8 Res[3];      //保留字节
	S8 ResetTime[12]; //重启时间(eg:23:55:55)
}wrFLLmtToEnbTimeToResetCfg_t;

//4.8. 管控名单查询应答,O_FL_ENB_TO_LMT_CONTROL_LIST _QUERY_ACK
typedef struct wrFLEnbToLmtControlListQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	/*0:黑名单
	  1:白名单
	  2:没有管控名单*/
	U8 ControlListProperty;
	/*管控名单中含有的UE 数目*/
	U8 ControlListUENum;
	/*UE 标示,内容为字符串(ASC 码),如:"460011111111111"非有效 UE ID 为'\0'(该字段为固定长度)*/
	U8 ControlListUEId[C_MAX_CONTROL_LIST_UE_NUM][C_MAX_IMSI_LEN];
} wrFLEnbToLmtControlListQueryAck_t;
//7.2 随机接入成功率上报 O_FL_ENB_TO_LMT_RA_ACCESS_QUERY_ACK(0xF066)
typedef struct wrFLEnbToLmtRaAccessQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 RrcConnReqNum; /* RRC 连接请求次数 */
	U32 RrcConnCmpNum; /* RRC 连接建立完成次数 */
	U32 Spare1;
	U32 Spare2;
} wrFLEnbToLmtRaAccessQueryAck_t;
//7.5TAC 手动修改配置下发 O_FL_LMT_TO_ENB_TAC_MODIFY_REQ(0xF069)
typedef struct wrFLLmtToEnbTACModifyReq
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 TacValue; /* TAC值 */
} wrFLLmtToEnbTACModifyReq_t;

// 7.7 辅 PLMN 列表配置 O_FL_LMT_TO_ENB_SECONDARY_PLMNS_SET(0xF060)
// 7.10 辅 PLMN 列表查询上报 O_FL_ENB_TO_LMT_SECONDARY_PLMNS_QUERY_ACK (0xF063)
typedef struct wrEnbToLmtSecondaryPlmnsQueryAck
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 u8SecPLMNNum;
	U8 u8SecPLMNList[5][7];
} wrEnbToLmtSecondaryPlmnsQueryAck_t;

/*3.99.扫频端口配置 O_FL_LMT_TO_ENB_REM_ANT_CFG(0xF07D)*/
typedef struct wrFLLmtToEnbRemAntCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 RxorSnf;     /*配置端口模式0:RX; 1:SNF*/
}wrFLLmtToEnbRemAntCfg_t;

/*3.101. IMEI  捕获功能配置 O_FL_LMT_TO_ENB_IMEI_REQUEST_CFG(0xF08A)*/
typedef struct wrFLLmtToEnbImeiEnableCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U8 ImeiEnable;     /*0 disable, 1 enable*/
	U8 reserved0;
	U8 reserved1;
	U8 reserved2;
}wrFLLmtToEnbImeiEnableCfg_t;

/*4.7.22 选频配置 O_FL_LMT_TO_ENB_SELECT_FREQ_CFG (0xF082),nyb,2017.12.22*/
typedef struct wrFLLmtToEnbSelectFreqCfg
{
	wrMsgHeader_t WrmsgHeaderInfo;	/*消息头定义*/
	U32 PinBandRelaNum;     /*指定结构体数组元素个数*/
	U8 pinBandRelaMap[15];	/*管脚频带关系表*/
	/*
	 *  Band   GPIO3  GPIO4   GPIO5  GPIO6
	 *  38,41  0 		0		0		1
	 *  39	   0		0		1		0
	 *  40     0		0 		1		1
	 *  8      1		0		0		0
	 *  1	   1		0		0		1
	 *  3,9    1		0		1		1
	 *  20	   1		1		0		0
	 *  5      1		1		0		1
	 *  7	   1		1		1		1
	 * */
}wrFLLmtToEnbSelectFreqCfg_t;


/* ++++++++++++++++++++++++++++++++++ */
#define RESTART_SIG1_GPIO (1)
#define RESTART_SIG2_GPIO (1 << 1)
#define RESTART_SIG3_GPIO (1 << 2)
#define RESTART_SIG4_GPIO (1 << 3)
#define RESTART_ALL_GPIO (RESTART_SIG1_GPIO | RESTART_SIG2_GPIO | RESTART_SIG3_GPIO | RESTART_SIG4_GPIO)
typedef struct switch_cmd_to_base_switch {
	wrMsgHeader_t WrmsgHeaderInfo;/*消息头定义*/
	U32 switch_cmd; /* 重启命令制定编号 */
	/*  1      重启1号
	 *  1 << 1 重启2号
	 *  1 << 2 重启3号
	 *  1 << 3 重启4号
	 *  */
}switch_cmd_to_base_switch_t;

/* 创建链表保存对应的频点信息 */
typedef struct deafult_band_config_struct {
	U32 sysUlARFCN;	/*上行频点*/
	U32 sysDlARFCN;	/*下行频点*/
	U8 PLMN[7]; 	/*plmn str, eg: “46001”*/
	U8 sysBandwidth; /*wrFLBandwidth*/
	U32 sysBand;	/*频段:Band38/band39/band40*/
	U16 PCI;		/*0~503*/
	U16 TAC;
	U32 CellId;
	U16 UePMax;/*<=23dBm*/
	U16 EnodeBPMax;
	//U8 Operator; /* 运行商 */
} def_band_config_t;
#pragma pack()
/* functions */
void bc_sta_json_obj_lock();
void bc_sta_json_obj_unlock();

cJSON *get_bc_sta_object();
cJSON *get_bc_sta_array();
void bc_sta_array_sum(cJSON *array);
S32 get_bc_sta_count();
void bc_sta_count_sum(S32 num);
void bc_sta_json_init();
void bc_sta_json_delete();
S32 baicell_send_msg_to_band(U8 *send_buffer, U32 send_len, S8 *ipaddr);
void send_generic_request_to_baseband(S8 *ip, U16 msgtype);
void send_reboot_request_to_baicell(S8 *ip);
//void baicell_pare_recv_value(U8 *recBuf, band_entry_t *entry);
/* boot in main */
void *thread_socket_to_baicell(void *arg);/* while recv */
void *thread_read_send_baicell_json_object(void *arg);/* timing send IMSI */


#endif /* __BAICELL_NET_H__ */
