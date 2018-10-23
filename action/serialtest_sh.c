#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/io.h>
#include <termios.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>

#include "arg.h"
#include "config.h"
#include "serialtest_sh.h"
#include "cJSON.h"
#include "http.h"
#include "cmdline.h"

#define MAX_SIZE   256
#define COM1       (0)
#define COM2       (1)
#define COM3       (2)
#define COM4       (3)
#define START_VAR  (0x7E)
#define STOP_VAR   (0x7F)

//应答编码定义
#define ACK_OK 	      (0x00)
#define ACK_CMD_ERR   (0x02)
#define ACK_DATA_ERR  (0x03)
#define ACK_OP_FAIL   (0x04)

//模块功能编码
enum MT_CODE
{
	MT_DET 		= 0x01,
	MT_ATT 		= 0x02,
	MT_AP  		= 0x08,  //功放模块地址
	MT_PSK 		= 0x0a,
	MT_IN_POWER 	= 0x0e,
	MT_MAX	 	= 0x0f
}MO_CODE_T;

//命令编号
enum
{
	SET_MO_ADD 	      = 0x10, //设置模块地址
	QUERY_MO_STATUS       = 0x11, //查询模块状态
	QUERY_MO_VERSION      = 0x12, //查询模块软件版本号
	SET_AP_ON_OFF 	      = 0x22, //设置功放开关
	SET_ALC 	      = 0x23, //设置ALC
	SET_MO_MAX_GAIN_ALIGN = 0x44,  //模块最大增益校准命令
	SET_MO_MAX_GAIN       = 0x45, //模块最大增益设置
	GET_MO_GAIN_ALING     = 0x46, //读取模块增益校准参数
	RESET_MO_AP 	      = 0x88, //模块抚慰
};

/* 功放模块状态信息 */
#define PA_STATUS_POWER_UP   (1)  /* 功放开关状态 0-关, 1-开 */
#define PA_WARNING_TXPOWER   (1 << 1)  /* 功放过功率告警 0-正常, 1-故障 */
#define PA_WARNING_TEMP      (1 << 2)  /* 功放过温度告警 0-正常, 1-故障 */
#define PA_WARNING_VSWR      (1 << 3)  /* 功放驻波比告警 0-正常, 1-故障 */
#define PA_WARNING_PA_DEV    (1 << 4)  /* 功放有故障告警 0-正常, 1-故障 */

#pragma pack(1)
typedef struct {
	U8  MO_TYP;  //模块类型
	U8  MO_ADD;  //模块地址
	U8  MO_CMD;  //命令类型
	U8  MO_ACK;
	U8  MO_LEN;
	U8  MO_DTU[MAX_SIZE];
} NP_PACK;

struct MPK {
	struct {
		U8 mt;
		U8 cmd;
		U8 tlen;
		U8 rlen;
	} HDL;
	unsigned char DTU[MAX_SIZE];
};//MPK;

typedef struct DataFmt {
	U8 mt;
	U8 cmd;
	U8 tlen;
	U8 rlen;
	U8 DTU[1];
} DataFmt_t;

typedef struct {
	U8 mt;//模块类型
	U8 ma;//模块地址
} TModules;

typedef struct PA_STATUS {
	U8 paStatus; //PA状态
	S8 backPower;//反向功率
	S8 paTemp;//功放温度
	S8 paAlc; //功放ALC的数值
	U8 swr;   //驻波比
	U8 paAtt; //功放ATT，无符号
	S8 fowardPower; //前向功率，有符号
} PA_STATUS_t;
#pragma pack()

static TModules  Modules[] =
{
  {0x08,0x00}, //功放模块
  {0x09,0x00}, //低噪模块
  {0x06,0x00}, //光模块
  {0xAA,0x00}, //配置模块
  {0x00,0x00}  //结束
};

static DataFmt_t  DataSend[]=
{
/*{ mt,   cmd, tlen, rlen,  DTU */
  {0x08, 0x11, 0x00, 0x07, {0x00}}, //查询功放状态
  {0x08, 0x22, 0x01, 0x00, {0x01}}, //设置功放开关---开
  {0x08, 0x22, 0x01, 0x00, {0x00}}, //设置功放开关---关
};

static unsigned short int fcstab[256]=
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};


/* ===================================================================== */
//int serial_fd;  //serial handle
static U8 isPaEnabled =  0;
static int serial_fd = -1;
static int return_serial_fd()
{
	return serial_fd;
}

static unsigned short crc16(unsigned char *d,unsigned short len,unsigned short initcrc)
{
	unsigned short i;
	unsigned short crc;
	crc=initcrc;
	for(i = 0; i < len; i++) {
		crc=(crc<<8)^fcstab[(crc>>8)^*d++];
	}
	return crc;
}

static int openPort(int id)
{
	char *SerialCom[] = {"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3"};

	int fd = open(SerialCom[id], O_NDELAY|O_RDWR|O_NOCTTY|O_NONBLOCK, 0777);

	if(fd == -1) {
		printf("%s open failed, please check\n",SerialCom[id]);
	}
	return fd;
}
/* 设置serial 参数 */
static int set_opt(int serial_fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	/*保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息*/
	if(tcgetattr( serial_fd,&oldtio)  !=  0) {
		perror("SetupSerial 1");
		printf("tcgetattr( serial_fd,&oldtio) -> %d\n",
			tcgetattr( serial_fd,&oldtio));
		return -1;
	}
	bzero( &newtio, sizeof( newtio ));

	/*步骤一，设置字符大小*/
	newtio.c_cflag  |=  CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;
	/*设置停止位*/
	switch(nBits)
	{
	case 6:
		newtio.c_cflag |= CS6;
		break;
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	default:
		break;
	}
	/*设置奇偶校验位*/
	switch(nEvent)
	{
	case 'o':
	case 'O': //奇数
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'e':
	case 'E': //偶数
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'n':
	case 'N':  //无奇偶校验位
		newtio.c_cflag &= ~PARENB;
		break;
	default:
		break;
	}
	/*设置波特率*/
	switch(nSpeed)
	{
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 19200:
		cfsetispeed(&newtio,B19200);
		cfsetospeed(&newtio,B19200);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	case 460800:
		cfsetispeed(&newtio, B460800);
		cfsetospeed(&newtio, B460800);
		break;
	default:
		cfsetispeed(&newtio, B19200);
		cfsetospeed(&newtio, B19200);
		break;
	}
	/*设置停止位*/
	if(nStop == 1)
		newtio.c_cflag &=  ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |=  CSTOPB;

	/*设置等待时间和最小接收字符*/
	newtio.c_cc[VTIME]  = 0;
	newtio.c_cc[VMIN] = 0;

	/*处理未接收字符*/
	tcflush(serial_fd,TCIFLUSH);
	/*激活新配置*/
	if((tcsetattr(serial_fd, TCSANOW, &newtio)) != 0)
	{
		perror("com set error");
		return -1;
	}
	printf("set done!\n");
	return 0;
}

/* 获取ma信息 */
static U8 GetModuleAddress(U8 mt)
{
	int i;
	for(i = 0; i < sizeof(Modules) / sizeof(TModules); i++) {
		if(mt == Modules[i].mt) {
			return Modules[i].ma;
		}
	}
	return 0;
}
/* 解析请求回复 */
static int ParseInfo(U8 ma, U8 cmd, U8 *ptr)
{
	switch(cmd) {
	case QUERY_MO_STATUS:
		{
			PA_STATUS_t *p = (PA_STATUS_t *)ptr;
			printf("pa status（1-ON，0-OFF）%d\n",
				p->paStatus & PA_STATUS_POWER_UP);
			if(p->paStatus & PA_STATUS_POWER_UP) {
				//open
				printf("pa[%d]ON\n",__LINE__);
				isPaEnabled = 1;
			} else {
				//close
				printf("pa[%d]OFF\n",__LINE__);
				isPaEnabled = 0;
			}
			printf("pa power too high warning?%s\n",
				(p->paStatus & PA_WARNING_TXPOWER)?"Yes":"No");
			printf("pa temperature too high warning?%s\n",
				(p->paStatus & PA_WARNING_TEMP)?"Yes":"No");
			printf("pa standing-wave ratio warning?%s\n",
				(p->paStatus & PA_WARNING_VSWR)?"Yes":"No");
			printf("pa malfunction warning?%s\n",
				(p->paStatus & PA_WARNING_PA_DEV)?"Yes":"No");

			printf("back power： %d\n", p->backPower);
			printf("pa temperature： %d\n", p->paTemp);
			printf("pa ALC： %d\n", p->paAlc);
			printf("standing-wave ratio： %d\n", p->swr);
			printf("pa ATT： %d\n", p->paAtt);
			printf("forward power： %d\n", p->fowardPower);
			cJSON *object = cJSON_CreateObject();
			if (!object) break;
			cJSON_AddStringToObject(object, "topic", my_topic);
			cJSON_AddNumberToObject(object, "pa_num", (int)ma);
			cJSON_AddNumberToObject(object,
				"switch_status", isPaEnabled);
			cJSON_AddNumberToObject(object,
				"backpower", p->backPower);
			cJSON_AddNumberToObject(object, "patemp", p->paTemp);
			cJSON_AddNumberToObject(object, "paalc", p->paAlc);
			cJSON_AddNumberToObject(object, "swr", p->swr);
			cJSON_AddNumberToObject(object, "paatt", p->paAtt);
			cJSON_AddNumberToObject(object,
				"fowardpower", p->fowardPower);
			cJSON_AddNumberToObject(object,"w_txpower",
				(p->paStatus & PA_WARNING_TXPOWER)?1:0);
			cJSON_AddNumberToObject(object,"w_temp",
				(p->paStatus & PA_WARNING_TEMP)?1:0);
			cJSON_AddNumberToObject(object,"w_vswr",
				(p->paStatus & PA_WARNING_VSWR)?1:0);
			cJSON_AddNumberToObject(object,"w_padev",
				(p->paStatus & PA_WARNING_PA_DEV)?1:0);

			char *send_str = cJSON_PrintUnformatted(object);
			cJSON_Delete(object);
			if (!send_str) {
				printf("pa info json to string error!\n");
				break;
			}
			printf("pa status query str:%s\n", send_str);
			pthread_mutex_lock(&http_send_lock);
			http_send(args_info.painfo_api_arg,
				send_str, NULL, NULL);
			pthread_mutex_unlock(&http_send_lock);
			free(send_str);
			break;
		}
	case SET_AP_ON_OFF:
		{
			//!todo:nyb,2018.04.10, add implementation to operate PA ON/OFF here
			printf("set pa ON/OFF success！,[%d]\n",__LINE__);
			break;
		}
	default:
		break;
	}

	return 0;
}
/* 根据命令进行pk赋值 */
static void fillPkInfo( struct MPK *pk,int cmd_type)
{
	pk->HDL.mt = DataSend[cmd_type].mt;
	pk->HDL.cmd = DataSend[cmd_type].cmd;
	pk->HDL.tlen = DataSend[cmd_type].tlen;
	pk->HDL.rlen = DataSend[cmd_type].rlen;

	//fill payload info
	if(pk->HDL.tlen != 0) {
		memcpy(pk->DTU, DataSend[cmd_type].DTU, pk->HDL.tlen);
		printf("pk->DTU 0x%x\n",pk->DTU[0]);
	}
	return;
}

/* 发送命令并接收回复 */
static int sendAndParse(U8 ma, struct MPK *pk)
{
	int recvBytes = 0, sendBytes, n, i;
	U8 *p = NULL;
	U8 recvBuf[MAX_SIZE] = {0};
	U8 sendBuf[MAX_SIZE] = {0};
	U8 mt,  cmd, rl, tl;
	U16 crc;
	int serial_fd = return_serial_fd();

	memset(sendBuf,0,sizeof(sendBuf));
	p = sendBuf;
	mt = pk->HDL.mt;  //module type
	//ma = GetModuleAddress(mt);  //module address
	cmd =pk->HDL.cmd;
	tl = pk->HDL.tlen; //transmit length
	rl = pk->HDL.rlen; //read length

//	printf("#---mt: 0x%x\n",mt);
//	printf("#---ma: 0x%x\n",ma);
//	printf("#---cmd: 0x%x\n",cmd);
//	printf("#---rlen: 0x%x\n",rl);
//	printf("#---tlen: 0x%x\n",tl);

	*p++ = START_VAR; /* 0x7E */
	*p++ = mt;  /* 模块地址 H*/
	*p++ = ma;  /* 模块地址 L*/
	*p++ = cmd; /* 命令编号 */
	*p++ = 0x00;/* ACK 指令发起方,该字段添0 */
	*p++ = tl;  /* 命令长度 单位:字节 */

	printf("transmit length %d,[%d]\n",tl,__LINE__);
	for(i = 0; i < tl; i++) {
		*p++ = pk->DTU[i];
	}
	crc = crc16(&sendBuf[1], tl + 5, 0);

	*p++ = (crc >> 0) & 0xff;
	*p++ = (crc >> 8) & 0xff;
	*p++ = STOP_VAR;
	printf("SH pa request:\n");
	for(i = 0; i < 9 + tl; i++) {
		printf("%02x ", sendBuf[i]);
	}
	printf("\n");
#if 1
	//send info to serial port
	sendBytes = write(serial_fd, sendBuf, 9 + tl);
	printf("write success!\n");

	memset(recvBuf, 0, sizeof(recvBuf));
	sleep(2);

	//this will avoid to we fetch invalid data
	recvBytes = read(serial_fd, recvBuf, MAX_SIZE);
	if (recvBytes == -1 && errno == EWOULDBLOCK) {
		printf("nothing to read, retuern!\n");
	}
	printf("recvBytes: %d\n",recvBytes);

	printf("pa info response:");
	for(int i = 0; i < recvBytes; i++) {
		printf("%02x ", recvBuf[i]);
	}
	printf("\n");
	if(recvBytes >= 8) {
		p = recvBuf;
		if(*p != START_VAR) {
			printf("[0x%x]start Var: 0x%x,[%d]",
				START_VAR, *p, __LINE__);
			goto exit;
		}
		if(p[recvBytes-1] != STOP_VAR) {
			printf("[0x%x]stop Var: 0x%x,[%d]",
				STOP_VAR, *p, __LINE__);
			goto exit;
		}
	}

	p++; //move address cross START_VAR
	NP_PACK *Ptr= (NP_PACK *)p;
	if(Ptr->MO_TYP != mt)  	goto exit;
	if(Ptr->MO_ADD != ma)  	goto exit;
	if(Ptr->MO_CMD != cmd) 	goto exit;
	if(Ptr->MO_ACK != 0x00) goto exit;

	//add CRC check
	crc = *(p+5+rl+0) | *(p+5+rl+1) << 8;
	U16 calCrc = crc16(p, rl + 5, 0);
	if(calCrc != crc) {
		printf("recvData CRC error [0x%x,0x%x],[%d]\n",
			calCrc, crc, __LINE__);
		goto crcErr;
	}

	ParseInfo(ma, Ptr->MO_CMD, Ptr->MO_DTU);
#endif
	return 0;
exit:
	//check start and stop flag
	printf("Miss start or stop flag\n");
	cJSON *object = cJSON_CreateObject();
	cJSON_AddStringToObject(object, "topic", my_topic);
	cJSON_AddNumberToObject(object, "pa_num", (int)ma);
	cJSON_AddNumberToObject(object, "switch_status", -1);
	cJSON_AddStringToObject(object, "backpower", "N/A");
	cJSON_AddStringToObject(object, "patemp", "N/A");
	cJSON_AddStringToObject(object, "paalc", "N/A");
	cJSON_AddStringToObject(object, "swr", "N/A");
	cJSON_AddStringToObject(object, "paatt", "N/A");
	cJSON_AddStringToObject(object, "fowardpower", "N/A");
	cJSON_AddStringToObject(object, "w_txpower", "N/A");
	cJSON_AddStringToObject(object, "w_temp", "N/A");
	cJSON_AddStringToObject(object, "w_vswr", "N/A");
	cJSON_AddStringToObject(object, "w_padev", "N/A");
	char *send_str = cJSON_PrintUnformatted(object);
	cJSON_Delete(object);
	if (!send_str) {
		printf("pa info json to string error!\n");
		return -1;
	}
	printf("pa status query str:%s\n", send_str);
	pthread_mutex_lock(&http_send_lock);
	http_send(args_info.painfo_api_arg, send_str, NULL, NULL);
	pthread_mutex_unlock(&http_send_lock);
	free(send_str);
	return -1;
crcErr:
	//crc check fail
	printf("Crc check fail for receive Data\n");
	return -2;
}
/* 打开串口设备并初始化 */
void init_sh_serial_port()
{
	printf("Try to open Serial Port\n");
	//open serial
	serial_fd = openPort(COM2);
	if(serial_fd == -1) {
		printf("Serial open failed\n");
		return;
	}
	printf("Serial open successfully,serial_fd:%d\n",serial_fd);
	int send_recv_timeout = 1000 * 5;
	setsockopt(serial_fd, SOL_SOCKET, SO_SNDTIMEO,
		(char *)&send_recv_timeout, sizeof(int));
	setsockopt(serial_fd, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&send_recv_timeout, sizeof(int));

	//indicate serial has been opened
	printf("cfg Serial option...\n");
	//set and configure serial
	if(set_opt(serial_fd, 19200, 8, 'N', 1) != 0) {
		perror("set_opt error\n");
		close(serial_fd);
		return;
	}
	return;
}

/* 获取功放状态信息 */
static int get_serial_status(U8 ma)
{
	struct MPK pk;
	memset(&pk, 0, sizeof(pk));
	fillPkInfo(&pk, DATA_SEND_TYPE_GET_ANT_STATUS);
	return sendAndParse(ma, &pk);

}
/* 控制功放开关 */
static int set_serial_switch(U8 ma,int cmd_type)
{
	struct MPK pk;	
	memset(&pk, 0, sizeof(pk));
	fillPkInfo(&pk, cmd_type);
	return sendAndParse(ma, &pk);
}
/* 发送信息 */
S32 send_sh_serial_request(U8 ma, int cmd_type)
{
	struct MPK pk;	
	memset(&pk, 0, sizeof(pk));
	fillPkInfo(&pk, cmd_type);
	return sendAndParse(ma, &pk);
}


#if 0
void print_help()
{
	printf("version: 1.0.1\n"
		" 	run with addr_num [cmdNum]\n"
		"addr_num:\n"
		"        0 ~ 7 ,have 8 serial port.\n"
		"cmdNum:\n"
		"        default: 0\n"
		"        0  get ANT status information.\n"
		"        1  turn ANT switch up.\n"
		"        2  turn ANT switch off.\n");
}
void main(int argc, char *argv[])
{
	int ret = 0;
	if (argc < 2) {
		print_help();	
		return;
	}
	printf("Try to init Serial Port\n");
	init_serial_port(); 
	if(serial_fd == -1) {
		printf("Serial init failed\n");
		return;
	}
	printf("----%s\n", argv[1]);
	U8 ma;
	sscanf(argv[1], "%x", &ma);
	printf("-----ma------%x\n", ma);
	ret = get_serial_status(ma);
	if (ret != 0) {
		printf("get ANT status error!\n");
	}
	if (argc < 3) return;
	int cmd_in = atoi(argv[2]);
	ret = set_serial_switch(ma, cmd_in);
	if (ret != 0) {
		printf("control ANT switch error!\n");
	}
	close(serial_fd);
	return;
}
#endif
