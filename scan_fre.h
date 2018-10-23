/*
 * =====================================================================================
 *
 *       Filename:  scan_fre.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年06月09日 14时37分48秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __SCAN_FRE_H__
#define __SCAN_FRE_H__

#define SYSARFCN_DEF_NUM 10
#define SCAN_FRE_FILE "/mnt/sd/scan_info/scan_flag"

/*小区激活去激活配置（0：去激活；1：激活）*/
int cell_up_down_config(band_entry_t *entry, U32 state);
/*辅PLMN配置*/
int assist_plmn_config(band_entry_t *entry,U8 cfgMode);
/*配置扫频端口（0：RX；1：SNF）*/
int scan_fre_com_config(band_entry_t *entry, U32 state);
/*开启扫频*/
int start_scan_fre(band_entry_t *entry);
/*每过一段时间，对基带板进行手动扫频，并上报
* 1.小区去激活(0)
* 2.配置扫频端口为SNF(1)
* 3.进行扫频
* 4.配置扫频端口为RX(0)
* 5.小区激活(1)
*/
void *thread_scan_frequency(void *arg);

#endif /* __SCAN_FRE_H__ */
