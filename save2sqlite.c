/*
 * ============================================================================
 *
 *       Filename:  save2sqlite.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年06月22日 14时37分20秒
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
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "arg.h"
#include "config.h"
#include "save2sqlite.h"

static char last_dirname[128] = {0};
static char now_dirname[128] = {0};
static int last_hour;
static sqlite3 *sqlite3_db = NULL, *bak_sqlite3_db = NULL;
static pthread_mutex_t sqlite_db_lock;
void pthread_sqlite_db_lock_init()
{
	pthread_mutex_init(&sqlite_db_lock, NULL); //初始化数据库访问锁
}
void pthread_sqlite_db_lock()
{
	pthread_mutex_lock(&sqlite_db_lock);
}
void pthread_sqlite_db_unlock()
{
	pthread_mutex_unlock(&sqlite_db_lock);
}

/* 操作数据库 call bak 函数 */
static int action_sqlite_cb(void *NotUsed, int argc, char **argv, char **azColName)
{
	return 0;
	int i;
	for(i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

/* 更新bak_sqlite3_db */
void set_bak_sqlite3_db(sqlite3 *db)
{
	bak_sqlite3_db = db;
}

/* 获取bak_sqlite3_db  获取前要上锁 */
sqlite3 *get_bak_sqlite3_db()
{
	return bak_sqlite3_db;
}
/* 更新sqlite3_db */
void set_sqlite3_db(sqlite3 *db)
{
	/* 修改sqlite3_db 必须先上锁 */
	pthread_sqlite_db_lock();
	if (sqlite3_db != NULL) { //先检测是否已经被打开，若被打开，先关闭在赋值
		sqlite3_close(sqlite3_db);
		sqlite3_db = NULL;
	}
	sqlite3_db = db;
	pthread_sqlite_db_unlock();
}

/* 获取sqlite3_db  获取前要上锁 */
sqlite3 *get_sqlite3_db()
{
	return sqlite3_db;
}

void save2sqlite_runcmd(sqlite3 *db, char *cmd)
{
	char *zerrmsg = NULL;
	int ret = 0;
	ret = sqlite3_exec(db, cmd, action_sqlite_cb, 0, &zerrmsg);
	if (ret != SQLITE_OK) {
		printf("SQL error: %s\n", zerrmsg);
	}
	if (zerrmsg)
		sqlite3_free(zerrmsg);
}
/* 往数据库中插入一条数据 */
void _save2sqlite_insert(sqlite3 *db, _imsi_st s_point)
{
	char insert_sqlite_cmd[256] = {0};
	if (!(s_point.imsi)) return;
	if (strlen(s_point.imei) > 0) {
		snprintf(insert_sqlite_cmd, 256,
			"INSERT INTO %s "
			"(probeIP,IMSI,IMEI,time,type)"
			" VALUES ('%s','%s','%s','%s','%s');"
			,
			args_info.sqltablename_arg,
			s_point.dev_ip,	s_point.imsi,
			s_point.imei, s_point.this_time, s_point.type);
	} else {
		snprintf(insert_sqlite_cmd, 256,
			"INSERT INTO %s "
			"(probeIP,IMSI,time,type)"
			" VALUES ('%s','%s','%s','%s');"
			,
			args_info.sqltablename_arg,
			s_point.dev_ip, s_point.imsi, s_point.this_time, s_point.type);
	}
	save2sqlite_runcmd(db, insert_sqlite_cmd);
}
/* 往数据库中插入一条数据 */
void save2sqlite_insert(_imsi_st s_point)
{
	_save2sqlite_insert(get_sqlite3_db(), s_point);
	_save2sqlite_insert(get_bak_sqlite3_db(), s_point);
}

/* 读取数据库初始化 
 *如果创建成功，设置sqlite3_db 否则不做处理
 * */
sqlite3 *save2sqlite_init( char *dbfile_name)
{
	printf("we will open db file:%s\n", dbfile_name);
	int ret;
	sqlite3 *new_db;
	char *zerrmsg = NULL;
	/* 打开或者创建数据库 */
	ret = sqlite3_open(dbfile_name, &new_db);
	if (ret) {
		fprintf(stderr, "Can't open database: %s\n",
			sqlite3_errmsg(new_db));
		return NULL;
	}
	fprintf(stderr, "Opened database successfully\n");
	/* 创建表的命令 */
	char creat_table_cmd[256] = {0};
	snprintf(creat_table_cmd, 256,
		"CREATE TABLE %s("
		"ID INTEGER PRIMARY KEY AUTOINCREMENT  NOT NULL,"
		"probeIP     TEXT     NOT NULL,"
		"IMSI    CHAR(20),"
		"IMEI    CHAR(20),"
		"time    CHAR(20),"
		"type    char(3));"
		,
		args_info.sqltablename_arg);

	/* 创建数据表并设置字段信息 */
	ret = sqlite3_exec(new_db, creat_table_cmd, action_sqlite_cb, 0, &zerrmsg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zerrmsg);
	} else {
		fprintf(stdout, "Table created successfully\n");
	}
	if (zerrmsg)
		sqlite3_free(zerrmsg);
	return new_db;
}
/* 初始化 数据库all.db */
void init_dbfile_save_imsi()
{
	sqlite3 *sqldb = save2sqlite_init(args_info.baksqlname_arg);
	if (sqldb == NULL) {
		perror("init databases that save all imsi failed!\n");
		exit(1);
	}
	set_bak_sqlite3_db(sqldb);
}

/* 线程函数，定时检测文件夹以及数据库文件 */
/* 每天一个文件夹,文件夹中每小时一个文件 */
void *thread_check_dbdirectory_file(void *arg)
{
	time_t rawtime;
	struct tm * timeinfo;
	char time_dirname[10] = {0};
	pthread_sqlite_db_lock_init(); //初始化数据库访问锁
	char db_filename[160] = {0};
	while(1) {
		memset(now_dirname, 0, 128);
		strcat(now_dirname, args_info.dbpath_arg);
		if (now_dirname[strlen(now_dirname) - 1] != '/') {
			strcat(now_dirname, "/");
		}
		rawtime = time(NULL);
		timeinfo = localtime(&rawtime);
		snprintf(time_dirname, 10, "%04d%02d%02d",
			timeinfo->tm_year + 1900,
			timeinfo->tm_mon + 1,
			timeinfo->tm_mday);
		strcat(now_dirname, time_dirname); //目录全称
		/* 如果现在的文件夹名称与原来的文件名称不同(说明过去了一天),
		 * 需要新建文件夹,然后将当前的小时作为db文件的文件名进行初始化 
		 */
		/* 根据时间来确定文件夹 */
		if (strcmp(last_dirname, now_dirname) != 0) {
			create_directory(now_dirname); /* 创建文件夹 */
			memcpy(last_dirname, now_dirname, 128); /* 同步last_dirname */
			last_hour = timeinfo->tm_hour;
			memset(db_filename, 0, 160);
			snprintf(db_filename, 160, "%s/%02d.db",
				now_dirname, timeinfo->tm_hour);
			/* eg: db_filename: /mnt/sd/20170622/09.db */
			/* 关闭旧的数据库文件，初始化新的数据库文件 */
			sqlite3 *sqldb =  save2sqlite_init(db_filename);
			if (sqldb) {
				set_sqlite3_db(sqldb);
			} else {
				printf("sqlite db(name:%s) init error!"
					"use old sqlite db!\n",
					db_filename);
			}
		} else {//文件夹相同，说明是同一天
			/* 小时不一样 */
			if (last_hour != timeinfo->tm_hour) {
				printf("creat db file!\n");
				memset(db_filename, 0, 160);
				snprintf(db_filename, 160, "%s/%02d.db",
					now_dirname, timeinfo->tm_hour);
				/* 关闭旧的数据库文件，初始化新的数据库文件 */
				sqlite3 *sqldb =  save2sqlite_init(db_filename);
				if (sqldb) {
					set_sqlite3_db(sqldb);
				} else {
					printf("sqlite db(name:%s) init error!"
						"use old sqlite db!\n",
						db_filename);
				}
				last_hour = timeinfo->tm_hour; /* 同步当前时间(仅小时) */
			}
		}
		/* 如果分钟数小于59,说明距离下一个整点还有1分钟多
		 * 使用sleep函数等待59 - timeinfo->tm_min 分钟,需要预留出5秒给59以后
		 * 如果分钟数等于59(不可能大于59),说明还有不到一分钟即可到达下一个整点
		 * 进入else
		 * */
		if (timeinfo->tm_min < 59) {
			sleep((59 - timeinfo->tm_min) * 60 - 5);
		} else {
			if (timeinfo->tm_sec < 59) {
				sleep(59 - timeinfo->tm_sec);
			}
		}
	} /* end while(1) */
	return NULL;
}

