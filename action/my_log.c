/*
 * ============================================================================
 *
 *       Filename:  my_log.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018年02月22日 11时46分25秒
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
#include <unistd.h>
#include <time.h>
#include "my_log.h"
#include "arg.h"
#include "config.h"

FILE *log_file = NULL;
int log_flag = 0; /* 是否开启操作日志写入 0-未开启,1-开启*/ 
int write_flag = 0;
int f_line = 0;
#define MAX_LOG_FILE_LINE (70) /* 70 line */
char log_fname[48] = {0}; /* 当前日志文件名称 */
char rm_fname[48] = {0}; /* 需要删除的历史文件名称 */
char sed_cmd[96] = {0}; /* 当文件行数大于MAX_LOG_FILE_LINE ,删除前10行 */

/* 每天一个文件 */
void *thread_checkdate_touchfile(void *arg)
{
#ifdef BIG_END
	return NULL;
#endif
	time_t rawtime, rmtime;
	struct tm *timeinfo, *rmtimeinfo;
	char time_dirname[10] = {0};
	char now_fname[16] = {0};
	char last_fname[16] = {0};
	char tlast_fname[16] = {0};
	strcat(log_fname, args_info.log_dir_arg);
	if (log_fname[strlen(log_fname) - 1] != '/') {
		strcat(log_fname, "/");
	}
	memcpy(rm_fname, log_fname, strlen(log_fname));
	while(1) {
		rawtime = time(NULL);
		timeinfo = localtime(&rawtime);
		snprintf(now_fname, 16, "%04d%02d%02d.log",
			timeinfo->tm_year + 1900,
			timeinfo->tm_mon + 1,
			timeinfo->tm_mday);

		rmtime = rawtime - 60 * 60 * 24 * 2; /* 删除两天前的日志文件 */
		rmtimeinfo = localtime(&rmtime);
		snprintf(tlast_fname, 16, "%04d%02d%02d.log",
			rmtimeinfo->tm_year + 1900,
			rmtimeinfo->tm_mon + 1,
			rmtimeinfo->tm_mday - 1);
		strcat(rm_fname, tlast_fname);
		remove(rm_fname);

		if (strcmp(last_fname, now_fname)) {
			memcpy(last_fname, now_fname, sizeof(last_fname));

			strcat(log_fname, now_fname);
			FILE *t_file = fopen(log_fname, "a+");
			if (t_file) {
				write_flag = 0;
				if (log_file) {
					fclose(log_file);
				}
				log_file = t_file;
				write_flag = 1;
			}
		}
		snprintf(sed_cmd, 96, "sed -i '10d' %s", log_fname); /* 删除文件前10行等命令 */
		if (timeinfo->tm_hour < 23) {
			sleep((24 - timeinfo->tm_hour) * 60 * 60);
		} else if (timeinfo->tm_min < 59) {
			sleep((59 - timeinfo->tm_min) * 60 - 5);
		} else {
			if (timeinfo->tm_sec < 59) {
				sleep(59 - timeinfo->tm_sec);
			}
		}
	} /* end while(1) */
	return NULL;
}

void init_action_log()
{
#ifdef BIG_END
	return;
#endif
	if (!strlen(args_info.log_dir_arg)) {
		return;
	}
	log_flag = 1;
	create_directory(args_info.log_dir_arg);
	safe_pthread_create(thread_checkdate_touchfile, NULL);
	sleep(1);
}

void write_action_log(char *f_key, char *f_str)
{
#ifdef BIG_END
	return;
#endif
	if (!log_flag) return;
	if (!write_flag) return;
	if (!f_key || !f_str) return;
	write_flag = 0;
	time_t rawtime;
	struct tm *timeinfo;
	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);
	if (log_file) {
		fprintf(log_file, "%04d-%02d-%02d %02d:%02d:%02d  %s,%s\n",
			timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
			timeinfo->tm_mday, timeinfo->tm_hour,
			timeinfo->tm_min, timeinfo->tm_sec, f_key, f_str);
		fflush(log_file);
		f_line++;
		if (f_line >= MAX_LOG_FILE_LINE) {
			system(sed_cmd);
			f_line -= 10;
		}
	}
	write_flag = 1;
}

