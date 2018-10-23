/*
 * ============================================================================
 *
 *       Filename:  tcp_action.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2018 10:46:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <netdb.h>

#include "typeda.h"

/*tcp write lock*/
pthread_mutex_t wri_lock;

/*tcp sockfd and epoll id*/
int tcp_sockfd = -1;
int epoll_sockfd = -1;

/*在线监测数组*/
extern char online[1024];
extern pthread_mutex_t online_lock;

/*10进制转换16进制*/
char *inttox(int num)
{
	char buf[3] = {0};
	if(num/16 < 10)
		buf[0] = num/16 + '0';
	else
		buf[0] = num/16 - 10 + 'A';
	if(num%16 < 10)
		buf[1] = num%16 + '0';
	else
		buf[1] = num%16 - 10 + 'A';
	buf[2] = '\0';
	return buf;
}
/*16进制转换10进制*/
unsigned int xtoint(unsigned char str)
{
	unsigned int tmp = (unsigned int)str;
	return tmp;
}

/*格林威治时间转换北京时间
 *传入t为三字节16进制日期格式 例如
 *0x13 0x00 0x04为13点0分4秒
 */
unsigned char *change_time_zone(int *tmp, const unsigned char *t)
{
  if(!t)
    return NULL;
  unsigned char *time = (unsigned char *)calloc(1, 4);
  if(t[0] >= 0x16)
  {
    time[0] = t[0] - 0x16;
    *tmp = 0;
  }else
  {
    time[0] = t[0] + 0x08;
    *tmp = -1;
  }
  time[1] = t[1];
  time[2] = t[2];
  return time;
}
/*格林威治日期转换北京日期
 *传入三字节16进制日期格式，默认为日期加1
 *日月年表示，年省略20
 */
unsigned char *change_date_zone(const unsigned char *d)
{
  if(!d)
    return NULL;
  unsigned char *date = (unsigned char *)calloc(1, 4);

  if(d[0] >= 0x28)
  {
    if(d[1] == 0x02)//二月
    {
      unsigned int year = d[2] + 2000;
      if((year%4==0)&&(year%100==0)||(year%400==0))//闰年
      {
        if(d[0] >= 0x29)
        {
          date[0] = d[2];
          date[1] = d[1] + 0x01;
          date[2] = d[0] - 0x28;
        }else
          goto DATE;
      }else
      {
        date[0] = d[2];
        date[1] = d[1] + 0x01;
        date[2] = d[0] - 0x27;
      }
    }else if(d[1] == 0x04 || d[1] == 0x06 || d[1] == 0x09 || d[1] == 0x10)//30天
    {
      if(d[0] >= 30)
      {
        date[0] = d[2];
        date[1] = d[1] + 0x01;
        date[2] = d[0] - 0x29;
      }else
        goto DATE;
    }else//31天
    {
      if(d[0] >= 31)
      {
        if(d[1] == 0x12)
        {
          date[0] = d[2] + 0x01;
          date[1] = d[1] - 0x11;
          date[2] = d[0] - 0x30;
        }else
        {
          date[0] = d[2];
          date[1] = d[1] + 0x01;
          date[2] = d[0] - 0x30;
        }
      }else
        goto DATE;
    }
  }else
  {
DATE:
    date[0] = d[2];
    date[1] = d[1];
    date[2] = d[0] + 0x01;
  }
  return date;
}

/*初始化套接字*/
void init_sockfd_msg()
{
	if(tcp_sockfd == -1)
	{
		tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(tcp_sockfd == -1)
		{
			printf("cteate sockfd failure\n");
			exit(-1);
		}

    int reuse = 1;
    if(setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
      perror("setsockopt");
      exit(-1);
    }
	}
	struct sockaddr_in ser_addr;
	memset(&ser_addr, 0, sizeof(struct sockaddr_in));
	ser_addr.sin_family = AF_INET;
	if(con_info->tcp_port)
		ser_addr.sin_port = htons(con_info->tcp_port);
	else{
		printf("con_info is null\n");
		ser_addr.sin_port = htons(PORT);
	}
	printf("bind port is %d\n", con_info->tcp_port);

	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(tcp_sockfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)))
	{
		printf("bind tcp_sockfd failure\n");
		exit(-1);
	}
	if(pthread_mutex_init(&wri_lock, NULL))
	{
		printf("pthread_mutex_init() failed\n");
		exit(-1);
	}
	printf("init tcp sockfd and bind\n");
}
void init_epoll()
{
	if(epoll_sockfd == -1)
		epoll_sockfd = epoll_create(1);
	if(epoll_sockfd == -1)
	{
		printf("epoll create failure\n");
		exit(-1);
	}
	printf("Create epoll\n");
}
/*与设备交互线程*/
void *p_tcp_recv_msg()
{
	
	struct epoll_event event;
	struct epoll_event *event_num;

	/*listen*/
	if(listen(tcp_sockfd, con_info->max_listen))
	{
		printf("listen failure\n");
		exit(-1);
	}
	event.data.fd = tcp_sockfd;
	event.events = EPOLLIN | EPOLLET;
	if(epoll_ctl(epoll_sockfd, EPOLL_CTL_ADD, tcp_sockfd, &event))
	{
		printf("epoll failure\n");
		exit(-1);
	}
	event_num = calloc(con_info->max_event, sizeof(struct epoll_event));
	if(!event_num)
	{
		printf("calloc failure\n");
		exit(-1);
	}

	while(1)
	{
		int num = 0;
		printf("epoll wait---------\n");
		num = epoll_wait(epoll_sockfd, event_num, con_info->max_event, -1);
		for(int i=0; i < num; ++i)
		{
			if((event_num[i].events & EPOLLERR) || 
				(event_num[i].events & EPOLLHUP) ||
				(!(event_num[i].events & EPOLLIN)))
			{
				printf("epoll sockfd %d error\n", event_num[i].data.fd);
				close(event_num[i].data.fd);
				continue;
			}
			else if(event_num[i].data.fd == tcp_sockfd)
			{/*检测到新设备连接*/
					struct sockaddr in_addr;
					socklen_t in_len;
					int in_sockfd = -1;
					char host_buf[NI_MAXHOST], port_buf[NI_MAXSERV];
					in_len = sizeof(struct sockaddr);                           
					in_sockfd = accept(tcp_sockfd, &in_addr, &in_len);          
					if(in_sockfd == -1)                                         
					{                                                           
							printf("accept failure\n");                         
							exit(-1);                                              
					}                                                           
					if(!getnameinfo(&in_addr, in_len,
										host_buf, sizeof(host_buf),                         
										port_buf, sizeof(port_buf),                         
										NI_NUMERICHOST | NI_NUMERICSERV))                   
					{
						printf("accept connection on descriptor %d"
								"(host=%s, port=%s)\n", in_sockfd, host_buf, port_buf);
					}
					memset(&event, 0, sizeof(event));
					event.data.fd = in_sockfd;
					event.events = EPOLLIN | EPOLLET;
					if(epoll_ctl(epoll_sockfd, EPOLL_CTL_ADD, in_sockfd, &event))
					{
						printf("epoll_ctl Add failure\n");
						exit(-1);
					}
	
					pthread_mutex_lock(&online_lock);
					online[in_sockfd] = 0;
					pthread_mutex_unlock(&online_lock);

          if(!add_devic_list_node(in_sockfd))
            break;
			}
			else
			{
				size_t count;
				int key = 0;
				struct dev_data *msg_buf = (struct dev_data *)calloc(1, sizeof(struct dev_data));
				if(!msg_buf)
				{
					printf("epoll calloc failed \n");
					exit(-1);
				}
				msg_buf->buf = (unsigned char *)calloc(1, 1024);
				if(!msg_buf->buf)
				{
					printf("calloc failed\n");
					exit(-1);
				}

				count = read(event_num[i].data.fd, msg_buf->buf, 1024);
				if(count == -1)
				{
					if(errno != EAGAIN)
					{
						printf("read failure\n");
						key = 1;
					}
				}
				else if(count == 0)
				{
					key = 1;
				}
				else{
					printf("-----------read MAG length is:%d\n", count);
	
					msg_buf->sockfd = event_num[i].data.fd;
					msg_buf->len = count;
					
					safe_pthread_create(p_recv_dev_message, msg_buf);
				}
				/* 断开tcp连接*/
				if(key)
				{
					printf("a socket close . sock: %d\n", event_num[i].data.fd);
					del_dev_list_node(event_num[i].data.fd);

					pthread_mutex_lock(&online_lock);
					online[event_num[i].data.fd] = -1;
					pthread_mutex_unlock(&online_lock);

					close(event_num[i].data.fd);
				}
			}
		}
	}
	free(event_num);                                                            
	close(epoll_sockfd);                                                        
	printf("epoll end\n");                                                      
}             

/*向设备发送消息*/
void send_msg_to_dev(int sockfd, unsigned char *str, int len)
{
	if(!str)
	{
		printf("str is nullstr\n");
		return;
	}
	printf("sockfd : %d\n", sockfd);
	for(int i=0; i<len; ++i)
	{
		printf(" %02x", str[i]);
	}
	printf("\n");

	pthread_mutex_lock(&wri_lock);
	int ret = write(sockfd, str, len);
	pthread_mutex_unlock(&wri_lock);
	if(ret <= 0)
	{
		printf("write failed\n");
		return;
	}else{
		printf("send success\n");
	}
}

/*解析设备发送来的数据线程*/
void *p_recv_dev_message(struct dev_data *msg_buf)
{
	if(!msg_buf)
	{
		printf("msg_buf is NULL\n");
		return NULL;
	}

	unsigned char check = 0;
	unsigned char tmp = 0;
	unsigned short int leng = 0;
	unsigned char *buf = NULL;
	int j = 0;
	for(int i=0; i<msg_buf->len; ++i)
	{
		if(buf)
		{
			if(msg_buf->buf[i+1] == 0x29)
			{
				check = msg_buf->buf[i];
				break;
			}
			if(msg_buf->buf[i] == 0x3d)
			{
				tmp = msg_buf->buf[i]^msg_buf->buf[i+1];
				if(msg_buf->buf[i+2] == 0x29)
				{
					check = tmp;
					break;
				}else{
					buf[j] = tmp;
					++i;
					++j;
				}
			}
			else{
				memcpy(&buf[j], &msg_buf->buf[i], 1);
				++j;
			}
		}
		if(msg_buf->buf[i] == 0x28)
		{
			leng = msg_buf->buf[9+i] << 8 | msg_buf->buf[10+i];
			buf = (unsigned char *)calloc(1, leng+11);
			
			if(!buf)
			{
				printf("calloc failure\n");
				return NULL;
			}
		}
	}
	if(buf)
	{
		printf("check:%x\n", check);
		/*校验*/
		unsigned int number = buf[0];
		for(int i=1; i<leng+10; ++i)
		{
			number = number^buf[i];
		}
		printf("number:%x\n", number);
		if(check == number)
		{	
			slation_data(msg_buf->sockfd, buf, leng+11);
		}
		else
		{	
			printf("a message failed(sockfd:%d)\n", msg_buf->sockfd);
		}
	}else{
		printf("无法识别的信息,丢弃(buf: %s)\n", msg_buf->buf);
	}

	if(buf) free(buf);
	if(msg_buf)
	{	
		if(msg_buf->buf) free(msg_buf->buf);
		free(msg_buf);
	}
}

void slation_data(int sockfd, unsigned char *buf, int buf_len)
{
	if(!buf)
	{
		printf("buf is null\n");
		return;
	}
	for(int i = 0; i<buf_len-1; ++i)
	{
		printf(" %02x", buf[i]);
	}
	printf("\n");
	int script = 0;
	char dev_id[7] = {0};
	memcpy(dev_id, &buf[script], 6);
	script+=6;
	char id[7] = {0};
	sprintf(id, "%02x%02x%02x%02x%02x%02x", 
              dev_id[0], dev_id[1], dev_id[2], dev_id[3], dev_id[4], dev_id[5]);
	printf("dev_id:%s\n", id);

	dev_node_t *node = sockfd_find_node(sockfd);
	if(!node) return;
	else if(!node->devid_16)
	{
		/*之前存在的节点，未删除又重新连接*/
		dev_node_t *new_node = devid_16_find_node(dev_id);
		if(new_node)
		{
      pthread_mutex_lock(&new_node->lock);
			new_node->sockfd = sockfd;
      pthread_mutex_unlock(&new_node->lock);
		}else{
      pthread_mutex_lock(&node->lock);
      node->devid_16 = strdup(dev_id);
      node->devid_10 = strdup(id);
      pthread_mutex_unlock(&node->lock);
			online_msg_send(sockfd, 1); /*设备上线上报*/
		}
	}

	unsigned char type = buf[script];
	++script;
	unsigned char num = buf[script];
	script+=3;
	if(type == M_CURRENCY)
	{
		printf("通用命令！num:%02x\n", num);
		switch(num)
		{
			printf("此信息暂时无法解析(type:%02x %02x)\n", type, num);
		}
	}
	else if(type == M_GSMMESSAGE)
	{
		printf("GSM命令！num:%02x\n", num);
		switch(num)
		{
			case G_GSM_HEART_BEAT:
				printf("heart brat message\n");
				pthread_mutex_lock(&online_lock);
				online[sockfd] = 0;
				pthread_mutex_unlock(&online_lock);
				break;
			default:
				printf("此信息暂时无法解析(type:%02x %02x)\n", type, num);
				break;
		}
	}
	else if(type == M_GPSMESSAGE)
	{
		char http_str[256] = {0};
		printf("GPS命令！num:%02x\n", num);	
		switch(num)
		{
			case G_GPS_OBD_MESSAGE:/*GPS+OBD混合信息上传*/
				if(sockfd == speed_message)
				{
					pthread_mutex_lock(&speed_lock);
					speed_message = 0;
					pthread_mutex_unlock(&speed_lock);
					printf("GPS OBD混合信息上传\n");
					script+=34;
					unsigned int speed = xtoint(buf[script]);
					printf("速度为：%u\n", speed);
					snprintf(http_str, 256, 
							"{\"devicid\":\"%s\",\"msgtype\":\"%d\","
							"\"speed\":\"%d\"}", id, T_GET_CAT_SPEED, speed);

					http_send(con_info->http_speed, http_str);
					break;
				}else
				{
					script+=3;
					unsigned char date[3] = {0};
					memcpy(date, &buf[script], 3);
					script+=3;

          int sign = 0;
					unsigned char time[3] = {0};
					memcpy(time, &buf[script], 3);
          unsigned char *ch_date;
          unsigned char *ch_time = change_time_zone(&sign, time);
          if(!sign)
            ch_date = change_date_zone(date);
					script+=3;

					unsigned char latitude[4] = {0};
					memcpy(latitude, &buf[script], 4);
					script+=4;
					unsigned char longitude[4] = {0};
					memcpy(longitude, &buf[script], 4);
					script+=4;
					unsigned char tmp = buf[script]/0x10;
          if(!sign)
          {
					  snprintf(http_str, 256, 
							  "{\"devicid\":\"%s\",\"msgtype\":\"%d\","
							  "\"data\":\"%s\",\"time\":\"%s\", "
							  "\"latitude\":\"%02x%02x%02x%02x\",\"longitude\":\"%02x%02x%02x%02x%01x\"}", 
								  id, T_GET_GPS_POSITION_YES, 
								  ch_date, ch_time, 
								  latitude[0], latitude[1], latitude[2], latitude[3],
								  longitude[0], longitude[1], longitude[2], longitude[3], tmp);
          }else
          {
					  snprintf(http_str, 256, 
							  "{\"devicid\":\"%s\",\"msgtype\":\"%d\","
							  "\"data\":\"%02x%02x%02x\",\"time\":\"%s\", "
							  "\"latitude\":\"%02x%02x%02x%02x\",\"longitude\":\"%02x%02x%02x%02x%01x\"}", 
								  id, T_GET_GPS_POSITION_YES, 
								  date[2], date[1], date[0], ch_time, 
								  latitude[0], latitude[1], latitude[2], latitude[3],
								  longitude[0], longitude[1], longitude[2], longitude[3], tmp);
          }
          if(ch_time) free(ch_time);
          if(ch_date) free(ch_date);
					http_send(con_info->http_gps, http_str);
					break;
				}
			default:
				printf("此信息暂时无法解析(type:%02x %02x)\n", type, num);
				break;
		}
	}
	else if(type == M_OBDMESSAGE)
	{
		double fuel;
		char http_str[256] = {0};
		printf("OBD命令！num:%02x\n", num);
		switch(num)
		{
			case O_OBD_OIL_CONSUMPTION:/*平均油耗信息回复*/
				if(sockfd != oil_message)
					break;
				pthread_mutex_lock(&oil_lock);
				oil_message = 0;
				pthread_mutex_unlock(&oil_lock);
				fuel = xtoint(buf[script])/10.0;
				printf("获得油耗信息：%.1lf\nh/100km", fuel);
				snprintf(http_str, 256, 
						"{\"devicid\":\"%s\",\"msgtype\":\"%d\","
						"\"average\":\"%.1lf\"}", id, T_GET_CAT_AVERAGE_OIL, fuel);

				http_send(con_info->http_oil, http_str);
				break;
			case O_OBD_IGNITE_REPORT:/*点火熄火报告*/
				printf("点火熄火报告！\n");
				dev_node_t *flnode = sockfd_find_node(sockfd);
				if(!flnode)	break;
				if(buf[script] == 0)
				{
					flnode->flameout = 0;
				}
				else
				{
					flnode->flameout = 1;
				}
				break;
			default:
				printf("此信息暂时无法解析(type:%02x %02x)\n", type, num);
				break;
		}
	}
	else if(type == M_UPGRADE)
	{
		printf("升级命令！num:%02x\n", num);
		switch(num)
		{
			printf("此信息暂时无法解析(type:%02x %02x)\n", type, num);
		}
	}
	else if(type == M_EXTERNAL)
	{
		printf("外设命令！num:%02x\n", num);
		switch(num)
		{
			printf("此信息暂时无法解析(type:%02x %02x)\n", type, num);
		}
	}
	else
	{
		printf("无法识别的命令！type:%02x %02x\n",type, num);
	}
}
