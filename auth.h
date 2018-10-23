/*
 * =====================================================================================
 *
 *       Filename:  auth.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年11月14日 10时01分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  weichao lu (chao), lwc_yx@163.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __AUTH_H__
#define __AUTH_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "md5.h"

#define LEN 64

#define SLEEP_TIME  5 * 60

#define AUTH_ID "dm%sibank"

#if MD5POS == CPUID
#define GETCPUID_CMD "dmidecode -t 4 | grep ID | awk -F':' '{print $2}' | sed 's/ //g'"
#warning "cpuid"
#elif MD5POS == MAC
#define GETCPUID_CMD "ifconfig eth0 | grep 'HWaddr' | awk NR==1 | awk -F 'HWaddr ' '{print $2}' | sed 's/://g'"
#warning "mac"
#elif MD5POS == SATASN
#define GETCPUID_CMD "hdparm -i /dev/sda | grep 'SerialNo' | awk NR==1 | awk -F'SerialNo=' '{print $2}'"
#warning "satasn"
#endif

#define GETID "uci get system.@system[0].id"
#define GETAUTHID "uci get system.@system[0].auth"
#define SETID "uci set system.@system[0].id='%s'"
#define SETSTATUS1 "uci set system.@system[0].status='1'"
#define SETAUTHWORD1 "uci set system.@system[0].authword='已授权'"
#define SETSTATUS0 "uci set system.@system[0].status='0'"
#define SETAUTHWORD0 "set system.@system[0].authword='未授权'"
#define UCICOMMIT "uci commit system"

MD5_CTX MD5_C;

int uci_set_devid();
int uci_get(char *buf, char *msg);
int set_dm_msg(char *newbuf, char *oldbuf);
int set_MD5(char *MD5_msg, unsigned char *msg);
int auth();

#endif
