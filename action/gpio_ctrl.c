/*
 * ============================================================================
 *
 *       Filename:  gpio_ctrl.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2017年07月15日 09时21分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:
 *
 * ============================================================================
 */

/************************************************/
/*-----------sf701 change list-------------*/
//091108, w39v04 fireware flash programme;
/************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/io.h>

#include "config.h"
#include "gpio_ctrl.h"

#define port_mask(x)  (1<<(x))

/* inb/outb:读/写8位端口 */
#define inportb(port)	inb(port)
#define outportb(port,data) outb(data,port)
/* inl/outl:读/写32位端口 */
#define inport(port) inl(port)
#define outport(port,data) outl(data,port)

/* Step 1: modify gpio number with hardware order */
/* GPIO num: 3, 4, 5, 6, 7, 8, 9,10 */
static int port[] = {35, 25, 24, 29, 34, 42, 31, 43};
static int offset[] = {0xA0, 0x80, 0x80, 0x80, 0xA0, 0xA0, 0x80, 0xA0};
static unsigned int GPIO_BASE = 0x500;

static int set_lvl(int index, int lvl);
static void set_io_mode(int index,int mode);
static void gpio_init(int gpio_num);
/* 获得 GPIO 数量 */
int get_gpio_count()
{
	return sizeof(port) / sizeof(port[0]);
}

/* 保存当前配置的结果 */
static unsigned int save_status_80; /* 0x80 offset */
static unsigned int save_status_a8; /* 0xA8 offset */
/*
 * 操作GPIO
 * index: GPIO数组的下标
 * lvl: 动作 LEVEL_HIG--拉高, LEVEL_LOW--拉低
 */

int set_lvl(int index,int lvl)
{
	int group,shift;
	unsigned int mask;
	group=port[index] / 32;
	shift=port[index] % 32;
	mask=port_mask(shift);

	if (offset[index] == 0x80) {
		if(lvl == LEVEL_LOW) {
			save_status_80 &= (~mask);
		} else {
			save_status_80 |= mask;
		}
		outport(GPIO_BASE + offset[index] + 0x08, save_status_80);
	} else if (offset[index] == 0xA0) {
		if(lvl == LEVEL_LOW) {
			save_status_a8 &= (~mask);
		} else {
			save_status_a8 |= mask;
		}
		outport(GPIO_BASE + offset[index] + 0x08, save_status_a8);
	}
	return 0;
}

/*
 * GPIO 初始化
 * gpio_num: GPIO数量
 */
static void gpio_init(int gpio_num)
{
	int index;
	int shift = 0;
	/* step2:modify pin_num refer to MB hardware */
	for(index = 0; index < gpio_num; index++) {
		if(port[index] > 31) {
			shift = port[index] - 32;
		} else {
			shift = port[index];
		}
		//!todo:nyb, 2018.04.02,here we need to consider if all gpio was initialized as low level, and then trigger it to high in following procedure
#if 1
		//move all needed GPIO to high default
		outport(GPIO_BASE + offset[index],
			(inport(GPIO_BASE + offset[index]) | (1 << shift)));
#else
		//move all needed GPIO to lower default and then move them too high as sequence
		outport(GPIO_BASE + offset[index],
			(inport(GPIO_BASE + offset[index]) | (0 << shift)));
#endif
		sleep(0.1);
	}
}

/*
 * 设置I/O的模式
 * index: GPIO数组的下标
 * mode: 模式: IO_MODE_IN--输入模式 IO_MODE_OUT--输出模式
 */
static void set_io_mode(int index, int mode)
{
	int shift = 0;
	if(port[index] > 31)
		shift = port[index] - 32;
	else
		shift = port[index];
	if(mode == IO_MODE_IN) {
		outport(GPIO_BASE + offset[index] + 0x04,
			(inport(GPIO_BASE + offset[index] + 0x04) | (1<<shift)));
	} else {
		outport(GPIO_BASE+offset[index] + 0x04,
			(inport(GPIO_BASE + offset[index] + 0x04) & ~(1<<shift)));
	}
}

/*
 * 拉高拉低或者重置GPIO端口
 * index: gpio数组下标
 * act: 动作(0--拉低, 1--拉高, 2--重置)
 * modify: move sleep interval from 1 seconds to 2 seconds
 * Date:   2018.04.02
 * Author: nyb
 */
void switch_gpio(int index, int act)
{
	sleep(1);
	if (act == LEVEL_RES) {
		set_lvl(index, LEVEL_LOW);
		sleep(2);
		set_lvl(index, LEVEL_HIG);
	} else {
		set_lvl(index, act);
	}
}

/*
 * 初始化并设置GPIO模式
 * gpio_num: GPIO的数量
 */
int  init_gpio_set_mode(int gpio_num)
{
	int index;
	/* 修改io权限 */
	if(iopl(3) < 0) {
		perror("iopl");
		write_action_log("@nyb-init_gpio_set_mode:", "change IO privilege failed");
		return 0;
	}

	/* GPIO 初始化(全部拉高) */
	gpio_init(gpio_num);

	/* 测试GPIO并修改模式为输出模式 */
	for(index = 0; index < gpio_num; index++) {
		set_io_mode(index, IO_MODE_OUT);
		sleep(0.2);
	}
	write_action_log("@nyb-init_gpio_set_mode:", "cfg GPIO IO model successfully");

	return 1;
}

