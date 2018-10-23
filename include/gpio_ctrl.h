/*
 * =====================================================================================
 *
 *       Filename:  gpio_ctrl.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年07月15日 09时24分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "config.h"
#ifdef SYS86
#ifndef __GPIO_CTRL_H__
#define __GPIO_CTRL_H__

#define IO_MODE_IN   (0)
#define IO_MODE_OUT  (1)

#define LEVEL_RES	(2)
#define LEVEL_HIG	(1)
#define LEVEL_LOW	(0)

/* 获得 GPIO 数量 */
int get_gpio_count();
/*
 * 拉高拉低或者重置GPIO端口
 * index: gpio数组下标
 * act: 动作(0--拉低, 1--拉高, 2--重置)
 */
void switch_gpio(int index, int act);

/*
 * 初始化并设置GPIO模式
 * gpio_num: GPIO的数量
 */
int init_gpio_set_mode(int gpio_num);
#endif /* __GPIO_CTRL_H__ */
#endif /* SYS86 */
