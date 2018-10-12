/*
 * ============================================================================
 *
 *       Filename:  arg.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月15日 16时21分24秒
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
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "arg.h"

int debug = 0;
int daemon_mode = 0;
struct  gengetopt_args_info args_info;

char *strip(char *tmp)
{
	while(*tmp == ' ' || *tmp == '\t')
		tmp++;
	return tmp;
}

char *next_entry(char *tmp)
{
	while(*tmp != ' ' && *tmp != '\t' && *tmp != 0) {
		/* skip esc */
		if(*tmp == '\\' && *(tmp + 1) != 0)
			tmp++;
		tmp++;
	}
	return strip(tmp);
}

static int file_exist(char *filename)
{
	/*  try to open file to read */
	FILE *file;
	if ((file = fopen(filename, "r"))){
		fclose(file);
		return 1;
	}
	return 0;
}

void proc_args(int argc, char *argv[])
{
	int ret;
	struct cmdline_parser_params *params;
	params = cmdline_parser_params_create();

	params->initialize = 1;
	params->check_required = 0;
	ret = cmdline_parser_ext(argc, argv,
		&args_info, params);
	if(ret != 0) exit(ret);

	params->initialize = 0;
	params->override = 0;
	if(args_info.config_arg && 
		file_exist(args_info.config_arg)) {
		ret = cmdline_parser_config_file(
			args_info.config_arg,
			&args_info, params);
		if(ret != 0) exit(ret);
	}

	ret = cmdline_parser_required(&args_info, argv[0]);
	if(ret != 0) exit(ret);

	debug = args_info.debug_arg;
	//daemon_mode = args_info.daemon_flag;
	//printf("debug: %d, daemon_mode: %d\n", debug, daemon_mode);
	free(params);
}
