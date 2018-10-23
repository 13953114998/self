/*
 * =====================================================================================
 *
 *       Filename:  arg.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月15日 16时22分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __ARG_H__
#define __ARG_H__
#include "cmdline.h"

extern struct  gengetopt_args_info args_info;
void proc_args(int argc, char *argv[]);
char *strip(char *tmp);
char *next_entry(char *tmp);
#endif /* __ARG_H__ */
