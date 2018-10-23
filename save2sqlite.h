/*
 * =====================================================================================
 *
 *       Filename:  save2sqlite.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年06月22日 14时37分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __SAVE2SQLITE_H__
#define __SAVE2SQLITE_H__

#include <sqlite3.h>

typedef struct save_to_sqlite3_imsi_info {
	char this_time[20];
	char imsi[16];
	char imei[16];
	char dev_ip[16];
	char type[12];
}_imsi_st;

/* 为sqlite_db_lock 上锁 */
void pthread_sqlite_db_lock();
/* 为sqlite_db_lock 解锁 */
void pthread_sqlite_db_unlock();
/* 更新bak_sqlite3_db */
void set_bak_sqlite3_db(sqlite3 *db);
/* 获取bak_sqlite3_db */
sqlite3 *get_bak_sqlite3_db();
/* 更新sqlite3_db */
void set_sqlite3_db(sqlite3 *db);
/* 获取sqlite3_db */
sqlite3 *get_sqlite3_db();
void save2sqlite_runcmd(sqlite3 *db, char *cmd);
/* 添加数据 */
void save2sqlite_insert(_imsi_st s_point);
/* 读取数据库初始化 
 *如果创建成功，设置sqlite3_db 否则不做处理
 * */
sqlite3 *save2sqlite_init( char *dbfile_name);

void *thread_check_dbdirectory_file(void *arg);
/* 初始化 数据库all.db */
void init_dbfile_save_imsi();

#endif /* __SAVE2SQLITE_H__ */
