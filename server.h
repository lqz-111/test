#ifndef __SERVER__H__
#define __SERVER__H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include "sqlite3.h"
#include <linux/in.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define DEV_UART 			"/dev/ttyUSB0"
#define STORAGE_NUM	5
#define GOODS_NUM 10
#define STORAGE_NO 2 // 仓库号
#define SQLITE_WAREHOUSE "/dat/warehouse.db"
#define N 32 // 共享内存大小（其实只需要知道共享内存首地址，所以在这大小无所谓）
char errflag = 0;



//仓库编号
#define STORE1  0x40
#define STORE2  0x80
#define STORE3  0xc0

//设备编号
#define FAN  0x00
#define BEEP 0x10
#define LED  0x20

enum MO_CMD{
	FAN_OFF = STORE2|FAN|0x00,
	FAN_1 = STORE2|FAN|0x01,
	FAN_2 = STORE2|FAN|0x02,
	FAN_3 = STORE2|FAN|0x03,

	BEEP_OFF = STORE2|BEEP|0x00,
	BEEP_ON = STORE2|BEEP|0x01,
	BEEP_ALRRM_OFF = STORE2|BEEP|0x02,
	BEEP_ALRRM_ON = STORE2|BEEP|0x03,

	LED_OFF = STORE2|LED|0x00,
	LED_ON = STORE2|LED|0x01,
};

struct env_info
{
	uint8_t head[3];	 //标识位st:
	uint8_t type;		 //数据类型
	uint8_t snum;		 //仓库编号
	uint8_t temp[2];	 //温度	
	uint8_t hum[2];		 //湿度
	uint8_t x;			 //三轴信息
	uint8_t y;
	uint8_t z;
	uint32_t ill;		 //光照
	uint32_t bet;		 //电池电量
	uint32_t adc; 		 //电位器信息
};

struct storage_goods_info
{
	unsigned char goods_type;
	unsigned int goods_count;
};

struct storage_info
{
	unsigned char storage_status;				// 0:open 1:close
	unsigned char led_status;
	unsigned char buzzer_status;
	unsigned char fan_status;
	unsigned char seg_status;
	signed char x;
	signed char y;
	signed char z;
	char samplingTime[20];
	float temperature;
	float temperatureMIN;
	float temperatureMAX;
	float humidity;
	float humidityMIN;
	float humidityMAX;
	float illumination;
	float illuminationMIN;
	float illuminationMAX;
	float battery;
	float adc;
	float adcMIN;
	struct storage_goods_info goods_info[GOODS_NUM];
};
//环境信息结构体
struct env_info_clien_addr
{
	struct storage_info storage_no[STORAGE_NUM];	
};
// 共享内存结构体
struct shm_addr
{
	char cgi_status;
	char qt_status;
	struct env_info_clien_addr rt_status;
};
//设置最大值最小值结构体
struct setEnv
{
	int temMAX; //10-50
	int temMIN; //10-50
	int humMAX; //10-50
	int humMIN; //10-50
	int illMAX; //10-500
	int illMIN; //10-500
};

//数据转换后的环境信息
struct conver_env_info {
	int snum;		 //仓库编号
	float temperature;	 //温度	
	float humidity;		 //湿度
	float ill;		 //光照
	float bet;		 //电池电量
	float adc; 		 //电位器信息

	signed char x;			 //三轴信息
	signed char y;			 
	signed char z;			 
};

typedef struct
{
	char name[32];//用户名
	char password[32];//密码
	int cmd;
}MSG; 
//消息队列结构体
struct msg
{
	long type;
	long msgtype;
	unsigned char text[N];
};
struct msg msg_buf = {0, 0, {0}};

sem_t sem_cmd,sem_shmget;

void do_register();//创建设置高级设置数据库函数
int do_login(int acceptfd,MSG msg);//用户登录函数
void update_func();//修改数据函数

void *pthread_signal(void *arg);//通信连接线程
void *pthread_client_request(void *arg);//处理消息队列里请求的线程.
void *pthread_refresh(void *arg);//更新共享内存里的实时数据线程.
//void *pthread_sqlite(void *arg);//数据库线程.
void *pthread_transfer(void *arg);//接收M0数据线程.
void *phtread_led_buzzer(void *arg);//创建发送命令线程. 
void led_on();
void led_off();
void buzzer_on();
void buzzer_off();


#endif




