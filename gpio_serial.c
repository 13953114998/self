/*
 * ============================================================================
 *
 *       Filename:  gpio_serial.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年06月05日 14时13分54秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>

#include "arg.h"
#include "gpio_ctrl.h"
#include "serialtest_sh.h"
#include "serialtest_ln.h"
#include "hash_list_active.h"

#include "config.h"
#include "gpio_serial.h"

static struct base_ipend_switch_gpio_pa base_pa_gpio[] =
{
/* {ipend, s_gpio, PA_addr} */
	{36, 0, 0},
	{37, 1, 0},
	{38, 2, 1}, /* +40 */
	{39, 3, 2},
	{40, 2, 1}, /*B40*/
	{202,4, 0}, /*192.168.178.202, PA address ignore, GSM,new add 2018.03.31*/
	{100,5, 0}  /*192.168.2.100,PA address ignore, wcdma, new add 2018.03.31*/
};

/* serial 发送请求给功放设备   */
S32 send_serial_request(U8 ma, int cmd_type)
{
	printf("PA,num %x,Pa Type(0--SH,1--LN) %d,cmd_type %x\n",
		ma, args_info.pa_cmp_arg, cmd_type);
	if (args_info.pa_cmp_arg == PA_VENDOR_SH) {
		return send_sh_serial_request(ma, cmd_type);
	}
	else if(args_info.pa_cmp_arg == PA_VENDOR_LN) {
		return send_ln_serial_request(ma, cmd_type);
	}
	else {
		write_action_log("unknown PA Type:", &args_info.pa_cmp_arg);
	}
	return -1;
}
/* 初始化串口与GPIO */
void init_serial_gpio_switch()
{
	/* serial 初始化功放操作的 fd */
	if (args_info.pa_cmp_arg == PA_VENDOR_SH)
	{
		/* 0-SH功放 */
		init_sh_serial_port();
		//record more log info to logfile, nyb, 2018.03.31
		write_action_log("SH PA:", "SH PA was initialized!");
	}
	else if(args_info.pa_cmp_arg == PA_VENDOR_LN)
	{
		/* 1-LN功放 */
		init_ln_serial_port();
		//record more log info to logfile, nyb, 2018.03.31
		write_action_log("LN PA:", "LN PA was initialized!");
	}
	else
	{
		/* other PA man */
		write_action_log("PA INFO:", "current PA vendor didn't support, please check and query from DM");
	}

	/* 初始化GPIO端口 */
	int gpio_count = get_gpio_count();

	//cfg IO model
	init_gpio_set_mode(gpio_count);

	//move GPIO to high level, here need to consider if we enable all port simultaneously, this maybe bring in some other issue
	for(int index = 0; index < gpio_count; index++) {
		switch_gpio(index, LEVEL_HIG);
		//sleep 1s to trigger GPIO level, nyb, 2018.03.31
		sleep(GPIO_ENABLE_INTERVAL);
	}
}
/* 通过基带板IP获取功放信息状态并发送 */
void *thread_pa_status(void *arg)
{
	band_entry_t *tpos, *n;
	sleep(3000);
	while (1) {
		if (!band_entry_count) continue;
		tpos = NULL;
		n = NULL;
		list_for_each_entry_safe(tpos, n, return_band_list_head(), node) {
			send_serial_request(tpos->pa_num,
				DATA_SEND_TYPE_GET_ANT_STATUS);
		}
		sleep(args_info.sendpa_itv_arg);
	} /* end while(... */
	return NULL;
}

void ctrl_pa_switch(int index, int act)
{
	switch_gpio(index, act);
}
/* 通过ip尾号获取基带连接功放地址 IP格式: xxx.xxx.xxx.xxx */
U8 get_base_pa_addr_by_ip(char *ip_addr)
{
	if (!ip_addr)
		return 0;
	S32 ip_tmp, ip_end, i;
	S32 gpio_count =
		sizeof(base_pa_gpio) / sizeof(base_ipend_switch_gpio_pa_t);
	sscanf(ip_addr, "%d.%d.%d.%d", &ip_tmp, &ip_tmp, &ip_tmp, &ip_end);
	for (i = 0; i < gpio_count; i++) {
		if (base_pa_gpio[i].ip_end == ip_end) {
			return base_pa_gpio[i].pa_addr;
		}
	}
	return 0;
}
/* 通过ip获取基带开关控制gpio标号 IP格式: xxx.xxx.xxx.xxx */
S32 get_base_switch_gpio_by_ip(char *ip_addr)
{
	if (!ip_addr)
		return 0;
	S32 ip_tmp, ip_end, i;
	S32 gpio_count =
		sizeof(base_pa_gpio) / sizeof(base_ipend_switch_gpio_pa_t);
	sscanf(ip_addr, "%d.%d.%d.%d", &ip_tmp, &ip_tmp, &ip_tmp, &ip_end);
	for (i = 0; i < gpio_count; i++) {
		if (base_pa_gpio[i].ip_end == ip_end) {
			return base_pa_gpio[i].s_gpio;
		}
	}
	return 0;
}


