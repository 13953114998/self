/*
 * =====================================================================================
 *
 *       Filename:  cache_file.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年06月06日 09时26分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __CACHE_FILE_H__
#define __CACHE_FILE_H__
#define NETWORK_OK 1
#define NETWORK_NOK 0
#define NETWORK_TIME 15 /* 网络休眠时间，default为 20s * 15 */

#define CUTNET_CONFIG "/mnt/sd/cutnet_config.ini"
#define CUTNET_BRFORE "/mnt/sd/cutnet_before.ini"

/* 多线程 ，循环检测网络是否正常，若正常，则将文件上锁然后读取文件上传信息*/
int write_string_cachefile(char *write_string);
int write_string_gsmcachefile(char *write_string);
int check_cache_file();
void *thread_checknet_readfile();
int get_netstate();
FILE *get_cachefile_fp();
void cache_filelock_init();
void read_gsm_cachefile();
void init_cutnet_config();
void dev_sleep_state(int swth);

#endif /* __CACHE_FILE_H__ */
