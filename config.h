/*
 * =====================================================================================
 *
 *       Filename:  config.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/17/2018 06:47:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

typedef struct 
{
	/*tcp交互*/
	int tcp_port;		/*tcp 端口   default: */
	int max_listen;	/*tcp listet*/
	int max_event;		/*tcp epllo*/

	/*mqtt 交互*/
	char *userid;		/*mqtt 用户id		default:obd_pub */
	char *sub_msg;		/*mqtt 订阅内容		default: */
	char *mqtt_ip;		/*mqtt ip		default: */
	char *name;			/*server name   default: */
	char *possword;		/*server possword  default: */
	int mqtt_port;		/*mqtt port		default: 1883*/
	bool clean_session; /*mqtt 断开连接是否清理消息		default:false */

	/*http 交互*/
	char *http_url;     /*http 交互的url*/
	int http_timeout;	/*http 回收超时的时间*/
	char *http_dlameout;/*http 查询熄火点火*/
	char *http_online;  /*http 设备状态上报*/
	char *http_speed;	/*http 设备车速上报*/
	char *http_oil;		/*http 设备油耗上报*/
	char *http_gps;		/*http 设备经纬度上报*/
}Con_Info;

void Create_config();

#endif
