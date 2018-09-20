#ifndef _DEVIC_LIST_H_
#define _DEVIC_LIST_H_

typedef struct node{
  struct list_head node;

  int sockfd;           /*套接字*/
  int flameout;         /*熄火点火状态（0 表示熄火 1 表示点火）*/
  char *devid_16;       /*16进制设备交互id*/
  char *devid_10;       /*10进制后台交互id*/
  pthread_mutex_t lock; /*节点锁*/
}dev_node_t;

void Init_List_Head();
bool add_devic_list_node(int sockfd);
dev_node_t *sockfd_find_node(int sockfd);
dev_node_t *devid_16_find_node(char *devid);
dev_node_t *devid_10_find_node(char *devid);
void del_dev_list_node(int sockfd); 

#endif
