/*
 * ============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/13/2018 05:53:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>

#include "typeda.h"

/*在线监测数组*/
char online[1024];
pthread_mutex_t online_lock;

void fun_error(const char *str)
{
  if(NULL == str)
    return;
  perror(str);
  exit(-1);
}

void pthread_lock_init()
{
	//http 发送锁
	if(pthread_mutex_init(&http_lock, NULL))
	{
		printf("pthread_mutex_init failed\n");
		exit(-1);
	}
	//服务器下发查询油耗锁
	if(pthread_mutex_init(&oil_lock, NULL))
	{
		printf("pthread_mutex_init failed\n");
		exit(-1);
	}
	//服务器下发查询速度锁
	if(pthread_mutex_init(&speed_lock, NULL))
	{
		printf("pthread_mutex_init failed\n");
		exit(-1);
	}
	//在线数组锁
	if(pthread_mutex_init(&online_lock, NULL))
	{
		printf("pthread_mutex_init failed\n");
		exit(-1);
	}
}

/*设备在线监测线程*/
void *p_monitor_online()
{
	memset(&online, -1, 1024);
	while(1)
	{
		sleep(180);
		for(int i=0; i<1024; ++i)
		{
			if(online[i] != -1)
			{
				++online[i];
				if(online[i] >= 2)/*离线超过360s*/
				{
					pthread_mutex_lock(&online_lock);
					online[i] = -1;
					pthread_mutex_unlock(&online_lock);
					printf("a socket close.sock:%d\n", i);
					dev_node_t *node = sockfd_find_node(i);
					if(node)
					{
						if(node->devid_16)
						{
							online_msg_send(i, 0); /*离线上报*/	
						}
            del_dev_list_node(i);
					}
					close(i);
				}
			}
		}
	}
}

/*创建线程*/                                                                  
void safe_pthread_create(void *(*start_routine) (void *), void *arg)            
{                                                                               
	pthread_attr_t attr;                                                    
	pthread_t mythread;                                                     
	if(pthread_attr_init(&attr)) 
	{                                          
		printf("pthread_attr_init faild: %s(%d)\n",                     
			strerror(errno), errno);                                
		exit(-1);                                                       
	}                                                                       
	if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) 
	{       
		printf("pthread_setdetach faild: %s(%d)\n",                     
			strerror(errno), errno);                                
		exit(-1);                                                       
	}                                                                       
	if(pthread_create(&mythread, &attr, start_routine, arg)) 
	{              
		printf("pthread_create faild: %s(%d)\n",                        
			strerror(errno), errno);                                
		exit(-1);                                                       
	}
}


int main(int argc, char *argv[])
{
	Create_config();/*初始化配置参数*/
	pthread_lock_init();

	init_sockfd_msg();/*初始化tcp socket*/
	init_epoll();/*初始化epoll id*/
  Init_List_Head();/*初始化链表头*/ 

	safe_pthread_create(p_monitor_online, NULL);/*设备在线监测*/
	safe_pthread_create(p_tcp_recv_msg, NULL); /*接收处理tcp请求*/

	getchar();

	//建立mqtt通信
	struct mosquitto *mosq = NULL;

	mosquitto_lib_init();
	mosq = mosquitto_new(con_info->userid, con_info->clean_session, NULL);
	if(!mosq)
	{
		printf("mosquitto new failure!\n");
		mosquitto_lib_cleanup();
		return -1;
	}
	mosquitto_set(mosq);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);

	if(!con_info->mqtt_ip && !con_info->mqtt_port)
	{
		printf("con_info is  NULL");
		return -1;
	}
	if(mosquitto_connect(mosq, con_info->mqtt_ip, con_info->mqtt_port, 500))
	{
		printf("mosquitto_connect  failure\n");
		return -1;
	}

	mosquitto_loop_forever(mosq, -1, 1);
	
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return 0;
}
