/*
 * ============================================================================
 *
 *       Filename:  config.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/17/2018 07:30:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), lambert2478@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>

#include "typeda.h"

//Con_Info *con_info;

void Create_config()
{
	con_info = (Con_Info *)malloc(sizeof(Con_Info));
	if(!con_info)
	{
		printf("malloc con_info failure\n");
		return;
	}
	con_info->tcp_port = 35524;				/* tcp 端口		default: 5050*/
	con_info->max_listen = 1024;			/*tcp 最大监听数量		default:1024*/
	con_info->max_event = 1024;				/*epoll 处理事件最大数量	default:1024*/

	con_info->userid = strdup("obd_pub");  /* mqtt 订阅客户端id     default: obd_pub */
	con_info->sub_msg = strdup("obd_communication");    /* mqtt 订阅内容     default: obd_communication*/  
	con_info->mqtt_ip = strdup("127.0.0.1");   /* mqtt ip       default: */    
	//con_info->name = strdup("pub");        /* server name   default: pub*/
	//con_info->possword = strdup("123456");   /* server possword  default: 123456*/ 
	con_info->mqtt_port = 1883;  		 /*mqtt port     default: 1883*/
	con_info->clean_session = false;   /* mqtt 断开连接是否清理消息     default:false */   

	con_info->http_url = strdup("http://idmcs.nodepool.cn:6080/index.php/Webservice/V100");		/*与http交互的url    default:  */
	con_info->http_timeout = 2;			/*http 回收超时的时间   default:2 */
	con_info->http_dlameout = strdup("obd_device_flameout_save");	/*http 查询熄火点火信息		default: obd_device_flameout_save*/
	con_info->http_online = strdup("obd_device_state_save");		/*http 设备状态上报    default：obd_device_state_save*/
	con_info->http_speed = strdup("obd_device_speed_save");			/*http 设备车速上报    default: obd_device_speed_save*/
	con_info->http_oil = strdup("obd_device_oilwear_save");			/*http 油耗信息上报    default: obd_device_oilwear_save*/
	con_info->http_gps = strdup("obd_device_position_save");		/*http 设备gps位置信息上报  default:obd_device_position_save*/
}
