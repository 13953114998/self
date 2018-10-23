#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "arg.h"
#include "cJSON.h"

int main(int argc,char *argv[])
{
	proc_args(argc, argv);
	int socketfd, i = 0;
	socklen_t addr_len;
	char buf[2064];
	char now_time[20] = {0};
	struct sockaddr_in server_addr;
	if((socketfd = socket(PF_INET,SOCK_DGRAM,0)) < 0) {
		perror("socket");
		exit(-1);
	}
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(args_info.uhost_arg);
	if (args_info.recev_type_arg == 0) {
		printf("rec IMSI, bind port:%d\n", args_info.uport_arg);
		server_addr.sin_port = htons(args_info.uport_arg);
	} else if (args_info.recev_type_arg == 1) {
		printf("rec WIFI, bind port:%d\n", args_info.uport_arg +1);
		server_addr.sin_port = htons(args_info.uport_arg + 1);
	} else if (args_info.recev_type_arg == 2) {
		printf("rec UE, bind port:%d\n", args_info.ueport_arg);
		server_addr.sin_port = htons(args_info.ueport_arg);
	}

	addr_len = sizeof(server_addr);
	int ret = 0;

	if(bind(socketfd,(struct sockaddr*)&server_addr,addr_len) < 0) {
		perror("bind");
		exit(-1);
	}
	printf("start to recv information ...\n");

	while(1) {
		memset(buf, '\0', 2064);
		if((ret = recvfrom(socketfd, buf, 2064, 0,(struct sockaddr*)&server_addr,&addr_len)) < 0) {
			perror("sendrto");
			exit(-1);
		}
		//printf("from: %s port:%d ,recv length %d buf:[%s]\n",inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port),ret,&buf[13]);
		cJSON *object = cJSON_Parse(buf);
		if (object) {
			cJSON *timetamp = cJSON_GetObjectItem(object, "timestamp");
			if (timetamp) {
				time_t rawtime;
				struct tm *timeinfo;
				rawtime = atol(timetamp->valuestring);
				timeinfo = localtime(&rawtime);
				snprintf(now_time, 20,
					"%04d-%02d-%02d %02d:%02d:%02d",
					timeinfo->tm_year + 1900,
					timeinfo->tm_mon + 1, timeinfo->tm_mday,
					timeinfo->tm_hour, timeinfo->tm_min,
					timeinfo->tm_sec);
			}
			cJSON_Delete(object);
		}
		printf("%s %s\n", now_time, buf);
		memset(now_time, 0, 20);
	}
	close(socketfd);
	return 0;
}
