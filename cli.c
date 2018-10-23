/*
 * ============================================================================
 *
 *       Filename:  cli.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年09月11日 13时28分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "arg.h"
#include "config.h"
#include "cJSON.h"
#include "lua_action.h"
#include "cli_action.h"
#include "my_log.h"
#include "mosquit_sub.h"

#define GSM_SCAN_FILE "/mnt/sd/scan_info/gsm_scan"
#define BASE_FREQ_STATUS_DIR "/mnt/sd/base_band/"
#define DEFAULT_DATABASES_FILE "/mnt/sd/imsi_info/all.db"
/* diffrent menu type*/
#define PRINT_TOP_MENU (0)
#define PRINT_IMSI_INFOR_MENU (1)
#define SET_LTE_CONFIG_MENU (2)
#define SET_WCDMA_CONFIG_MENU (3)
#define SET_GSM_CONFIG_MENU (4)
/* Print imsi information type */
#define GET_IMSI_INFO_TO_STDOUT (0)
#define GET_IMSI_INFO_TO_FILE (1)

/* 信息保存file fd */
FILE *file = NULL;
/* 保存发送与接收的信息的内存空间 */
char info_mem[5120] = {0};

/* get one line input from stdin */
char *_get_one_line()
{
	char *zresult;
	zresult = readline(NULL);
	if (zresult && *zresult) add_history(zresult);
	return zresult;
}
char *get_one_line_choice()
{
	char *result = NULL;
	while(1) {
		fflush(stdout);
		printf("> ");
		result = _get_one_line();
		if (result) {
			if (result[0]) {
				return result;
			}
			free(result);
		}
	}
}
char *get_one_line()
{
	char *result = NULL;
	while(1) {
		fflush(stdout);
		printf("cli:> ");
		result = _get_one_line();
		if (result) {
			if (result[0]) {
				return result;
			}
			free(result);
		}
	}
}
int tcp_alive(int sock)
{
	int optval = 1;
	int optlen = sizeof(optval);
	if(setsockopt(sock, SOL_SOCKET,	SO_KEEPALIVE, &optval, optlen) < -1) {
		printf("Set tcp keepalive failed: %s\n",
			strerror(errno));
		return 0;
	}
	optval = 1;
	if(setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &optval, optlen) < -1) {
		printf("Set tcp_keepalive_probes failed: %s\n",
			strerror(errno));
		return 0;
	}
	optval = 5;
	if(setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &optval, optlen) < -1) {
		printf("Set tcp_keepalive_time failed: %s\n",
			strerror(errno));
		return 0;
	}
	optval = 5;
	if(setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &optval, optlen) < -1) {
		printf("Set tcp_keepalive_intvl failed: %s\n",
			strerror(errno));
		return 0;
	}
	return 1;
}
/* ================== connect to base_master by socket ==================== */

int init_unixsocket_base_master()
{
	int sockfd;
	struct sockaddr_un server_addr;
	/* connect server */
	sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("cannot create communication socket");
		return -1;
	}
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, CLI_TO_BASE_MASTER_UNIX_DOMAIN);
	int ret = connect(sockfd, (struct sockaddr*)&server_addr,
		sizeof(server_addr));
	if(ret == -1) {
		perror("cannot connect to the server");
		close(sockfd);
		return -1;
	}
	return sockfd;
}
void send_command_to_base_master(char *send_str)
{

}
/* put menu to stdout */
void print_menu(int type)
{
	switch(type) {
	case PRINT_TOP_MENU:
		printf("########################################################\n");
		printf("Welcome to cli!\n");
		printf("Please input command!\n");
		printf(
			"\tlte list       'LTE device list'\n"
			"\tget work mode  'get work mode (Same or pilot frequency)'\n"
			"\tset same mode  'Set same frequency work mode'\n"
			"\tset pilotA mode 'set pilotA frequency work mode'\n"
			"\tset pilotB mode 'set pilotB frequency work mode'\n"
			"\twcdma list     'WCDMA device list'\n"
			"\tgsm list       'GSM device list'\n"
			"\tpa config      'config PA status'\n"
			"\timsi info      'get imsi information'\n"
			"\tbackup imsi    'get and backup imsi information'\n"
			"\thelp [or ?]    'get help information'\n"
			"\tquit           'quit this tool'\n"
		      );
		printf("########################################################\n");
		break;
	case PRINT_IMSI_INFOR_MENU:
		printf("########################################################\n");
		printf("Please input command to get IMSI information\n");
		printf( "\tlast xx min\n"
			"\tlast xx hour\n"
			"\tday YYYY-MM-DD\n"
			"\thour YYYY-MM-DD hh\n"
			"\tbetween 'YYYY-MM-DD hh:mm:ss' and 'YYYY-MM-DD hh:mm:ss'\n"
			"\thelp [or ?]\n"
			"\tcancel\n"
			"\tquit\n");
		printf("########################################################\n");
		break;
	case SET_LTE_CONFIG_MENU:
		printf("########################################################\n");
		printf("Please input command to set LTE device config!\n");
		printf( "\tshow list\n"
			"\n"
			"\tget assplmn IPaddr     -- get assist PLMN list information\n"
			"\tget random IPaddr      -- get proportion of random access\n"
			"\tget arfcn IPaddr       -- get arfcn config information\n"
			"\tget power IPaddr       -- get TX-powerdereas and RX-regain information\n"
			"\tget site IPaddr        -- get device latitude longitude and altitude information\n"
			"\tget reject IPaddr      -- get Location Update reject cause config\n"
			"\tset reject xx IPaddr   -- set Location Update reject cause config\n"
			"\tRange of 'al' (0 ~ 4)\n"
			"\t   0 - cause15 : Access is not allowed in tracking area\n"
			"\t   1 - cause12 : There is no suitable area in tracking area\n"
			"\t   2 - cause3  : Invalid terminal\n"
			"\t   3 - cause13 : ...\n"
			"\t   4 - cause22 : ...\n"
			"\tturn on IPaddr         -- set device hard switch ON\n"
			"\tturn off IPaddr        -- set device hard switch OFF\n"
			"\tresume IPaddr          -- set device hard restart based on power off/on\n"
			"\tactivate IPaddr        -- activate device cell\n"
			"\tdeactivate IPaddr      -- deactivate device cell\n"
			"\tset assplmn IPaddr     -- set assist PLMN list\n"
			"\tset gps sync IPaddr    -- set device GPS sync mode\n"
			"\tset air sync IPaddr    -- set device Air sync mode\n"
			"\tset txpower xx IPaddr  -- set device TX powerdereas (0 Max Export)\n"
			"\tset rxgain xx IPaddr   -- set device RX regain(Max 70)\n"
			"\tset arfcn IPaddr       -- set device arfcn config\n"
			"\tsrestart IPaddr        -- set device soft reboot\n"
			"\threstart IPaddr        -- set device hard reboot\n"
			"\tset TAC xx IPaddr      -- set TAC to baseband manually\n"   /*new add for manually TAC change*/
			"\tset IMEI xx IPaddr     -- set IMEI enable or disable (1 enable, 0 disable)\n"   /*new add for manually IMEI change*/
			"\thelp [or ?]            -- get help\n"
			"\tcancel                 -- return to the next menu\n"
			"\tquit                   -- quit this tool\n");
		printf("########################################################\n");
		break;
	case SET_WCDMA_CONFIG_MENU:
		printf("########################################################\n");
		printf("Please input command to set LTE device config!\n");
		printf( "\tshow list              -- get all WCDMA device information\n"
			"\n"
			"\tget dac IPaddr         -- set device DAC\n"
			"\n"
			"\trestart IPaddr\n"
			"\tactivate IPaddr     --    active device radio frequency\n"
			"\tdeactivate IPaddr   --    deactive device radio frequency\n"
			"\tset workmode IPaddr    -- set device work mode\n"
			"\tset txpower xx IPaddr  -- set device txpower\n"
			"\tset dac xx IPaddr      -- set device DAC\n"
			"\tstop lac IPaddr        -- stop change LAC/RAC by self\n"
			"\tset pilot ratio IPaddr -- set Pilot ratio config\n"
			"\t  Range of 'ratio' (1 ~ 5)\n"
			"\t  1- >10%%, 2- >20%%, 3- >30%%, 4- >40%%, 5- == 10%%\n"
			"\thelp [or ?]            -- get help\n"
			"\tcancel                 -- return to the next menu\n"
			"\tquit                   -- quit this tool\n");
		printf("########################################################\n");
		break;
	case SET_GSM_CONFIG_MENU:
		printf("########################################################\n");
		printf("Please input command to set GSM device config!\n");
		printf("\n"
			"\tget arfcn xx IPaddr\n"
			"\t	   xx:  1:CMCC 2:UNICOM\n"
			"\tset arfcn xx IPaddr\n"
			"\t	   xx:  1:CMCC 2:UNICOM\n"
			"\tRF on xx IPaddr       -- turn on the GSM RF\n"
			"\t      xx: GSM Carrier, 0-ChinaMobile, 1-ChinaUnicom, 2-All of 0 and 1\n"
			"\tRF off xx IPaddr       -- turn off the GSM RF\n"
			"\t       xx: GSM Carrier, 0-ChinaMobile, 1-ChinaUnicom, 2-All of 0 and 1\n"
			"\n"
			"\tget captime IPaddr\n"
			"\tset captime xx(min) IPaddr\n"
			"\n"
			"\tstart scan IPaddr      -- start gsm scan\n"
			"\tget scan info          -- get scan info from scan file\n"
			"\n"
			"\trestart                -- restart gsm device\n"
			"\thelp [or ?]            -- get help\n"
			"\tcancel                 -- return to the next menu\n"
			"\tquit                   -- quit this tool\n");
		printf("########################################################\n");
		break;
	default:
		break;
	}
}

void open_and_readdir(char *band_ip)
{
	FILE *rfd = NULL;
	char *line_str = NULL;
	size_t line_len = 0;
	ssize_t read_len;
	char file_name[512] = {0};
	if (band_ip) {
		snprintf(file_name, 512, "%s%s", BASE_FREQ_STATUS_DIR, band_ip);
		FILE *rfd = fopen(file_name, "r");
		if (!rfd) {
			perror("get arfcn status information  failed!");
			return;
		}
		printf("band(IP:%s), arfcn status information is:\n", band_ip);
		line_len = 0;
		while ((read_len = getline(&line_str, &line_len, rfd)) != -1) {
			printf("    %s", line_str);
		}
		if (line_str) free(line_str);
		line_str = NULL;
		fclose(rfd);
		return;
	}
	DIR* dp;
	struct dirent* ep = NULL;
	dp = opendir(BASE_FREQ_STATUS_DIR);
	if (dp == NULL) {
		printf("not found arfcn information\n");
	}
	printf("======================================\n");
	while ((ep = readdir(dp)) != NULL) {
		if (strcmp(ep->d_name, ".") && strcmp(ep->d_name, "..")) {
			snprintf(file_name, 512, "%s%s",
				BASE_FREQ_STATUS_DIR, ep->d_name);
			printf("band IP:%s\n", ep->d_name);
			FILE *rfd = fopen(file_name, "r");
			if (!rfd) {
				continue;
			}
			line_len = 0;
			while ((read_len = getline(&line_str, &line_len, rfd)) != -1) {
				printf("    %s", line_str);
			}
			if (line_str) free(line_str);
			line_str = NULL;
			fclose(rfd);
			printf("======================================\n");
		}
	}
	closedir(dp);
}
/* =============== sqlite action ==================== */
/* call bak function for sqlite_exec() */
static int action_sqlite_cb(void *NotUsed,
	int argc,
	char **argv,
	char **azColName)
{
	int i;
	for(i = 0; i < argc; i++) {
		if (!strncmp(azColName[i], "IMSI", 4) ||
			!strncmp(azColName[i], "IMEI", 4) ||
			!strncmp(azColName[i], "probeIP", 7))
			fprintf(stdout, "%-15s\t", (argv[i])?argv[i]:"NULL");
		else if (!strncmp(azColName[i], "ID", 2))
			fprintf(stdout, "%-7s\t", (argv[i])?argv[i]:"NULL");
		else
			fprintf(stdout, "%s\t", (argv[i])?argv[i]:"NULL");
	}
	printf("\n");
	return 0;
}
static int action_sqlite_cb_writefile(void *NotUsed,
	int argc,
	char **argv,
	char **azColName)
{
	int i;
	for(i = 0; i < argc; i++) {
		if (!strncmp(azColName[i], "IMSI", 4) ||
			!strncmp(azColName[i], "IMEI", 4) ||
			!strncmp(azColName[i], "probeIP", 7))
			fprintf(file, "%-15s\t", (argv[i])?argv[i]:"NULL");
		else if (!strncmp(azColName[i], "ID", 2))
			fprintf(file, "%-7s\t", (argv[i])?argv[i]:"NULL");
		else
			fprintf(file, "%s\t", (argv[i])?argv[i]:"NULL");
	}
	fprintf(file, "\n");
	return 0;
}
/* open databases */
sqlite3 *open_sqlite3_databases(char *dbfile)
{
	if (dbfile == NULL) return NULL;
	sqlite3 *new_db;
	int ret = sqlite3_open(dbfile, &new_db);
	if (ret) {
		fprintf(stderr, "Can't open database: %s\n",
			sqlite3_errmsg(new_db));
		return NULL;
	}
	return new_db;
}

int action_command_in_database(sqlite3 *db_fd,
	char *cmd_value,
	int (*callback)(void*, int, char**, char**))
{
	if (!db_fd) {
		printf("sqlite3 fd is ineffective!\n");
		return 0;
	}
	if (!cmd_value) {
		printf("can not get database command\n");
		return 0;
	}
	//printf("cmd:%s\n", cmd_value);
	char *zerrmsg = NULL;
	int ret = sqlite3_exec(db_fd, cmd_value, callback, 0, &zerrmsg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zerrmsg);
		sqlite3_free(zerrmsg);
		return 0;
	}
	return 1;
}

void config_frequenc_fun(int mode)
{
	char m[2] = {0};
	snprintf(m, 2, "%d", mode);
	cJSON *object = cJSON_CreateObject();
	if (!object) {
		printf("Oops, creat request error, please try again later!\n");
		sleep(1);
		return;
	}
	cJSON_AddStringToObject(object, "msgtype", "E004");
	cJSON_AddStringToObject(object, "command", m);
	char *send_str = cJSON_PrintUnformatted(object);
	cJSON_Delete(object);
	if (!send_str) {
		printf("Oops, creat request error, please try again later!\n");
		sleep(1);
		return;
	}
	int sockfd = init_unixsocket_base_master();
	if (sockfd != -1) {
		write(sockfd, "AAAA", 4);
		write(sockfd, send_str, strlen(send_str) + 1);
		sleep(1);
		memset(info_mem, 0, 4096);
		int num = read(sockfd, info_mem, sizeof(info_mem));
		if (num > 0)
			printf("%s\n", info_mem);
		close(sockfd);
		sleep(1);
	}
}

void get_imsi_information(int type)
{
	print_menu(PRINT_IMSI_INFOR_MENU);
	char *result = NULL;
	while (1) {
		result = get_one_line();
		if (!strncmp(result, "quit", 4)) {
			printf("Bye!\n");
			exit(0);
		} else if (!strncmp(result, "cancel", 6)) {
			return;
		} else if (!strncmp(result, "help", 4) || result[0] == '?') {
			print_menu(PRINT_IMSI_INFOR_MENU);
		} else if (!strncmp(result, "last", 4)) {
			char this_time[20] = {0};
			char last_time[20] = {0};
			time_t rawtime;
			struct tm * timeinfo;
			rawtime = time(NULL);
			timeinfo = localtime(&rawtime);
			char now_time[20] = {0};
			snprintf(this_time, 20,
				"%04d-%02d-%02d %02d:%02d:%02d",
				timeinfo->tm_year + 1900,
				timeinfo->tm_mon +1, timeinfo->tm_mday,
				timeinfo->tm_hour, timeinfo->tm_min,
				timeinfo->tm_sec);
			if (strstr(result, "min")) {
				int mine = 0;
				sscanf(result, "last %d min", &mine);
				rawtime -= mine * 60;
				timeinfo = localtime(&rawtime);
				snprintf(last_time, 20,
					"%04d-%02d-%02d %02d:%02d:%02d",
					timeinfo->tm_year + 1900,
					timeinfo->tm_mon +1, timeinfo->tm_mday,
					timeinfo->tm_hour, timeinfo->tm_min,
					timeinfo->tm_sec);
			} else if (strstr(result, "hour")) {
				int mine = 0;
				sscanf(result, "last %d hour", &mine);
				rawtime -= mine * 60 * 60;
				timeinfo = localtime(&rawtime);
				snprintf(last_time, 20,
					"%04d-%02d-%02d %02d:%02d:%02d",
					timeinfo->tm_year + 1900,
					timeinfo->tm_mon +1, timeinfo->tm_mday,
					timeinfo->tm_hour, timeinfo->tm_min,
					timeinfo->tm_sec);
			}
			char cmd[128] = {0};
			snprintf(cmd, 128,
				"SELECT * FROM imsi_info "
				"WHERE time BETWEEN '%s' AND '%s'",
				last_time, this_time);
			sqlite3 *dfd =
				open_sqlite3_databases(DEFAULT_DATABASES_FILE);
			if (type == GET_IMSI_INFO_TO_STDOUT) {
				printf("%-7s\t%-15s\t%-15s\t%-15s\t%-19s\t%-10s\n",
					"Num", "ProbeIP", "IMSI", "IMEI",
					"Time", "Type");
				action_command_in_database(dfd, cmd,
					action_sqlite_cb);
			} else if (type == GET_IMSI_INFO_TO_FILE) {
				srand(time(NULL));
				char file_name[24] = {0};
				snprintf(file_name, 24,
					"/tmp/%02d.log", rand() % 100);
				file = fopen(file_name, "w+");
				fprintf(file,
					"%-7s\t%-15s\t%-15s\t%-15s\t%-19s\t%-10s\n",
					"Num", "ProbeIP", "IMSI", "IMEI",
					"Time", "Type");
				action_command_in_database(dfd, cmd,
					action_sqlite_cb_writefile);
				fclose(file);
				printf("#=================================\n");
				printf("# imsi infor has been saved to [%s]\n",
					file_name);
				printf("#=================================\n");
			}
			sqlite3_close(dfd);
		} else if (strstr(result, "between")){
			sqlite3 * dfd =
				open_sqlite3_databases(DEFAULT_DATABASES_FILE);
			char cmd[128] = {0};
			snprintf(cmd, 128, "SELECT * FROM imsi_info "
				"WHERE time %s", result);
			if (type == GET_IMSI_INFO_TO_STDOUT) {
				action_command_in_database(dfd, cmd,
					action_sqlite_cb);
			} else if (type == GET_IMSI_INFO_TO_FILE) {
				srand(time(NULL));
				char file_name[24] = {0};
				snprintf(file_name, 24,
					"/tmp/%02d.log", rand() % 100);
				file = fopen(file_name, "w+");
				action_command_in_database(dfd, cmd,
					action_sqlite_cb_writefile);
				fclose(file);
				printf("#=================================\n");
				printf("# imsi infor has been saved to [%s]\n",
					file_name);
				printf("#=================================\n");
			}
			sqlite3_close(dfd);
		} else if (!strncmp(result, "hour", 4)) {
			int year = 0,month = 0,day = 0, hour = 0;
			char ct;
			sscanf(result, "hour %d%c%d%c%d %d",
				&year, &ct, &month, &ct, &day, &hour);
			char cmd[128] = {0};
			snprintf(cmd, 128,
				"SELECT * FROM imsi_info WHERE time BETWEEN "
				"'%04d-%02d-%02d %02d:00:00' AND '%04d-%02d-%02d %02d:59:59'",
				year, month, day, hour, year, month, day, hour);
			sqlite3 * dfd =
				open_sqlite3_databases(DEFAULT_DATABASES_FILE);
			if (type == GET_IMSI_INFO_TO_STDOUT) {
				action_command_in_database(dfd, cmd,
					action_sqlite_cb);
			} else if (type == GET_IMSI_INFO_TO_FILE) {
				srand(time(NULL));
				char file_name[24] = {0};
				snprintf(file_name, 24,
					"/tmp/%02d.log", rand() % 100);
				file = fopen(file_name, "w+");
				action_command_in_database(dfd, cmd,
					action_sqlite_cb_writefile);
				fclose(file);
				printf("#=================================\n");
				printf("#imsi infor has been saved to [%s]\n",
					file_name);
				printf("#=================================\n");
			}
			sqlite3_close(dfd);
		} else if (!strncmp(result, "day", 3)) {
			int year = 0,month = 0,day = 0;
			char ct;
			sscanf(result, "day %4d%c%2d%c%2d",
				&year, &ct, &month, &ct, &day);
			char cmd[128] = {0};
			snprintf(cmd, 128,
				"SELECT * FROM imsi_info WHERE time BETWEEN "
				"'%04d-%02d-%02d 00:00:00' AND '%04d-%02d-%02d 23:59:59'",
				year, month, day, year, month, day);
			sqlite3 * dfd =
				open_sqlite3_databases(DEFAULT_DATABASES_FILE);
			if (type == GET_IMSI_INFO_TO_STDOUT) {
				action_command_in_database(dfd, cmd,
					action_sqlite_cb);
			} else if (type == GET_IMSI_INFO_TO_FILE) {
				srand(time(NULL));
				char file_name[24] = {0};
				snprintf(file_name, 24,
					"/tmp/%02d.log", rand() % 100);
				file = fopen(file_name, "w+");
				action_command_in_database(dfd, cmd,
					action_sqlite_cb_writefile);
				fclose(file);
				printf("#=================================\n");
				printf("#imsi infor has been saved to [%s]\n",
					file_name);
				printf("#=================================\n");
			}
			sqlite3_close(dfd);
		} else if (!strncmp(result, "all", 3)) {
			char *cmd = "SELECT * FROM imsi_info";
			sqlite3 * dfd =
				open_sqlite3_databases(DEFAULT_DATABASES_FILE);
			if (type == GET_IMSI_INFO_TO_STDOUT) {
				action_command_in_database(dfd, cmd,
					action_sqlite_cb);
			} else if (type == GET_IMSI_INFO_TO_FILE) {
				srand(time(NULL));
				char file_name[24] = {0};
				snprintf(file_name, 24,
					"/tmp/%02d.log", rand() % 100);
				file = fopen(file_name, "w+");
				action_command_in_database(dfd, cmd,
					action_sqlite_cb_writefile);
				fclose(file);
				printf("#=================================\n");
				printf("#imsi infor has been saved to [%s]\n",
					file_name);
				printf("#=================================\n");
			}
			sqlite3_close(dfd);
		} else {
			printf("Oops!\n"
				"    unknow command \"%s\", please check!\n\n\n\n",
				result);
			sleep(1);
			print_menu(PRINT_IMSI_INFOR_MENU);
		}

		free(result);
		result = NULL;
	}
}

/* set wcdma base band device config */
int set_wcdma_base_band_config()
{
	char *result = NULL;
	char set_ip[20];
	char *send_str = NULL;
	print_menu(SET_WCDMA_CONFIG_MENU);
	while (1) {
		result = get_one_line();
		if (!strncmp(result, "quit", 4)) {
			printf("Bye!\n");
			exit(1);
		} else if (!strncmp(result, "help", 4) || result[0] == '?') {
			print_menu(SET_WCDMA_CONFIG_MENU);
			continue;
		} else if (!strncmp(result, "cancel", 6)) {
			return 0;
		} else if (!strncmp(result, "show list", 10)) {
			return 1;
		} else if (!strncmp(result, "restart", 7)) {
			sscanf(result, "restart %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00B");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "activate", 8)) {
			sscanf(result, "activate %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00D");
			cJSON_AddStringToObject(object, "work_admin_state", "1");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "deactivate", 10)) {
			sscanf(result, "deactivate %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00D");
			cJSON_AddStringToObject(object, "work_admin_state", "0");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set txpower", 11)) {
			char txpower[4] = {0};
			sscanf(result, "set txpower %s %s", txpower, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F015");
			cJSON_AddStringToObject(object, "powerderease", txpower);
			cJSON_AddStringToObject(object, "is_saved", "1");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get dac", 7)) {
			sscanf(result, "get dac %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F083");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set dac", 7)) {
			char dac_of[4] = {0};
			sscanf(result, "set dac %s %s", dac_of, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F084");
			cJSON_AddStringToObject(object, "offset", dac_of);
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "stop lac", 8)) {
			char dac_of[4] = {0};
			sscanf(result, "stop lac %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F086");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set pilot", 9)) {
			char ratio[4] = {0};
			sscanf(result, "set pilot %s %s", ratio, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F080");
			cJSON_AddStringToObject(object, "ratio", ratio);
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set workmode", 12)) {
			sscanf(result, "set workmode %s", set_ip);
			printf("set ip is [%s]\n", set_ip);
			lua_State *luaenv;
			printf("if you want to set wcdma device work mode,"
				"please change config in file "
				"'/etc/config/base_master' by uci or vim;\n"
				"when change finished "
				"please input 'yes'/'no'\n");
			while (1) {
				char *tmp_re = get_one_line();
				if (!strcmp(tmp_re, "yes")) {
					luaenv = lua_dofile_func(
						GET_STATE_LUA_FILE,
						"get_wcdma_wm_config_by_ip",
						set_ip);
					if (!luaenv) {
						printf("Load LUA file error!\n"
							"please check file "
							"and run gain!\n"
							"file name :%s\n",
							GET_STATE_LUA_FILE);
						free(result);
						return 0;
					}
					const char *lua_ret =
						lua_tostring(luaenv, -1);
					lua_close_luaenv(luaenv);
					if(!lua_ret ||
						!strncmp(lua_ret, "none", 4)) {
						printf("Not get lua return!"
							"Or can't get arfcn "
							"in /etc/config/base_master!\n");
						free(result);
						return 0;
					}
					send_str = calloc(1, strlen(lua_ret) + 1);
					strncpy(send_str, lua_ret, strlen(lua_ret));
					break;
				} else if (!strcmp(tmp_re, "no")) {
					free(tmp_re);
					return 1;
				} else {
					printf("please input 'yes' or 'no'\n");
				}
				free(tmp_re);
			}
		}
		if (send_str) {
			//printf("%s\n", send_str);
			printf("request sended!\n");
			int sockfd = init_unixsocket_base_master();
			if (sockfd != -1) {
				write(sockfd, "AAAA", 4);
				write(sockfd, send_str, strlen(send_str) + 1);
				sleep(1);
				memset(info_mem, 0, 4096);
				int num = read(sockfd, info_mem, sizeof(info_mem));
				if (num > 0)
					printf("Config response:\n%s\n",
						info_mem);
				close(sockfd);
				sleep(1);
			}
			free(send_str);
			send_str = NULL;
		}
		free(result);
	}
}
char *assemble_set_accplmn_json_string(char *ipaddr)
{
	if (!ipaddr) {
		printf("please in put Ipaddr !\n");
		return NULL;
	}
	cJSON *object = cJSON_CreateObject();
	if (!object) return NULL;
	cJSON_AddStringToObject(object, "msgtype", "F062");
	cJSON_AddStringToObject(object, "ip", ipaddr);
	char *send_str = cJSON_PrintUnformatted(object);
	if (!send_str) return NULL;
	cJSON_Delete(object);
	int sockfd = init_unixsocket_base_master();
	if (sockfd != -1) {
		write(sockfd, "AAAA", 4);
		write(sockfd, send_str, strlen(send_str) + 1);
		sleep(1);
		memset(info_mem, 0, 4096);
		int num = read(sockfd, info_mem, sizeof(info_mem));
		if (num > 0)
			printf("base band (IP:%s)\n%s\n", ipaddr, info_mem);
		close(sockfd);
		sleep(1);
	}
	free(send_str);
	send_str = NULL;
	while (1) {
		printf("Do you want to change assist plmn config?(yes/no)\n");
		char *tmp_re = get_one_line_choice();
		if (!strncmp(tmp_re, "yes", 3)) {
			printf("please input plmn list, "
				"separate plmn with commas "
				"and without white space between them,"
				"such as 46000,46001\n");
			char *plmn_l = get_one_line_choice();
			printf("you input plmns are %s\n", plmn_l);
			int i, j, m = 0;
			char plmn_set[5][7] = {0};
			for (i = 0, j = i; i < strlen(plmn_l); i++) {
				if (plmn_l[i] == ',') {
					memcpy(plmn_set[m++], &plmn_l[j], i - j);
					j = i + 1;
				}
			}
			memcpy(plmn_set[m], &plmn_l[j], i - j);
			object = cJSON_CreateObject();
			if (!object) {
				printf("Oops! creat string failed! please run later\n");
				return NULL;
			}
			char plmn_count[2] = {0};
			snprintf(plmn_count, 2, "%d", m + 1);
			cJSON_AddStringToObject(object, "msgtype", "F060");
			cJSON_AddStringToObject(object, "ip", ipaddr);
			cJSON_AddStringToObject(object, "plmn_num", plmn_count);
			cJSON *array = cJSON_CreateArray();
			while(m >= 0) {
				cJSON *t = cJSON_CreateString(plmn_set[m--]);
				if (t) cJSON_AddItemToArray(array, t);
			}
			cJSON_AddItemToObjectCS(object, "plmn_list", array);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			return send_str;
		} else if (!strncmp(tmp_re, "no", 2)) {
			free(tmp_re);
			return NULL;
		} else {
			printf("please input 'yes' or 'no'\n");
		}
	}
}

#define ZONE_CONFIG_PA_SWITCH 1
void config_zone_value(char *ipaddr, int zone_t)
{
	switch (zone_t) {
	case ZONE_CONFIG_PA_SWITCH: /* PA switch config */
	while(1) {
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "E003");
			cJSON_AddStringToObject(object, "remmode", "0");
			cJSON_AddStringToObject(object, "ip", ipaddr);
			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (send_str) free(send_str);
			break;
		}
	default:
		break;
	}
}
/* set LTE base band device config */
int set_lte_base_band_config()
{
	char *result = NULL;
	char set_ip[20];
	char *send_str = NULL;
	print_menu(SET_LTE_CONFIG_MENU);
	while (1) {
		result = get_one_line();
		if (!strncmp(result, "quit", 4)) {
			printf("Bye!\n");
			exit(1);
		} else if (!strncmp(result, "help", 4) || result[0] == '?') {
			print_menu(SET_LTE_CONFIG_MENU);
			continue;
		} else if (!strncmp(result, "cancel", 6)) {
			return 0;
		} else if (!strncmp(result, "show list", 10)) {
			return 1;
		} else if (!strncmp(result, "srestart", 7)) {
			sscanf(result, "srestart %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00B");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "hrestart", 8)) {
			sscanf(result, "hrestart %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F099");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "cmd", "2");
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "turn on", 7)) {
			//write_action_log(set_ip, "cli power on action");
			sscanf(result, "turn on %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F099");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "cmd", "1");
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "turn off", 8)) {
			//write_action_log(set_ip, "cli power off action");
			sscanf(result, "turn off %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F099");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "cmd", "0");
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "resume", 6)) {
			//write_action_log(set_ip, "cli resume action");
			//this will restart corresponding baseband,nyb,2018.04.02
			sscanf(result, "resume %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F099");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "cmd", "2");
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set TAC", 7)) {
			/*new add for cli to modify the TAC manually nyb 2018.05.01*/
			U8 tac_value[4] = {0};
			sscanf(result, "set TAC %s %s", tac_value, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F069");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "tac_value", (char *)tac_value);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set IMEI", 8)) {
			/*new add for cli to modify the TAC manually nyb 2018.05.01*/
			U8 *ImeiEnable = NULL;
			sscanf(result, "set IMEI %s %s", ImeiEnable, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F08A");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "ImeiEnable", (char *)ImeiEnable);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get site", 8)) {
			sscanf(result, "get site %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F05C");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get arfcn", 9)) {
			sscanf(result, "get arfcn %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F027");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get random", 10)) {
			sscanf(result, "get random %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F065");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get power", 9)) {
			sscanf(result, "get power %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F031");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get reject", 10)) {
			sscanf(result, "get reject %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F06B");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set reject", 10)) {
			char rejectCause[2] = {0};
			sscanf(result, "set reject %s %s",rejectCause, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F057");
			cJSON_AddStringToObject(object, "reject_cause", rejectCause);
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "activate", 8)) {
			sscanf(result, "activate %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00D");
			cJSON_AddStringToObject(object, "work_admin_state", "1");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "deactivate", 10)) {
			sscanf(result, "deactivate %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00D");
			cJSON_AddStringToObject(object, "work_admin_state", "0");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get assplmn", 11)) {
			sscanf(result, "get assplmn %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F062");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set assplmn", 11)) {
			sscanf(result, "set assplmn %s", set_ip);
			send_str = assemble_set_accplmn_json_string(set_ip);
			if (!send_str) return 1;
		} else if (!strncmp(result, "set rxgain", 10)) {
			char rxgain[4] = {0};
			sscanf(result, "set rxgain %s %s", rxgain, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F013");
			cJSON_AddStringToObject(object, "rxgain", rxgain);
			cJSON_AddStringToObject(object, "is_saved", "1");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set txpower", 11)) {
			char txpower[4] = {0};
			sscanf(result, "set txpower %s %s", txpower, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F015");
			cJSON_AddStringToObject(object, "powerderease", txpower);
			cJSON_AddStringToObject(object, "is_saved", "1");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set gps sync", 12)) {
			sscanf(result, "set gps sync %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F023");
			cJSON_AddStringToObject(object, "remmode", "1");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set air sync", 12)) {
			sscanf(result, "set air sync %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F023");
			cJSON_AddStringToObject(object, "remmode", "0");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set arfcn", 9)) {
			sscanf(result, "set arfcn %s", set_ip);
			open_and_readdir(set_ip);
			lua_State *luaenv;
			luaenv = lua_dofile_func(GET_STATE_LUA_FILE,
				"get_freq_config_by_ip",
				set_ip);
			if (!luaenv) {
				printf("sorry, load LUA file error!\n"
					"please check file and run gain!\n"
					"file name :%s", GET_STATE_LUA_FILE);
				lua_close_luaenv(luaenv);
				return 0;
			}
			lua_tostring(luaenv, -1);
			lua_close_luaenv(luaenv);
			printf("if you want to set arfcn,"
				"please change config in file "
				"'/etc/config/base_master' by uci or vim;\n"
				"when change finished "
				"please input 'yes'/'no'\n");
			while (1) {
				char *tmp_re = get_one_line_choice();
				if (!strcmp(tmp_re, "yes")) {
					luaenv = lua_dofile_func(
						GET_STATE_LUA_FILE,
						"get_freq_config_by_ip",
						set_ip);
					if (!luaenv) {
						printf("Load LUA file error!\n"
							"please check file "
							"and run gain!\n"
							"file name :%s\n",
							GET_STATE_LUA_FILE);
						free(result);
						return 0;
					}
					const char *lua_ret =
						lua_tostring(luaenv, -1);
					lua_close_luaenv(luaenv);
					if(!lua_ret ||
						!strncmp(lua_ret, "none", 4)) {
						printf("Not get lua return!"
							"Or can't get arfcn "
							"in /etc/config/base_master!\n");
						free(result);
						return 0;
					}
					send_str = calloc(1, strlen(lua_ret) + 1);
					strncpy(send_str, lua_ret, strlen(lua_ret));
					break;
				} else if (!strcmp(tmp_re, "no")) {
					free(tmp_re);
					return 1;
				} else {
					printf("please input 'yes' or 'no'\n");
				}
				free(tmp_re);
			}
		} else {
			printf("Oops!\n"
				"    unknow command \"%s\", please check!\n\n\n\n",
				result);
			//write_action_log("cli unknow command:", result);
			sleep(1);
			print_menu(SET_LTE_CONFIG_MENU);
		}
		if (send_str) {
			//printf("%s\n", send_str);
			printf("request sended!\n");
			int sockfd = init_unixsocket_base_master();
			if (sockfd != -1) {
				write(sockfd, "AAAA", 4);
				write(sockfd, send_str, strlen(send_str) + 1);
				sleep(1);
				memset(info_mem, 0, 4096);
				int num = read(sockfd, info_mem, sizeof(info_mem));
				if (num > 0)
					printf("Config response:\n%s\n", info_mem);
				close(sockfd);
				sleep(1);
			}
			free(send_str);
			send_str = NULL;
		}
		free(result);
	}
}

/* set GSM base band device config */
int set_gsm_base_band_config()
{
	char *result = NULL;
	char set_ip[20];
	char *send_str = NULL;
	print_menu(SET_GSM_CONFIG_MENU);
	while (1) {
		result = get_one_line();
		if (!strncmp(result, "quit", 4)) {
			printf("Bye!\n");
			exit(1);
		} else if (!strncmp(result, "help", 4) || result[0] == '?') {
			print_menu(SET_GSM_CONFIG_MENU);
			continue;
		} else if (!strncmp(result, "cancel", 6)) {
			return 0;
		} else if (!strncmp(result, "restart", 7)) {
			sscanf(result, "restart %s", set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00B");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "start scan", 10)) {
			printf("please get scan info after 5 min\n");
			cJSON *object = cJSON_CreateObject();
			sscanf(result, "start scan %s", set_ip);
			cJSON_AddStringToObject(object, "msgtype", "03");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get scan info", 13)) {
			FILE * gsmscan_fp = fopen(GSM_SCAN_FILE, "r");
			char *len;
			if(gsmscan_fp == NULL)
			{
				printf("open gsm scan file err!\n");
				sleep(1);
				continue;
			}
			while(1)
			{
				char buf[64];
				if(!fgets(buf, 64, gsmscan_fp))
				{
					printf("read end\n");
					break;
				}
				printf("%s", buf);
			}
			fclose(gsmscan_fp);
			sleep(1);
			continue;
		} else if (!strncmp(result, "RF on", 5)) {
			U32 rf_ac = 0;
			sscanf(result, "RF on %u %s", &rf_ac, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00D");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "work_admin_state", "1");
			cJSON_AddNumberToObject(object, "gsm_carrier", rf_ac);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "RF off", 6)) {
			U32 rf_ac = 0;
			sscanf(result, "RF off %u %s", &rf_ac, set_ip);
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object, "msgtype", "F00D");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "work_admin_state", "0");
			cJSON_AddNumberToObject(object, "gsm_carrier", rf_ac);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get captime", 11)) {
			cJSON *object = cJSON_CreateObject();
			sscanf(result, "get captime %s", set_ip);
			cJSON_AddStringToObject(object, "msgtype", "A005");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set captime", 11)) {
			char set_captime[5] = "10";
			cJSON *object = cJSON_CreateObject();
			sscanf(result, "set captime %s %s", set_captime, set_ip);
			cJSON_AddStringToObject(object, "msgtype", "A006");
			cJSON_AddStringToObject(object, "ip", set_ip);
			cJSON_AddStringToObject(object, "captime", set_captime);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "get arfcn", 9)) {
			char arfcn_num[2] = "1";
			cJSON *object = cJSON_CreateObject();
			sscanf(result, "get arfcn %s %s", arfcn_num, set_ip);
			if(!strncmp(arfcn_num, "1", 1))
				cJSON_AddStringToObject(object, "msgtype", "A001");
			else
				cJSON_AddStringToObject(object, "msgtype", "A002");
			cJSON_AddStringToObject(object, "ip", set_ip);
			send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
		} else if (!strncmp(result, "set arfcn", 9)) {
			char arfcn_num[2] = "1";
			sscanf(result, "set arfcn %s %s", arfcn_num, set_ip);
			if(strncmp(set_ip, GSM_IP_ADDR, 15))
			{
				printf("sorry, can not found %s\n", set_ip);
				sleep(1);
				continue;
			}
			open_and_readdir(set_ip);
			lua_State *luaenv;
			luaenv = lua_dofile_func(GET_STATE_LUA_FILE,
				"get_gsm_config_by_ip",
				arfcn_num);
			if (!luaenv) {
				printf("sorry, load LUA file error!\n"
					"please check file and run gain!\n"
					"file name :%s", GET_STATE_LUA_FILE);
				lua_close_luaenv(luaenv);
				return 0;
			}
			lua_tostring(luaenv, -1);
			lua_close_luaenv(luaenv);
			printf("if you want to set arfcn,"
				"please change config in file "
				"'/etc/config/base_master' by uci or vim;\n"
				"when change finished "
				"please input 'yes'/'no'\n");
			while (1) {
				char *tmp_re = get_one_line();
				if (!strcmp(tmp_re, "yes")) {
					luaenv = lua_dofile_func(
						GET_STATE_LUA_FILE,
						"get_gsm_config_by_ip",
						arfcn_num);
					if (!luaenv) {
						printf("Load LUA file error!\n"
							"please check file "
							"and run gain!\n"
							"file name :%s\n",
							GET_STATE_LUA_FILE);
						free(result);
						return 0;
					}
					const char *lua_ret =
						lua_tostring(luaenv, -1);
					lua_close_luaenv(luaenv);
					if(!lua_ret ||
						!strncmp(lua_ret, "none", 4)) {
						printf("Not get lua return!"
							"Or can't get arfcn "
							"in /etc/config/base_master!\n");
						free(result);
						return 0;
					}
					send_str = calloc(1, strlen(lua_ret) + 1);
					strncpy(send_str, lua_ret, strlen(lua_ret));
					break;
				} else if (!strcmp(tmp_re, "no")) {
					free(tmp_re);
					return 1;
				} else {
					printf("please input 'yes' or 'no'\n");
				}
				free(tmp_re);
			}
		} else {
			printf("Oops!\n"
				"    unknow command \"%s\", please check!\n",
				result);
			sleep(1);
			print_menu(SET_GSM_CONFIG_MENU);
		}
		if (send_str) {
//			printf("%s\n", send_str);
			printf("request sended!\n");
			int sockfd = init_unixsocket_base_master();
			if (sockfd != -1) {
				write(sockfd, "AAAA", 4);
				write(sockfd, send_str, strlen(send_str) + 1);
				sleep(1);
				memset(info_mem, 0, 4096);
				int num = read(sockfd, info_mem, sizeof(info_mem));
				if (num > 0)
				{
					if(!strncmp(result, "get arfcn", 9))
					{
						if (strncmp(info_mem, "nothing", 7) && strncmp(info_mem, "sorry, you can not config to offlinedevice!", 43)) {
							lua_State *luaenv = lua_dofile_func(GET_STATE_LUA_FILE,
								"pares_gsm_base_band_json_information",
								info_mem);
							if (luaenv) {
								lua_close_luaenv(luaenv);
							} else {
								printf("sorry, load LUA file error!\n"
									"please check file and run gain!\n");
							}
						} else {
							printf("sorry, I can't get GSM device infor from base_master\n");
							sleep(1);
						}
					}
					else
						printf("Config response:\n%s\n", info_mem);
				}
				close(sockfd);
				sleep(1);
			}
			free(send_str);
			send_str = NULL;
		}
		free(result);
	}
}

void get_lte_band_list_information()
{
	int offset, num;
	int sockfd = -1;
base_dev_start:
	offset = 0;
	memcpy(&info_mem[offset], "AAAA", 4);
	offset += 4;
	unsigned short int type = CLI_TO_BASE_MASTER_GET_LTE_DEV_INFO;
	memcpy(&info_mem[offset], &type, 2);
	offset += 2;
	info_mem[offset] = 0;
	offset += 1;
	sockfd = init_unixsocket_base_master();
	if (sockfd == -1) {
		printf("cread socket that connet to base_master failed!\n");
		return;
	}
	write(sockfd, info_mem, offset);
	memset(info_mem, 0, offset);
	num = read(sockfd, info_mem, sizeof(info_mem));
	close(sockfd);
	if (num > 0 && strncmp(info_mem, "nothing", 7)) {
		lua_State *luaenv = lua_dofile_func(GET_STATE_LUA_FILE,
			"pares_base_band_json_information", info_mem);
		if (luaenv) {
			lua_close_luaenv(luaenv);
		} else {
			printf("sorry, load LUA file error!\n"
				"please check file and run gain!\n");
			return;
		}
		if (set_lte_base_band_config()) {
			sleep(1);
			goto base_dev_start;
		}
	} else {
		printf("sorry, I can't get LTE device infor from base_master\n");
		sleep(1);
	}
}

void get_wcdma_device_list_information()
{
	int offset, num;
	int sockfd = -1;
base_dev_start:
	offset = 0;
	memcpy(&info_mem[offset], "AAAA", 4);
	offset += 4;
	unsigned short int type = CLI_TO_BASE_MASTER_GET_WCDMA_DEV_INFO;
	memcpy(&info_mem[offset], &type, 2);
	offset += 2;
	info_mem[offset] = 0;
	offset += 1;
	sockfd = init_unixsocket_base_master();
	if (sockfd == -1) {
		printf("cread socket that connet to base_master failed!\n");
		return;
	}
	write(sockfd, info_mem, offset);
	sleep(1);
	memset(info_mem, 0, sizeof(info_mem));
	num = read(sockfd, info_mem, sizeof(info_mem));
	close(sockfd);
	if (num > 0 && strncmp(info_mem, "nothing", 7)) {
		lua_State *luaenv = lua_dofile_func(GET_STATE_LUA_FILE,
			"pares_wcdma_base_band_json_information",
			info_mem);
		if (luaenv) {
			lua_close_luaenv(luaenv);
		} else {
			printf("sorry, load LUA file error!\n"
				"please check file and run gain!\n");
			return;
		}
		if (set_wcdma_base_band_config()) {
			sleep(1);
			goto base_dev_start;
		}
	} else {
		printf("sorry, I can't get WCDMA device infor from base_master\n");
		sleep(1);
	}
	return;
}

void get_gsm_device_list_information()
{
	int offset, num;
	int sockfd = -1;
base_dev_start:
	offset = 0;
	memcpy(&info_mem[offset], "AAAA", 4);
	offset += 4;
	unsigned short int type = CLI_TO_BASE_MASTER_GET_GSM_DEV_INFO;
	memcpy(&info_mem[offset], &type, 2);
	offset += 2;
	info_mem[offset] = 0;
	offset += 1;
	sockfd = init_unixsocket_base_master();
	if (sockfd == -1) {
		printf("cread socket that connet to base_master failed!\n");
		return;
	}
	write(sockfd, info_mem, offset);
	sleep(1);
	memset(info_mem, 0, sizeof(info_mem));
	num = read(sockfd, info_mem, sizeof(info_mem));
	close(sockfd);
	if (num > 0 && strncmp(info_mem, "nothing", 7)) {
		cJSON *object = cJSON_Parse(info_mem);
		if (object) {
			char *str = cJSON_Print(object);
			if (str) {
				printf("%s\n", str);
				free(str);
			}
			cJSON_Delete(object);
		}
#if 0
		lua_State *luaenv = lua_dofile_func(GET_STATE_LUA_FILE,
			"pares_gsm_base_band_json_information",
			info_mem);
		if (luaenv) {
			lua_close_luaenv(luaenv);
		} else {
			printf("sorry, load LUA file error!\n"
				"please check file and run gain!\n");
			return;
		}
#endif
		if (set_gsm_base_band_config()) {
			sleep(1);
			goto base_dev_start;
		}
	} else {
		printf("sorry, I can't get GSM device infor from base_master\n");
		sleep(1);
	}
	return;
}
/* 避免多个进程*/
static int one_instance()
{
	int pid_file = open("/var/run/cli.pid", O_CREAT | O_RDWR, 0666);
	int flags = fcntl(pid_file, F_GETFD);
	flags |= FD_CLOEXEC;
	fcntl(pid_file, F_SETFD, flags);
	int rc = flock(pid_file, LOCK_EX | LOCK_NB);
	if(rc) {
		if(EWOULDBLOCK == errno) {
			printf("another mqttc is running\n");
			return -1;
		}
	}
	return 0;
}
/* ================================================= */
int main(int argc, char *argv[])
{
	if (one_instance() < 0) {
		printf("%s is running.\n", argv[0]);
		return 0;
	}
	print_menu(PRINT_TOP_MENU);
	char *result = NULL;
	while(1) {
		result = get_one_line();
		if (!strncmp(result, "help", 4) || result[0] == '?') {
			print_menu(PRINT_TOP_MENU);
			continue;
		} else if (!strncmp(result, "quit", 4)) {
			printf("Bye!\n");
			return 0;
		} else if (!strncmp(result, "lte list", 8)) {
			get_lte_band_list_information();
			print_menu(PRINT_TOP_MENU);
		} else if (!strncmp(result, "wcdma list", 10)) {
			get_wcdma_device_list_information();
			print_menu(PRINT_TOP_MENU);
		} else if (!strncmp(result, "gsm list", 8)) {
			get_gsm_device_list_information();
			print_menu(PRINT_TOP_MENU);
		} else if (!strncmp(result, "imsi info", 9)) {
			get_imsi_information(GET_IMSI_INFO_TO_STDOUT);
			print_menu(PRINT_TOP_MENU);
		} else if (!strncmp(result, "backup imsi", 11)) {
			get_imsi_information(GET_IMSI_INFO_TO_FILE);
			print_menu(PRINT_TOP_MENU);
		} else if (!strncmp(result, "get work mode", 13)) {
			config_frequenc_fun(GET_SAME_OR_DIF_STATE);
		} else if (!strncmp(result, "set same mode", 13)) {
			config_frequenc_fun(SET_STATE_SAME);
		} else if (!strncmp(result, "set pilotA mode", 15)) {
			config_frequenc_fun(SET_STATE_DIF_A);
		} else if (!strncmp(result, "set pilotB mode", 15)) {
			config_frequenc_fun(SET_STATE_DIF_B);
		} else {
			if (!strncmp(result, "pa config", 9)) {
				printf("please run serialtest follow the prompt!\n\n\n\n");
			} else {
				printf("Oops!\n"
					"    unknow command \"%s\", please check!\n\n\n\n",
					result);
			}
			sleep(1);
			print_menu(PRINT_TOP_MENU);
		}
		free(result);
		result = NULL;
	}
}
