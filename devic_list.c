#include "typeda.h"

/*设备链表头*/
struct list_head *dev_head;
int dev_list_count;/*节点数*/
pthread_mutex_t list_lock;/*链表总锁*/

/*初始化链表头*/
void Init_List_Head()
{
  if(!dev_head)
  {
    dev_head = (struct list_head *)calloc(1, sizeof(struct list_head));
    if(!dev_head)
      fun_error("init_list_head calloc");
    INIT_LIST_HEAD(dev_head);
    dev_list_count = 0;
    if(pthread_mutex_init(&list_lock, NULL))
      fun_error("init mutex list failed\n");
  }
  printf("init list success\n");
}

/*根据sockfd创建节点*/
bool add_devic_list_node(int sockfd)
{
  if(sockfd < 0)
    return false;
  dev_node_t *new_node = (dev_node_t *)calloc(1, sizeof(dev_node_t));
  if(!new_node)
    fun_error("add list node calloc");
  new_node->sockfd = sockfd;
  new_node->flameout = 0;
  new_node->devid_10 = NULL;
  new_node->devid_16 = NULL;

  if(pthread_mutex_init(&(new_node->lock), NULL))
    fun_error("add list node init mutex");
  pthread_mutex_lock(&list_lock);
  list_add_tail(&(new_node->node), dev_head);
  ++dev_list_count;
  pthread_mutex_unlock(&list_lock);
  return true;
}

/*根据fd返回节点*/
dev_node_t *sockfd_find_node(int sockfd)
{
  if(sockfd < 0)
    return NULL;

  pthread_mutex_lock(&list_lock);
  dev_node_t *tpos, *n;
  list_for_each_entry_safe(tpos, n, dev_head, node)
  {
    if(tpos->sockfd == sockfd)
    {
      pthread_mutex_unlock(&list_lock);
      return tpos;
    }
  }
  pthread_mutex_unlock(&list_lock);
  return NULL;
}

/*根据devid返回节点*/
dev_node_t *devid_16_find_node(char *devid)
{
  if(!devid)
    return NULL;

  pthread_mutex_lock(&list_lock);
  dev_node_t *tpos, *n;
  list_for_each_entry_safe(tpos, n, dev_head, node)
  {
    if(!strcmp(tpos->devid_16, devid))
    {
      pthread_mutex_unlock(&list_lock);
      return tpos;
    }
  }
  pthread_mutex_unlock(&list_lock);
  return NULL;
}

/*根据devid返回节点*/
dev_node_t *devid_10_find_node(char *devid)
{
  if(!devid)
    return NULL;

  pthread_mutex_lock(&list_lock);
  dev_node_t *tpos, *n;
  list_for_each_entry_safe(tpos, n, dev_head, node)
  {
    if(!strcmp(tpos->devid_10, devid))
    {
      pthread_mutex_unlock(&list_lock);
      return tpos;
    }
  }
  pthread_mutex_unlock(&list_lock);
  return NULL;
}

/*根据fd删除一个节点*/
void del_dev_list_node(int sockfd)
{
  dev_node_t *del = sockfd_find_node(sockfd);
  if(!del)
    return;
  pthread_mutex_lock(&list_lock);
  list_del(&(del->node));
  if(del->devid_10) free(del->devid_10);
  if(del->devid_16) free(del->devid_16);
  free(del);
  pthread_mutex_unlock(&list_lock);
}
