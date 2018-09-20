/*
 * =====================================================================================
 *
 *       Filename:  typeda.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/13/2018 05: 59:09 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _TYPEDA_H_
#define _TYPEDA_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <pthread.h>
#include <mosquitto.h>
#include <errno.h>

#include "cJSON.h"
#include "list.h"
#include "devic_list.h"
#include "mqtt.h"
#include "tcp_action.h"
#include "config.h"
#include "http.h"


Con_Info *con_info;

/* 与mk6交互的命令类型 */
#define M_CURRENCY		  0x00 	/*通用命令*/
#define M_GSMMESSAGE 	  0x10 	/*GSM命令*/
#define M_GPSMESSAGE  	0x20 	/*GPS命令*/
#define M_OBDMESSAGE 	  0x30 	/*OBD命令*/
#define M_UPGRADE		    0x40 	/*升级命令*/
#define M_EXTERNAL		  0x50 	/*外设命令*/


/*与服务器交互数据类型*/
#define T_GET_GPS_POSITION_YES			0x00	/*查询GPS位置信息*/
#define T_GET_CAT_AVERAGE_OIL			  0x01	/*查询汽车的平均油耗*/
#define T_GET_CAT_DLAMEOUT				  0x02	/*查询点火熄火信息*/
#define T_GET_CAT_SPEED					    0x03	/*查询速度信息*/
#define T_PUT_DEV_ONLINE_MSG			  0x04	/*设备上线离线上报*/



/*与设备交互数据类型*/
/*通用命令  0x00*/
/*GSM命令   0x10*/
#define G_GSM_HEART_BEAT    				0x88	/*GSM心跳包*/
/*GPS命令   0x20*/
#define G_GPS_OBD_MESSAGE		    		0x84	/*GPS+OBD混合信息上传*/
#define G_GPS_MSG_POSITION_NO		  	0x82	/*熄火GPS位置信息回复*/
#define G_GPS_MSG_POSITION_YES			0x96	/*点火GPS位置信息回复*/
/*OBD命令   0x30*/
#define O_OBD_OIL_CONSUMPTION			  0x8B	/*平均油耗信息回复*/
#define O_OBD_IGNITE_REPORT			  	0x89	/*上传点火熄火报告*/
/*升级命令  0x40*/
/*外设命令  0x50*/

#if 0
#define T_GET_MESSAGE_TO_DEV_FACTURE	0x000100	/*获取设备制造商*/
#define T_PUT_MSG_SWER_TO_DEV_FACTURE	0x0081		/*获取设备制造商应答*/
#define T_GET_MESSAGE_TO_SOFE_VERSION	0x000200	/*获取设备软件版本*/
#define T_PUT_MSG_SWER_TO_SOFE_VERSION	0x0082		/*获取软件版本号应答*/
#define T_GET_MESSAGE_TO_DEV_ID			0x000300	/*获取设备的ID号*/
#define T_PUT_MSG_SWER_TODEV_ID			0x0083		/*获取设备ID号应答*/
#define T_SET_MESSAGE_REDUCTION			0x000401	/*恢复出厂设置*/
#define T_PUT_MSG_SWER_REDUCTION		0x0084		/*恢复出厂设置应答*/
#define T_SET_MESSAGE_RESTART			0x000501	/*重新启动*/
#define T_PUT_MSG_SWER_RESTART			0x0085		/*重新启动应答*/
#define T_GET_MESSAGE_IS_FRUGAL			0x000600	/*查询是否开启省电模式*/
#define T_SET_MESSAGE_IS_FRUGAL			0x000601	/*设置是否开启省电模式*/
#define T_PUT_MSG_SWER_IS_FRUGAL		0x0086		/*查询是否开启省电模式应答*/
#define T_GET_MESSAGE_JET_LAG			0x000700	/*查询时差*/
#define T_SET_MESSAGE_JET_LAG			0x000701	/*设置时差*/
#define T_PUT_MSG_SWER_JET_LAG			0x0087		/*查询时差应答*/
#define T_PUT_MSG_IS_POLICE_TIME		0x008809	/*设备报警  不带GPS信息，只带时间信息*/
#define T_PUT_MSG_IS_POLICE_GPS			0x008812	/*设备报警  带GPS信息，也带时间信息*/
#define T_GET_MESSAGE_IS_POLICE			0x0008		/*报警确认*/
#define T_GET_MESSAGE_SEND_POLICE		0x000900	/*查询报警发送机制*/
#define T_SET_MESSAGE_SEND_POLICE		0x000901	/*设置报警发送机制*/
#define T_PUT_MSG_SWER_SEND_POLICE		0x0089		/*报警发送机制回复*/
#define T_GET_MESSAGE_MATER_POLICE		0x000A00	/*查询报警参数*/
#define T_SET_MESSAGE_MATER_POLICE		0x000A01	/*设置报警参数*/
#define T_PUT_MSG_SWER_MATER_POLICE		0x008A		/*报警参数应答*/
#define T_GET_MESSAGE_DEV_ALIAS			0x000B00	/*查询设备的别名*/
#define T_SET_MESSAGE_DEV_ALIAS			0x000B01	/*设置设备的别名*/
#define T_PUT_MSG_SWER_DEV_ALIAS		0x008B		/*设备别名应答*/
#define T_GET_MESSAGE_DEV_MILEAGE		0x000D00	/*查询里程*/
#define T_SET_MESSAGE_DEV_MILEAGE		0x000D01	/*标定原始里程*/
#define T_PUT_MES_SWER_DEV_MILEAGE		0x008D		/*查询里程应答*/
#define T_GET_GPS_MESSAGE_TIME			0x200100	/*查询GPS位置信息上传通道和间隔*/
#define T_SET_GPS_MESSAGE_TIME			0x200101	/*设置GPS位置信息上传通道和间隔*/
#define T_PUT_GPS_SWER_TIME				0x2081		/*GPS位置信息上传通道和间隔应答*/
#define T_GET_GPS_POSITION_NO			0x200200	/*查询GPS位置信息(熄火上传)*/
#define T_PUT_GPS_POSITION_NO			0x2082		/*GPS位置回复(熄火)*/
#define T_GET_GPS_AND_OBD_TIME			0x200300	/*查询GPS+OBD混合信息上传间隔*/
#define T_SET_GPS_AND_OBD_TIME			0x200301	/*设置GPS+OBD混合信息上传间隔*/
#define T_PUT_GPSOBD_SWER_TIME			0x2083		/*查询GPS+OBD混合信息上传应答*/
#define T_GET_GPS_AND_OBD_MSG			0x200400	/*查询GPS+OBD混合信息*/
#define T_PUT_GPSOBD_SWER_MSG			0x2084		/*上传GPS+OBD混合信息*/
#define T_GET_GPS_AND_OBD_SEND			0x200500	/*查询GPS+OBD信息的上传方式*/
#define T_SET_GPS_AND_OBD_SEND			0x200501	/*设置GPS+OBD信息的上传方式*/
#define T_PUT_GPSOBD_SWER_UPLOAD		0x2085		/*GPS+OBD信息上传方式应答*/
#define T_GET_GPS_POSITION_YES			0x200600	/*查询GPS位置信息(点火上传)*/
#define T_PUT_GPSOBD_SWER_POSI_YES		0x2086		/*GPS位置回复(点火)*/
#define T_GET_CAT_AVERAGE_OIL			0x300B00	/*查询汽车的平均油耗*/
#define T_SET_CAT_AVERAGE_OIL			0x300B01	/*设置汽车的平均油耗*/
#define T_PUT_CAT_SWER_AVE_OIL			0x308B		/*汽车的平均油耗回复*/
#define T_PUT_MSG_CAT_TPIP				0x308E		/*定时上传行程报告*/
#define T_GET_MSG_CAT_TRIP_TIME			0x300D00	/*查询该次行程报告上传的时间间隔*/
#define T_SET_MSG_CAT_TRIP_TIME			0x300D01	/*设置该次行程报告上传的时间间隔*/
#define T_PUT_MSG_SWER_TRIP_TIME		0x308D		/*该次行程报告上传时间间隔回复*/
#define T_PUT_IS_FLAMEOUT				0x308909	/*点火熄火报告(不带GPS)*/
#define T_PUT_IS_FLAMEOUT_GPS			0x308912	/*点火熄火报告(GPS)*/
#endif

void fun_error(const char *str);
void safe_pthread_create(void *(*start_routine)(void *), void *arg);
void pthread_lock_init();
void *p_monitor_online();


#endif
