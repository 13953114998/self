/*
 * ============================================================================
 *
 *       Filename:  bt_model_action.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/02/2018 10:08:32 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <pthread.h>
#include "config.h"
#include "cJSON.h"
#include "hash_list_active.h"
#include "bt_model_action.h"
#include "baicell_net.h"
#include "tcp_devid.h"
#include "arg.h"
track_config_t *track_cfg = NULL;
static int ue_send_flag = 0; /* 是否发送UE测量信息，0-否，1-是 */
static int readline(FILE *file, char *buf, int bufsize);
extern int sockfd;

/*蓝牙发送锁*/
pthread_mutex_t blue_lock;

int get_ue_send_flag()
{
	return ue_send_flag;
}
void set_ue_send_flag(int f)
{
	ue_send_flag = f;
}
static int imsi_send_flag = 0;/* 是否发送IMSI信息，0-否，1-是 */ 
int get_imsi_send_flag()
{
	return imsi_send_flag;
}
/* 向蓝牙发送信息 */
int write_to_blue_tooth(int fd, char *str)
{
	int ret = 0;
	ret += write(fd, REP_START, strlen(REP_START));
	ret += write(fd, str, strlen(str));
	ret += write(fd, REP_STOP, strlen(REP_STOP));
	return ret;
}

/* 设置串口配置 */
int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newttys1,oldttys1;
	if(tcgetattr(fd,&oldttys1)!=0) {    /*保存原先串口配置*/
		perror("Setupserial 1");
		return -1;
	}

	bzero(&newttys1,sizeof(newttys1)); /*将一段内存区域的内容全清为零*/
	newttys1.c_cflag|=(CLOCAL|CREAD ); /*CREAD 开启串行数据接收,
					     CLOCAL并打开本地连接模式*/   

	newttys1.c_cflag &=~CSIZE;  /*设置数据位数*/
	switch(nBits) {    /*选择数据位*/  
	case 7:
		newttys1.c_cflag |=CS7;
		break;
	case 8:
		newttys1.c_cflag |=CS8;
		break;
	}
	switch( nEvent ) {   /*设置校验位 */ 
	case '0':       /*奇校验  */
		newttys1.c_cflag |= PARENB;             /*开启奇偶校验 */ 
		newttys1.c_iflag |= (INPCK | ISTRIP);   /*INPCK打开输入奇偶校验;
							  ISTRIP去除字符的第八个比特*/  
		newttys1.c_cflag |= PARODD;             /*启用奇校验(默认为偶校验)*/  
		break;
	case 'E' :       /*偶校验  */
		newttys1.c_cflag |= PARENB;             /*开启奇偶校验 */ 
		newttys1.c_iflag |= ( INPCK | ISTRIP);  /*打开输入奇偶校验
							  并去除字符第八个比特 */
		newttys1.c_cflag &= ~PARODD;            /*启用偶校验；  */
		break;
	case 'N':     /*关闭奇偶校验*/
		newttys1.c_cflag &= ~PARENB;
		break;
	}

	switch( nSpeed ) {       /*设置波特率 */ 
	case 2400:
		cfsetispeed(&newttys1, B2400);    /*设置输入速度*/
		cfsetospeed(&newttys1, B2400);    /*设置输出速度*/
		break;
	case 4800:
		cfsetispeed(&newttys1, B4800);
		cfsetospeed(&newttys1, B4800);
		break;
	case 9600:
		cfsetispeed(&newttys1, B9600);
		cfsetospeed(&newttys1, B9600);
		break;
	case 115200:
		cfsetispeed(&newttys1, B115200);
		cfsetospeed(&newttys1, B115200);
		break;
	default:
		cfsetispeed(&newttys1, B9600);
		cfsetospeed(&newttys1, B9600);
		break;
	}

	if( nStop == 1) {  /* 设置停止位；
			    * 若停止位为1，则清除CSTOPB;
			    * 若停止位为2，则激活CSTOPB
			    * */ 
		newttys1.c_cflag &= ~CSTOPB;      /*默认为送一位停止位； */
	} else if( nStop == 2) {
		newttys1.c_cflag |= CSTOPB;       /*CSTOPB表示送两位停止位；*/
	}

	/*设置最少字符和等待时间，对于接收字符和等待时间没有特别的要求时*/
	newttys1.c_cc[VTIME] = 0; /*非规范模式读取时的超时时间； */ 
	newttys1.c_cc[VMIN]  = 0;  /*非规范模式读取时的最小字符数；*/

	tcflush(fd ,TCIFLUSH); /* tcflush清空终端未完成的输入/输出请求及数据；
				* TCIFLUSH表示清空正收到的数据，且不读取出来
				* */
	/* 在完成配置后，需要激活配置使其生效*/
	/* TCSANOW不等数据传输完毕就立即改变属性 */
	if((tcsetattr( fd, TCSANOW,&newttys1))!=0) {
		perror("com set error");
		return -1;
	}
	return 0;
}

/* 蓝牙相关 */
static int bltooth_fd = -1;

int get_bluetooth_fd()
{
	return bltooth_fd;
}
void set_bluetooth_fd(int fd)
{
	bltooth_fd = fd;
}

void init_bluetooth_device_fd()
{
	int fd = open(BT_DEV_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		perror("open model "BT_DEV_NAME"error!");
		exit(1);
	}
	fcntl(fd, F_SETFL, 0);
	set_opt(fd, 9600, 8, 'N', 1);
	set_bluetooth_fd(fd);
	return;
}

/* 扫频模块相关 */
int scan_model_fd = -1;
int get_scan_model_fd()
{
	return scan_model_fd;
}
void set_scan_model_fd(int fd)
{
	scan_model_fd = fd;
}
int init_scan_model_device_fd()
{
	int fd;
	fd = open( MODEL_DEV_NAME, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd == -1) {
		perror("open model "MODEL_DEV_NAME"error!");
    printf("device reboot\n");
    sleep(5);
		sync();
    reboot(RB_AUTOBOOT);
	}
	fcntl(fd, F_SETFL, 0);
	set_opt(fd, 115200, 8, 'N', 1);
	set_scan_model_fd(fd);
	return fd;
}

int config_location_set_ue_config()
{
	wrFLLmtToEnbMeasUecfg_t pStr;
	memset(&pStr, '\0', sizeof(pStr));
	wrMsgHeader_t WrmsgHeader;
	WrmsgHeader.u16MsgType = O_FL_LMT_TO_ENB_MEAS_UE_CFG;
	WrmsgHeader.u32FrameHeader = MSG_FRAMEHEAD_DEF;
	WrmsgHeader.u16frame =
		(entry->work_mode == TDD)?SYS_WORK_MODE_TDD:SYS_WORK_MODE_FDD;
	WrmsgHeader.u16SubSysCode = SYS_CODE_DEFAULT;
	WrmsgHeader.u16MsgLength = sizeof(pStr);

	pStr.u8WorkMode = 2; /* 定位模式 */
	pStr.u8SubMode = 1;
	memcpy(pStr.IMSI, track_cfg->imsi, IMSI_LENGTH);
	pStr.u8MeasReportPeriod = RPT_PERIOD_640MS;/* UE测量信息上报周期 */
	pStr.SchdUeMaxPowerTxFlag = track_cfg->ue_maxpower_switch;/* 最大功率开关 */
	pStr.SchdUeMaxPowerValue = 23;/* UE最大功率 */
	pStr.CampOnAllowedFlag = 0; /* 是否踢非定位用户回公网 0 shi */
#ifdef BIG_END
	WrmsgHeader.u16MsgType = my_htons_ntohs( WrmsgHeader.u16MsgType);
	WrmsgHeader.u32FrameHeader = my_htonl_ntohl(WrmsgHeader.u32FrameHeader);
	WrmsgHeader.u16frame = my_htons_ntohs(WrmsgHeader.u16frame);
	WrmsgHeader.u16SubSysCode = my_htons_ntohs(WrmsgHeader.u16SubSysCode);
	WrmsgHeader.u16MsgLength = my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
	pStr.WrmsgHeaderInfo = WrmsgHeader;
	
	unsigned char *send_str = calloc(1, sizeof(pStr) + 1);
	memcpy(send_str, &pStr, sizeof(pStr));
	int ret = baicell_send_msg_to_band(send_str, sizeof(pStr), NULL);
	if (ret == -1) {
		free(send_str);
		perror("send value failed！");
		return 0;
	}
	free(send_str);
	while (1) {
		sleep(1);
		if (track_cfg->config_status == CFG_STATUS_SET_UE_OK)
			break;
	}
	return 1;

}
int config_location_check_cell_info()
{
	if ((track_cfg->sysdarfcn && track_cfg->sysdarfcn == entry->sysDlARFCN)
		|| !memcmp(track_cfg->plmn, entry->PLMN, PLMN_LENGTH)) {
		track_cfg->config_status = CFG_STATUS_SET_ARFCN_OK;
		return 1;
	}
	wrMsgHeader_t WrmsgHeader;
	WrmsgHeader.u16MsgType = O_FL_LMT_TO_ENB_SYS_ARFCN_CFG;
	WrmsgHeader.u32FrameHeader = MSG_FRAMEHEAD_DEF;
	WrmsgHeader.u16frame =
		(entry->work_mode == TDD)?SYS_WORK_MODE_TDD:SYS_WORK_MODE_FDD;
	WrmsgHeader.u16SubSysCode = SYS_CODE_DEFAULT;

	wrFLLmtToEnbSysArfcnCfg_T pStr;
	memset(&pStr, '\0', sizeof(pStr));
	pStr.sysDlARFCN = track_cfg->sysdarfcn;
	memcpy(pStr.PLMN, track_cfg->plmn, PLMN_LENGTH);
	WrmsgHeader.u16MsgLength = sizeof(pStr);
	/* 未设置频点(自动定位时不进行指定), 根据plmn自动配置 */
	if (pStr.sysDlARFCN == 0) {
		if (!memcmp(pStr.PLMN, CHINA_MOBILE_LTE_PLMN, PLMN_LENGTH)) {
			pStr.sysUlARFCN = 255;
			pStr.sysDlARFCN = 37900;
			pStr.sysBand = 38;
		} else if (!memcmp(pStr.PLMN, CHINA_UNICOM_LTE_PLMN, PLMN_LENGTH)) {
			pStr.sysUlARFCN = 19650;
			pStr.sysDlARFCN = 1650;
			pStr.sysBand = 3;
		} else if (!memcmp(pStr.PLMN, CHINA_NET_LTE_PLMN, PLMN_LENGTH)) {
			pStr.sysUlARFCN = 19825;
			pStr.sysDlARFCN = 1825;
			pStr.sysBand = 3;
		}
	} else { /* 指定频点(手动导入频点) */
		if (pStr.sysDlARFCN <= 599) { 
			pStr.sysBand = 1;
		} else if (1200 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 1949) {
			pStr.sysBand = 3;
		} else if (2400 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 2649) {
			pStr.sysBand = 5;
		} else if (2650 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 2749) {
			pStr.sysBand = 6;
		} else if (2750 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 3449) {
			pStr.sysBand = 7;
		} else if (3450 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 3799) {
			pStr.sysBand = 8;
		} else if (3800 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 4149) {
			pStr.sysBand = 9;
		} else if (37750 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 38249) {
			pStr.sysBand = 38;
		} else if (38250 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 38649) {
			pStr.sysBand = 39;
		} else if (38650 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 39649) {
			pStr.sysBand = 40;
		} else if (39650 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 41589) {
			pStr.sysBand = 41;
		} else if (41590 <= pStr.sysDlARFCN &&
			pStr.sysDlARFCN <= 43589) {
			pStr.sysBand = 42;
		}
	}
	if (track_cfg->cell_pci)
		pStr.PCI = track_cfg->cell_pci;
	else
		pStr.PCI = 501; /*0~503*/
	pStr.sysBandwidth = 25;/*wrFLBandwidth*/
	pStr.TAC = 4366;
	pStr.CellId = 12364;
	pStr.UePMax = 23; /*<=23dBm*/
	pStr.EnodeBPMax = 20;
#ifdef BIG_END
	WrmsgHeader.u16MsgType = my_htons_ntohs( WrmsgHeader.u16MsgType);
	WrmsgHeader.u32FrameHeader = my_htonl_ntohl(WrmsgHeader.u32FrameHeader);
	WrmsgHeader.u16frame = my_htons_ntohs(WrmsgHeader.u16frame);
	WrmsgHeader.u16SubSysCode = my_htons_ntohs(WrmsgHeader.u16SubSysCode);
	WrmsgHeader.u16MsgLength = my_htons_ntohs(WrmsgHeader.u16MsgLength);

	pStr.sysUlARFCN = my_htonl_ntohl(pStr.sysUlARFCN);
	pStr.sysDlARFCN = my_htonl_ntohl(pStr.sysDlARFCN);
	pStr.sysBand = my_htonl_ntohl(pStr.sysBand);
	pStr.PCI = my_htons_ntohs(pStr.PCI);
	pStr.TAC = my_htons_ntohs(pStr.TAC);
	pStr.CellId = my_htonl_ntohl(pStr.CellId);
	pStr.UePMax = my_htons_ntohs(pStr.UePMax);
	pStr.EnodeBPMax = my_htons_ntohs(pStr.EnodeBPMax);
#endif
	pStr.WrmsgHeaderInfo = WrmsgHeader;
	unsigned char *send_str = calloc(1, sizeof(pStr) + 1);
	if (send_str == NULL) {
		perror("malloc send str error!");
		return 0;
	}
	memcpy(send_str, &pStr, sizeof(pStr));
	int ret = baicell_send_msg_to_band(send_str, sizeof(pStr), NULL);
	if (ret == -1) {
		perror("send value failed！");
		free(send_str);
		return 0;
	}
	free(send_str);
	char control_pa[128] = {0};
	snprintf(control_pa, 128, "%s %d", CONTROL_PA_CMD, pStr.sysBand);
	return 1;
}

int config_location_check_sysmode()
{
	printf("Auto location set work mode!\n");
	if (track_cfg->work_mode == entry->work_mode) {
		printf("track_cfg->work_mode(%u) == entry->work_mode(%u)",
			track_cfg->work_mode, entry->work_mode);
		track_cfg->config_status = CFG_STATUE_SET_WORK_MODE_OK;
		return 1;
	}
	wrFLLmtToEnbSysModeCfg_t pStr;
	pStr.sysMode = track_cfg->work_mode;

	wrMsgHeader_t WrmsgHeader;
	WrmsgHeader.u16MsgType = O_FL_LMT_TO_ENB_SYS_MODE_CFG;
	WrmsgHeader.u32FrameHeader = MSG_FRAMEHEAD_DEF;
	WrmsgHeader.u16frame =
		(entry->work_mode == TDD)?SYS_WORK_MODE_TDD:SYS_WORK_MODE_FDD;
	WrmsgHeader.u16SubSysCode = SYS_CODE_DEFAULT;
	WrmsgHeader.u16MsgLength = sizeof(pStr);
#ifdef BIG_END
	pStr.sysMode = my_htonl_ntohl(pStr.sysMode);
	WrmsgHeader.u16MsgType = my_htons_ntohs( WrmsgHeader.u16MsgType);
	WrmsgHeader.u32FrameHeader = my_htonl_ntohl(WrmsgHeader.u32FrameHeader);
	WrmsgHeader.u16frame = my_htons_ntohs(WrmsgHeader.u16frame);
	WrmsgHeader.u16SubSysCode = my_htons_ntohs(WrmsgHeader.u16SubSysCode);
	WrmsgHeader.u16MsgLength = my_htons_ntohs(WrmsgHeader.u16MsgLength);
#endif
	pStr.WrmsgHeaderInfo = WrmsgHeader;
	unsigned char *send_str = (unsigned char *)calloc(1, sizeof(pStr) + 1);
	if (send_str == NULL) {
		perror("malloc send str error!");
		return 0;
	}
	memcpy(send_str, &pStr, sizeof(pStr));
	int ret = baicell_send_msg_to_band(send_str, sizeof(pStr), NULL);
	if (ret == -1) {
		free(send_str);
		perror("send value failed！");
		return 0;
	}
	free(send_str);
	entry->online = 0;
	entry->change_count = -2;
	while (1) {
		sleep(1);
		if (track_cfg->work_mode == entry->work_mode)
			break;
	}
	return 1;
}

int config_location_order()
{
	if (!entry || entry->online == 0) return 1;
	if (track_cfg->config_status != CFG_STATUE_NOCONFIG){
		printf("in config_location_order, track_cfg->config_status =%d\n",
			track_cfg->config_status);
		return 1;
	}

	if (!config_location_check_sysmode()) {
		char error_str[256] = {0};
		snprintf(error_str, 256,
			LOCALTION_ERROR_STR,
			"Set device work mode Error!");
		write_to_blue_tooth(get_bluetooth_fd(), error_str);
		return 0;
	}
	if (!config_location_check_cell_info()){
		char error_str[256] = {0};
		snprintf(error_str, 256,
			LOCALTION_ERROR_STR,
			"Set device cell config Error!");
		write_to_blue_tooth(get_bluetooth_fd(), error_str);
	}
	if (!config_location_set_ue_config()) {
		char error_str[256] = {0};
		snprintf(error_str, 256,
			LOCALTION_ERROR_STR,
			"Set device UE config Error!");
		write_to_blue_tooth(get_bluetooth_fd(), error_str);
	}
	track_cfg->config_status = CFG_STATUE_NOCONFIG;
	return 1;
}

void *send_ue_information(void *arg)
{
	if (!arg) {
		char error_str[256] = {0};
		snprintf(error_str, 256,
			LOCALTION_ERROR_STR,
			"Please input IMSI!");
		write_to_blue_tooth(get_bluetooth_fd(), error_str);
		return NULL;
	}
	char *imsi = (char *)arg;
	memcpy(track_cfg->imsi, imsi, IMSI_LENGTH);
	if (!strncmp(imsi, "46000", PLMN_LENGTH) ||
		!strncmp(imsi, "46002", PLMN_LENGTH) ||
		!strncmp(imsi, "46007", PLMN_LENGTH)) {
		memcpy(track_cfg->plmn, "46000", PLMN_LENGTH);
		track_cfg->work_mode = TDD;
		track_cfg->ue_maxpower_switch = 0;
	} else if (!strncmp(imsi, "46001", PLMN_LENGTH) ||
		!strncmp(imsi, "46006", PLMN_LENGTH)) {
		memcpy(track_cfg->plmn, "46001", PLMN_LENGTH);
		track_cfg->work_mode = FDD;
		track_cfg->ue_maxpower_switch = 1;
	} else if (!strncmp(imsi, "46011", PLMN_LENGTH) ||
		!strncmp(imsi, "46003", PLMN_LENGTH)) {
		memcpy(track_cfg->plmn, "46011", PLMN_LENGTH);
		track_cfg->work_mode = FDD;
		track_cfg->ue_maxpower_switch = 1;
	} else {
		char error_str[256] = {0};
		snprintf(error_str, 256,
			LOCALTION_ERROR_STR,
			"Not support this IMSI!");
		write_to_blue_tooth(get_bluetooth_fd(), error_str);
		return NULL;
	}
	track_cfg->sysdarfcn = 0;
	track_cfg->ue_mode = 2;
	track_cfg->config_status = 0;
	track_cfg->meas_report_period = 4;
	/* 处理自动配置定位 */
	config_location_order();
	return NULL;
}
void *thread_pare_bluetooth_cmd_buf(void *arg)
{
	if (!arg || !strstr((const char*)arg, CMD_START)) return NULL;
	printf("pare bt command:[%s]\n", (char *)arg);
	char cmd[1024] = {0};
	sscanf(strstr((const char*)arg, CMD_START), CMD_START"%[^#]", cmd);
	free(arg);
	printf("read cmd is[%s]\n", cmd);
	cJSON *object = cJSON_Parse(cmd); 
	if (!object) {
		return NULL;
	}
	cJSON *ctype = cJSON_GetObjectItem(object, "type");
	if (!ctype) {
		goto pare_finish;
	}
	char *reponse_str = NULL;
	unsigned int type;
	sscanf(ctype->valuestring, "%02x", &type);
	switch(type) {
	case STOP_SEND_UE_INFOR_REQ: /* 停止上传UE测量信息 */
		set_ue_send_flag(0);
		break;
	case DEVICE_HEART_BEAT_REQ: /* 心跳检测 */
		{
			printf("BT>-----心跳检测!\n");
			cJSON *time_set = cJSON_GetObjectItem(object, "time");
			if (time_set) {
				char set_time_cmd[32] = {0};
				snprintf(set_time_cmd, 32, "date -s \"%s\" &",
					time_set->valuestring);
				system (set_time_cmd);
			}
			write_to_blue_tooth(get_bluetooth_fd(),
				DEVICE_HEARTBEATRES);
			break;
		}
	case DEVICE_SCAN_ARFCN_REQ: /* 扫频请求 */
		{
			printf("BT>----扫频请求!\n");
			cJSON *cmode = cJSON_GetObjectItem(object, "sweep");
			if (!cmode) break;
      FILE *file = NULL;
      int fd = open_model_devid(&file); 
      while(1){
        char *scan_str = NULL;
        /*服务器端  电信扫描*/
        if(args_info.tcp_state_arg)
        {
          /*46011 38 3电信LTE*/
			    scan_str = get_scan_cell_information(CHINA_NET_LTE_PLMN, LTE_ONLY, LTE_MODE, fd, file);
			    if (scan_str) {
            pthread_mutex_lock(&blue_lock);
				    write_to_blue_tooth(get_bluetooth_fd(), scan_str);
            pthread_mutex_unlock(&blue_lock);
            scan_str = NULL;
			    }
          /* 电信2G CDMA */
          scan_str = get_cdma_cell_info(fd, file);
          if(scan_str) {
            pthread_mutex_lock(&blue_lock);
	          write_to_blue_tooth(get_bluetooth_fd(), scan_str);
            pthread_mutex_unlock(&blue_lock);
            scan_str = NULL;
          }
        }else
        {
          if(args_info.operator_arg == 1)/*客户端 移动扫描*/
          {
            /*移动LTE*/
			      scan_str = get_scan_cell_information(CHINA_MOBILE_LTE_PLMN, LTE_ONLY, LTE_MODE, fd, file);
			      if (scan_str) {
				      if(send_to_msg(scan_str, sockfd) < 0) break;
              scan_str = NULL;
			      }
            /*移动GSM*/
			      scan_str = get_scan_cell_information(CHINA_MOBILE_LTE_PLMN, GSM_ONLY, GSM_MODE, fd, file);
			      if (scan_str) {
				      if(send_to_msg(scan_str, sockfd) < 0) break;
              scan_str = NULL;
			      }
            /*移动 TD_SCDMA*/
		        scan_str = get_scan_cell_information(CHINA_MOBILE_LTE_PLMN, TD_SCDMA_ONLY, GSM_MODE, fd, file);
            if(scan_str) {
				      if(send_to_msg(scan_str, sockfd) < 0) break;
              scan_str = NULL;
            }
          }else if(args_info.operator_arg == 2)/*联通扫描*/
          {
            /*联通LTE*/
			      scan_str = get_scan_cell_information(CHINA_UNICOM_LTE_PLMN, LTE_ONLY, LTE_MODE, fd, file);
			      if (scan_str) {
				      if(send_to_msg(scan_str, sockfd) < 0) break;
              scan_str = NULL;
			      }
            /*联通GSM*/ 
			      scan_str = get_scan_cell_information(CHINA_UNICOM_LTE_PLMN, GSM_ONLY, GSM_MODE, fd, file);
			      if (scan_str) {
				      if(send_to_msg(scan_str, sockfd) < 0) break;
              scan_str = NULL;
			      }
            /*联通 WCDMA*/
		        scan_str = get_scan_cell_information(CHINA_MOBILE_LTE_PLMN, WCDMA_ONLY, GSM_MODE, fd, file);
            if(scan_str) {
				      if(send_to_msg(scan_str, sockfd) < 0) break;
              scan_str = NULL;
            }
          }
        }
      }
      close(fd);
			break;
		}
	case DEVICE_AUTO_LOCATION_REQ: /* 自动定位请求 */
		{
			if (!entry|| !(entry->is_have & BASE_BAND_GENERIC_FULL) ) {
				char error_str[256] = {0};
				snprintf(error_str, 256,
					LOCALTION_ERROR_STR,
					"location device is not ready!");
				write_to_blue_tooth(get_bluetooth_fd(), error_str);
			}
			printf("BT>----自动定位请求!\n");
			cJSON *cimsi = cJSON_GetObjectItem(object, "imsi");
			if (!cimsi) {
				char error_str[256] = {0};
				snprintf(error_str, 256,
					LOCALTION_ERROR_STR,
					"please input IMSI!");
				write_to_blue_tooth(get_bluetooth_fd(), error_str);
				break;
			}
			char imsi[C_MAX_IMSI_LEN] = {0};
			snprintf(imsi, C_MAX_IMSI_LEN, "%s", cimsi->valuestring);
			safe_pthread_create(send_ue_information, (void *)imsi);
			break;
		}
	case DEVICE_SET_ARFCN_REQ: /* 设置频点配置信息请求 */
		{
			printf("BT>----设置频点请求\n");
			cJSON *darfcn = cJSON_GetObjectItem(object, "arfcn");
			cJSON *cellpci = cJSON_GetObjectItem(object, "pci");
			if (!darfcn) break;
			if (darfcn->type == cJSON_String) {
				track_cfg->sysdarfcn = atoi(darfcn->valuestring);
			} else if (darfcn->type == cJSON_Number) {
				track_cfg->sysdarfcn = darfcn->valueint;
			} else {
				break;
			}
			if (cellpci) {
				if (cellpci->type == cJSON_String) {
					track_cfg->cell_pci
						= atoi(cellpci->valuestring);
				} else if (cellpci->type == cJSON_String) {
					track_cfg->cell_pci
						= cellpci->valueint;
				}
			}
			/* 设置频点信息 */
			config_location_check_cell_info();
			break;
		}
	case IMSI_PROBE_INFO_SEND_REQ: /* 是否开启IMSI信息上传 */
		{
			cJSON *cflag = cJSON_GetObjectItem(object, "flag");
			if (cflag) {
				imsi_send_flag = atoi(cflag->valuestring);
				if (imsi_send_flag) {
					printf("BT>----IMSI上传请求:开始上传\n");
				} else {
					printf("BT>----IMSI上传请求:停止上传\n");
				}
			}
		}
	default:
		break;
	} /* end switch */
pare_finish:
	cJSON_Delete(object);
	return NULL;
}
void *thread_read_bluetooth_cmd(void *arg)
{
	init_bluetooth_device_fd();  /* 初始化蓝牙连接串口设备 */
	if (track_cfg) {
		free(track_cfg);
	}
  if(pthread_mutex_init(&blue_lock, NULL))
  {
    perror("init mutex");
    exit(-1);
  }
	track_cfg = (track_config_t *)malloc(sizeof(track_config_t));
	if (track_cfg == NULL) {
		perror("malloc track_cfg error!");
		exit(0);
	}
	memset(track_cfg, 0, sizeof(track_config_t));

	int fd = get_bluetooth_fd();
	int read_len = 0;
	char read_tmp[SERIAL_READ_MAX] = {0};
	char *tmp = NULL;
	while (1) {
		fflush(stdin);
		getchar();
		char *buffer = (char *)calloc(1, BUFSIZE);
		if (!buffer) continue;
		int buf_offset = 0;
		if (tmp) {
			buf_offset = sprintf(buffer, "%s", tmp);
		}
		while (1) {
			memset(read_tmp, 0, read_len);
			read_len = read(fd, read_tmp, SERIAL_READ_MAX);
			if (read_len <= 0 ||
				(read_len == 1 && read_tmp[0] == '\n')) {
				continue;
			}
			printf("RES(%d):%s\n",read_len, read_tmp);
			memcpy(&buffer[buf_offset], read_tmp, read_len);
			buf_offset += read_len;

			if (strstr(buffer, CMD_STOP)) {
				if (strstr(read_tmp, CMD_STOP) + 3 - 1 !=
					read_tmp + SERIAL_READ_MAX - 1) 
					tmp = strstr(read_tmp, CMD_STOP) + 3;
				else 
					tmp = NULL;
				break;
			}
		} /* end while (1) */
		if (buf_offset > 0) {
			printf("read---------%s\n", buffer);
			safe_pthread_create(thread_pare_bluetooth_cmd_buf,
				(void *)buffer);
		}
	} /* end while (1) */
	exit(1);
}
/* ============================================== */

char *get_scan_cell_information(char *mnc, int mode, int act, int fd, FILE *file)
{
  if(act != LTE_MODE && act != GSM_MODE)
    return NULL;
  if(mode != GSM_ONLY && mode != LTE_ONLY && mode != TD_SCDMA_ONLY && mode != WCDMA_ONLY)
    return NULL;
  if(strcmp(CHINA_MOBILE_LTE_PLMN, mnc) && 
     strcmp(CHINA_UNICOM_LTE_PLMN, mnc) && 
     strcmp(CHINA_NET_LTE_PLMN, mnc))
    return NULL;
	//char plmn[6] = {0};
	//snprintf(plmn, 6, "460%02d", mnc);
	int lock = 0;
	char act_cmd[64] = {0};
	int len = 0, ret = 0;
	/* get model  */
	char mod_buf[BUFSIZE] = {0};
	len = snprintf(act_cmd, 64, "AT^MODECONFIG?\r\n");
	printf("<<<----CMD:%s\n", act_cmd);
	ret = write(fd, act_cmd, len);
	if (ret != len) {
		printf("get modconfig error\n");
		goto go_with_set_mod;
	}
	sleep(2);
	ret = readline(file, mod_buf, BUFSIZE);
	fflush(file);
	printf("len:%d mod_buf:%s\n", ret, mod_buf);
	if (ret  && strstr(mod_buf, "MODECONFIG")) {
		int _mod = 0;
		sscanf(mod_buf, "%*s%d", &_mod);
		printf("check modeconfig :%d\n", _mod);
		if (_mod == mode) {
			goto go_without_set_mod;
		}
	}
go_with_set_mod:
	/* check LTE model */
	len = snprintf(act_cmd, 64, "AT^MODECONFIG=%02d\r\n", mode);
	printf("<<<----CMD:%s\n", act_cmd);
	write(fd, act_cmd, len);
	usleep(500000);
  if(mode == WCDMA_ONLY || mode == TD_SCDMA_ONLY){
    goto go_get_information;
  }
go_without_set_mod:
	/* lock plmn */
	len = snprintf(act_cmd, 64, "AT+LOCKPLMN=%01d,\"%s\",%01d\r\n",
		LOCK_PLMN, mnc, act);
	printf("<<<----CMD:%s\n", act_cmd);
	write(fd, act_cmd, len);
	lock = 1;
	sleep(15);
go_get_information:
	/* get cell information */
	len = snprintf(act_cmd, 64, "AT+SCELLINFO\r\n");
	printf("<<<----CMD:%s\n", act_cmd);
	write(fd, act_cmd, len);
	sleep(1);
	/* get all neighbor cell info */
	len = snprintf(act_cmd, 64, "AT+ANCELLINFO\r\n");
	printf("<<<----CMD:%s\n", act_cmd);
	write(fd, act_cmd, len);
	sleep(2);
	if(lock){
		len = snprintf(act_cmd, 64, "AT+LOCKPLMN=%01d,\"%s\",%01d\r\n",
			UNLOCK_PLMN, mnc, act);
		printf("<<<----CMD:%s\n", act_cmd);
		write(fd, act_cmd, len);
		sleep(1);
	}
	char *str = read_model_response(file);
	return str;
}

/*打开模块*/
int open_model_devid(FILE **file)
{
	int model = init_scan_model_device_fd();
FDOPEN:
	*file = fdopen(model, "w+");
	if (*file == NULL) {
		perror("fdopen error!");
		goto FDOPEN;
	}
  return model;
}

#if 1
char *get_cdma_cell_info(int fd, FILE *file)
{
	char act_cmd[64] = {0};
	int len = 0, ret = 0;
	/* get model  */
	char mod_buf[BUFSIZE] = {0};
	len = snprintf(act_cmd, 64, "AT^MODECONFIG?\r\n");
	printf("<<<----CMD:%s\n", act_cmd);
	ret = write(fd, act_cmd, len);
	if (ret != len) {
		printf("get modconfig error\n");
		goto go_with_set_mod;
	}
	ret = readline(file, mod_buf, BUFSIZE);
	fflush(file);
	if (ret  && strstr(mod_buf, "MODECONFIG")) {
		int _mod = 0;
		sscanf(mod_buf, "%*s%d", &_mod);
		printf("check modeconfig :%d\n", _mod);
		if (_mod == CDMAX_EVDO) {
			goto go_without_set_mod;
		}
	}
go_with_set_mod:
	/* check CDMA_EVDO model */
	len = snprintf(act_cmd, 64, "AT^MODECONFIG=%02d\r\n", CDMAX_EVDO);
	printf("<<<----CMD:%s\n", act_cmd);
	write(fd, act_cmd, len);
	usleep(500000);
go_without_set_mod:
	len = snprintf(act_cmd, 64, "AT+BSINFO\r\n");
	printf("<<<----CMD:%s\n", act_cmd);
	write(fd, act_cmd, len);
  sleep(1);
	/* get all neighbor cell info */
	char *str = read_model_response(file);
	return str;
}
#endif
static int readline(FILE *file, char *buf, int bufsize)
{
	int nread;
	memset(buf, 0, bufsize);
	nread = fscanf(file, "%[^\r]", buf);
	if(nread  == EOF) {
		fprintf(stderr, "fscanf failed\n");
		return -1;
	}
	/* skip \r\n */
	char skip[16];
	fscanf(file, "%[\r\n]", &skip[0]);
	return 0;
}
char *read_model_response(FILE *file)
{
	cJSON *object = cJSON_CreateObject();
	if (!object) return NULL;
	cJSON *array = cJSON_CreateArray();
	if (!array) {
		cJSON_Delete(object);
		return NULL;
	}
	cJSON_AddItemToObject(object, "ncells", array);
	cJSON_AddStringToObject(object, "type", "02");
	printf("read model response...\n");
	char buf[BUFSIZE];
	int ret = 0;
	int sign = 0;
	while (1) {
		ret = readline(file, buf, BUFSIZE);
		if (ret == -1) {
			break;
		}
		if (strstr(buf, "AT+") || strstr(buf, "OK")) {
			printf(">>>>-------REC:%s\n", buf);

		} else {
			printf("ststus:%s\n", buf);
			if (strstr(buf, "+SCELLINFO:")) {
				cell_info_t cell_info;
				sscanf(buf,
					"%*s%d,%d,%d,\"%04s\",\"%08s\",%d,%d,%d",
					&cell_info.cell_mod,
					&cell_info.mmc,
					&cell_info.mnc,
					cell_info.tac,
					cell_info.cellid,
					&cell_info.arfcn,
					&cell_info.pci,
					&cell_info.rxlevel);
				if(cell_info.cell_mod < 0 || cell_info.cell_mod > 4)
          continue;
        if(cell_info.arfcn == 0 && cell_info.pci == 0 && cell_info.rxlevel == 0)
          continue;
				cJSON_AddNumberToObject(object,"cell_mod", 
          cell_info.cell_mod);
				cJSON_AddNumberToObject(object, "mmc",
					cell_info.mmc);
				cJSON_AddNumberToObject(object, "mnc",
					cell_info.mnc);
				cJSON_AddStringToObject(object, "ac",
					cell_info.tac);
				cJSON_AddStringToObject(object,
					"cellid", cell_info.cellid);
				cJSON_AddNumberToObject(object, "arfcn",
					cell_info.arfcn);
				cJSON_AddNumberToObject(object, "pci",
					cell_info.pci);
				cJSON_AddNumberToObject(object,
					"rxlevel", cell_info.rxlevel);
				if (cell_info.arfcn <= 599) {
					cJSON_AddNumberToObject(object,
						"sysband", 1);
				} else if (1200 <= cell_info.arfcn &&
					cell_info.arfcn <= 1949) {
					cJSON_AddNumberToObject(object,
						"sysband", 3);
				} else if (2400 <= cell_info.arfcn &&
					cell_info.arfcn <= 2649) {
					cJSON_AddNumberToObject(object,
						"sysband", 5);
				} else if (2750 <= cell_info.arfcn &&
					cell_info.arfcn <= 3449) {
					cJSON_AddNumberToObject(object,
						"sysband", 5);
				} else if (3450 <= cell_info.arfcn &&
					cell_info.arfcn <= 3799) {
					cJSON_AddNumberToObject(object,
						"sysband", 8);
				} else if (37750 <= cell_info.arfcn &&
					cell_info.arfcn <= 38249) {
					cJSON_AddNumberToObject(object,
						"sysband", 38);
				} else if (38250 <= cell_info.arfcn &&
					cell_info.arfcn <= 38649) {
					cJSON_AddNumberToObject(object,
						"sysband", 39);
				} else if (38650 <= cell_info.arfcn &&
					cell_info.arfcn <= 39649) {
					cJSON_AddNumberToObject(object,
						"sysband", 40);
				} else if (39650 <= cell_info.arfcn &&
					cell_info.arfcn <= 41589) {
					cJSON_AddNumberToObject(object,
						"sysband", 41);
				} else if (41590 <= cell_info.arfcn &&
					cell_info.arfcn <= 43589) {
					cJSON_AddNumberToObject(object,
						"sysband", 42);
				}
				++sign;
			} else if (strstr(buf, "+ANCELLINFO:")){
				cell_info_t cell_info;
				int tmp = 0;
				sscanf(buf,
					"%*s%d,%d,%d,%d,%d,\"%4s\",\"%8s\",%d,%d,%d",
					&tmp, &tmp, &cell_info.cell_mod,
					&cell_info.mmc, &cell_info.mnc,
					cell_info.tac, cell_info.cellid,
					&cell_info.arfcn, &cell_info.pci,
					&cell_info.rxlevel);
				cJSON *obj = cJSON_CreateObject();
				if (obj) {
				  if(cell_info.cell_mod < 0 || cell_info.cell_mod > 4)
            continue;
          if(cell_info.arfcn == 0 && cell_info.pci == 0 && cell_info.rxlevel == 0)
            continue;
					cJSON_AddNumberToObject(obj,"cell_mod",
							cell_info.cell_mod);
					cJSON_AddNumberToObject(obj,"mmc",
						cell_info.mmc);
					cJSON_AddNumberToObject(obj,"mnc",
						cell_info.mnc);
					cJSON_AddStringToObject(obj,"ac",
						cell_info.tac);
					cJSON_AddStringToObject(obj,"cellid",
						cell_info.cellid);
					cJSON_AddNumberToObject(obj,"arfcn",
						cell_info.arfcn);
					cJSON_AddNumberToObject(obj,"pci",
						cell_info.pci);
					cJSON_AddNumberToObject(obj,"rxlevel",
						cell_info.rxlevel);
					if (cell_info.arfcn <= 599) {
						cJSON_AddNumberToObject(obj,
							"sysband", 1);
					} else if (1200 <= cell_info.arfcn &&
						cell_info.arfcn <= 1949) {
						cJSON_AddNumberToObject(obj,
							"sysband", 3);
					} else if (2400 <= cell_info.arfcn &&
						cell_info.arfcn <= 2649) {
						cJSON_AddNumberToObject(obj,
							"sysband", 5);
					} else if (2750 <= cell_info.arfcn &&
						cell_info.arfcn <= 3449) {
						cJSON_AddNumberToObject(obj,
							"sysband", 5);
					} else if (3450 <=cell_info.arfcn &&
						cell_info.arfcn <= 3799) {
						cJSON_AddNumberToObject(obj,
							"sysband", 8);
					} else if (37750 <= cell_info.arfcn &&
						cell_info.arfcn <= 38249) {
						cJSON_AddNumberToObject(obj,
							"sysband", 38);
					} else if (38250 <= cell_info.arfcn &&
						cell_info.arfcn <= 38649) {
						cJSON_AddNumberToObject(obj,
							"sysband", 39);
					} else if (38650 <= cell_info.arfcn &&
						cell_info.arfcn <= 39649) {
						cJSON_AddNumberToObject(obj,
							"sysband", 40);
					} else if (39650 <= cell_info.arfcn &&
						cell_info.arfcn <= 41589) {
						cJSON_AddNumberToObject(obj,
							"sysband", 41);
					} else if (41590 <= cell_info.arfcn &&
						cell_info.arfcn <= 43589) {
						cJSON_AddNumberToObject(obj,
							"sysband", 42);
					}
				}
				cJSON_AddItemToArray(array, obj);
				++sign;
			}else if(strstr(buf, "+BSINFO:")){
        cdma_info_t  cell_info;
        memset(&cell_info, 0, sizeof(cdma_info_t));
        sscanf(buf, 
            "%d,%d,%d,%d,%d,%d,%d,%d", 
            &cell_info.sid, &cell_info.nid, 
            &cell_info.bid, &cell_info.channel, 
            &cell_info.pn, &cell_info.rssi, 
            &cell_info.dev_long, &cell_info.dev_lat);
        if(cell_info.sid == 0 && cell_info.nid == 0 && cell_info.bid == 0)
          continue;
	      cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "sid", cell_info.sid);
        cJSON_AddNumberToObject(obj, "nid", cell_info.nid);
        cJSON_AddNumberToObject(obj, "bid", cell_info.bid);
        cJSON_AddNumberToObject(obj, "channel", cell_info.channel);
        cJSON_AddNumberToObject(obj, "pn", cell_info.pn);
        cJSON_AddNumberToObject(obj, "rssi", cell_info.rssi);
        cJSON_AddNumberToObject(obj, "dev_long", cell_info.dev_long);
        cJSON_AddNumberToObject(obj, "dev_lat", cell_info.dev_lat);
        cJSON_AddItemToArray(array, obj);
	      ++sign;
      }
		}
	} /* end while */
	if(!sign)
		return NULL;
	char *json_string = cJSON_PrintUnformatted(object);
	cJSON_Delete(object);
	if (json_string) {
		printf("json_string:%s\n", json_string);
		return json_string;
	}
	return NULL;
}

