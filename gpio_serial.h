/*
 * =====================================================================================
 *
 *       Filename:  gpio_serial.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年06月05日 14时14分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __GPIO_SERIAL_H__
#define __GPIO_SERIAL_H__

#include "config.h"
#define GPIO_ENABLE_INTERVAL (1)   // GPIO ENABLE INTERVAL based on seconds

#pragma pack(1)
typedef struct base_ipend_switch_gpio_pa {
	S32 ip_end;
	S32 s_gpio;
	U8 pa_addr;
} base_ipend_switch_gpio_pa_t;
#pragma pack()

/* serial 发送请求给功放设备 */
S32 send_serial_request(U8 ma, int cmd_type);
/* 初始化串口与GPIO */
void init_serial_gpio_switch();
/* 通过基带板IP获取功放信息状态并发送 */
void *thread_pa_status(void *arg);
/* 通过编码找到GPIO编号并进行控制 */
void ctrl_pa_switch(int index, int act);

#endif /* __GPIO_SERIAL_H__ */

