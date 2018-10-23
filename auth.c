/*
 * ============================================================================
 *
 *       Filename:  auth.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年11月14日 10时00分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  weichao lu (chao), lwc_yx@163.com
 *   Organization:  
 *
 * ============================================================================
 */

#include "auth.h"
#include "md5.h"

int uci_set_devid()
{
	unsigned char cpuid_buf[LEN] = {0};
	char cpuid_md5[LEN] = {0};
	char uci_set_cmd[128] = {0};

	FILE *fp = popen(GETCPUID_CMD, "r");
	if(fp == NULL) {
		printf("popen err!\n");
		return -1;
	}
	// fgets(cpuid_buf, LEN, fp);
	// cpuid_buf[strlen(cpuid_buf) - 1] = '\0';
	fscanf(fp, "%s", cpuid_buf);
	// printf("--->cpuid:%s\n", cpuid_buf);
	pclose(fp);

	if(set_MD5(cpuid_md5, cpuid_buf) != 0) {
		printf("cpu id set MD5 err!\n");
		return -1;
	}
	// printf("--->cpuid MD5:%s\n", cpuid_md5);

	sprintf(uci_set_cmd, SETID, cpuid_md5);
	// printf("--->set:%s\n", uci_set_cmd);
	system(uci_set_cmd);
	system(UCICOMMIT);

	return 0;
}

int uci_get(char *buf, char *msg)
{
	FILE *fp = popen(msg, "r");
	if(fp == NULL) {
		printf("popen err!\n");
		return -1;
	}
	// fgets(buf, LEN, fp);
	// buf[strlen(buf) - 1] = '\0';
	fscanf(fp, "%s", buf);
	pclose(fp);
	return 0;
}

int set_dm_msg(char *newbuf, char *oldbuf)
{
	sprintf(newbuf, AUTH_ID, oldbuf);
	return 0;
}

int set_MD5(char *MD5_msg, unsigned char *msg)
{
	unsigned char md[16] = {0};
	MD5Init(&MD5_C);
	MD5Update(&MD5_C, msg, strlen((char *)msg));
	MD5Final(&MD5_C, md);
	int i, n = 0;
	for(i = 0; i < 16; i++) {
		sprintf(&MD5_msg[n], "%02x", md[i]);
		n += 2;
	}
	return 0;
}

int auth()
{
	char devid[LEN] = {0};
	unsigned char dm_devid[LEN] = {0};
	char md5_authid[LEN] = {0};
	char file_authid[LEN] = {0};
	if(uci_set_devid() != 0) {
		printf("uci set devid err!\n");
		return 0;
	}
	if(uci_get(devid, GETID) != 0) {
		printf("get uci msg err!\n");
		return 0;
	}
	// printf("devid:%s\n", devid);
	set_dm_msg((char *)dm_devid, devid);
	// printf("dm devid:%s\n", dm_devid);
	if(set_MD5(md5_authid, dm_devid) != 0) {
		printf("set MD5 err!\n");
		return 0;
	}
	if(uci_get(file_authid, GETAUTHID) != 0) {
		printf("get uci msg err!\n");
		return 0;
	}
	// printf("--->%s\n", md5_authid);
	// printf("--->%s\n", file_authid);
	if(strncmp(file_authid, md5_authid, strlen(md5_authid)) != 0) {
		// printf("设备未授权！\n");
		system(SETSTATUS0);
		system(SETAUTHWORD0);
		system(UCICOMMIT);
		return 0;
	}
	// printf("设备已授权！\n");
	system(SETSTATUS1);
	system(SETAUTHWORD1);
	system(UCICOMMIT);
	return 1;
}
