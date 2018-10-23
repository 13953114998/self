/*
 * =====================================================================================
 *
 *       Filename:  udp_wifi.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年03月30日 10时19分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __UDP_WIFI_H__
#define __UDP_WIFI_H__
void *thread_recv_wifi_and_send(void *arg);
void *thread_send_wifi_json_object(void *arg);

#endif /* __UDP_WIFI_H__ */
