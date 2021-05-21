#include "server.h"
#include"sem.h"

int semid;
static float dota_atof (char unitl)
{
	if (unitl > 100)
	{
		return unitl / 1000.0;
	}
	else if (unitl > 10)
	{
		return unitl / 100.0;
	}
	else
	{
		return unitl / 10.0;
	}
}

static int dota_atoi (const char *cDecade)
{
	int result = 0;
	if (' ' != cDecade[0])
	{
		result = (cDecade[0] - 48) * 10;
	}
	result += cDecade[1] - 48;
	return result;
}

static float dota_adc (unsigned int ratio)
{
	return ((ratio * 3.3) / 1024);
}

//串口初始化
void serial_init(int fd)
{
	struct termios options;
	tcgetattr(fd, &options);
	options.c_cflag |= ( CLOCAL | CREAD );
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CSTOPB;
	options.c_iflag |= IGNPAR;
	options.c_iflag &= ~(ICRNL | IXON);
	options.c_oflag = 0;
	options.c_lflag = 0;

	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);
	tcsetattr(fd,TCSANOW,&options);
}
struct setEnv envdata = {0};

int acceptfd;//用于通信的文件描述符
int dev_uart_fd;//USB串口文件描述符   	
int type;//控制发送命令。具体是给A53还是M0
int flag = 0;//登录成功置位1
int flag_update = 0;
struct shm_addr *shm_buf = NULL;
int main(int argc, char *argv[])
{

	int val = 1;
	int serverfd,ret;
	int recvbytes;
	char buf[128];

	if(argc < 3)
	{
		printf("please use %s <IP> <PORT>.\n",argv[0]);
		exit(-1);
	}

	//创建设置高级设置数据库函数
	//do_register();

	if(sem_init(&sem_cmd,0,1) < 0)
	{
		perror("sem_cmd_init error");
		return -1;
	}
	if(sem_init(&sem_shmget,0,1) < 0)
	{
		perror("sem_shmget_init error");
		return -1;
	}

	//创建套接字文件，用于连接
	serverfd = socket(AF_INET,SOCK_STREAM,0);

	//允许地址端口号重用
	setsockopt(serverfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));

	//填充结构体sockaddr_in
	struct sockaddr_in serverAddr,clientAddr;	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));  // 端口号 ,atoi()字符串转整形
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);  //IP
	socklen_t addrlen = sizeof(serverAddr);

	//把socket()函数返回的文件描述符和IP、端口号进行绑定;
	ret = bind(serverfd,(struct sockaddr *)&serverAddr,addrlen);
	if(ret<0)
	{
		perror("Failed to bind");
		return -1;
	}
	printf("bind ok.\n");

	//将socket()返回的文件描述符的属æ§，由主动变为被动;
	ret = listen(serverfd,10);
	if(ret<0)
	{
		perror("Failed to bind");
		return -1;
	}
	printf("listen ok.\n");		
	//创建的线程对象
	pthread_t tid_signal,tid_request,tid_refresh,tid_transfer,tid_led_buzzer;

	//创建处理消息队列里请求的线程.
	if(pthread_create(&tid_request,NULL,pthread_client_request,NULL) != 0)
	{
		printf("创建处理消息队列里请求的线程失败\n");
		perror("error");
		return -1;
	}
	printf("11111111\n");
	//创建更新共享内存里的实时数据线程.
	if(pthread_create(&tid_refresh,NULL,pthread_refresh,NULL) != 0)
	{
		printf("创建更新共享内存里的实时数据线程失败\n");
		perror("error");
		return -1;
	}
	printf("2222222222\n");
	//创建接收M0数据线程
	if(pthread_create(&tid_transfer,NULL,pthread_transfer,NULL) != 0)
	{
		printf("创建接收M0数据线程失败\n");
		perror("error");
		return -1;
	}
	printf("3333333333\n");
	while(1)
	{
		acceptfd = accept(serverfd,NULL,NULL);
		printf("acceptfd ok.\n");

		//通信线程,并判断指令.
		if(pthread_create(&tid_signal,NULL,pthread_signal,NULL) != 0)
		{
			printf("通信连接线程失败\n");
			perror("error");
			return -1;
		}
		printf("444444444444\n");
		/*
		//创建发送命令线程
		if(pthread_create(&tid_led_buzzer,NULL,phtread_led_buzzer,NULL) != 0)
		{
		printf("创建发送命令线程失败\n");
		perror("error");
		return -1;
		}
		*/

	}
	return 0;
}

//创建设置高级设置数据库函数
void do_register()
{
	struct setEnv new; //环境变量临时结构体
	sqlite3 *db;
	char buf[128] = {0};
	char *errmsg = NULL;
	if (sqlite3_open(SQLITE_WAREHOUSE,&db) != 0)
	{
		fprintf(stderr,"sqlite3_open failed: %s\n",sqlite3_errmsg(db));
		exit(-1);
	}
	printf("sqlite3_open ok.\n");
	/*
	   if(sqlite3_exec(db,"create table env(dev_no int,temMAX int,temMIN int,humMAX int,humMIN int,illMAX int,illMIN int);",NULL,NULL,&errmsg) != 0)
	   {
	   fprintf(stderr,"sqlite3_exec err:%s\n",errmsg);
	   exit(-1);
	   }
	   printf("sqlite3_exec create ok\n");

	//sprintf(buf,"insert into env values(2,50,10,200,1,100,1);");
	sprintf(buf,"insert into env values(2,10,50,1,200,1,100);");
	if(sqlite3_exec(db,buf,NULL,NULL,&errmsg) != 0)
	{
	fprintf(stderr,"insert failed %s\n",errmsg);
	exit(-1);
	}*/

	sqlite3_close(db);
}

//用户登录函数
int do_login(int acceptfd,MSG msg)
{
	char buf[32] = {0};
	/*char sql[128] = {0};
	  strcpy(buf,"select err");
	  sqlite3 *db;
	  char *errmsg = NULL;
	  sprintf(sql,"select * from stu where name=\"%s\" and password=\"%s\";",msg.name,msg.password);

	  if(sqlite3_open("text.db",&db) != 0)
	  {
	  fprintf(stderr,"sqlite3_open failed: %s\n",sqlite3_errmsg(db));
	  exit(-1);
	  }
	  printf("sqlite3_open ok.\n");

	  char **resultp;
	  int nrow;
	  int ncolumn;

	  if(sqlite3_get_table(db,sql,&resultp,&nrow,&ncolumn,&errmsg) != 0)
	  {
	  fprintf(stderr,"sqlite3_get_table failed: %s\n",errmsg);
	  printf("name err\n");
	  send(acceptfd,buf,sizeof(buf),0);
	  exit(-1);		
	  }
	  else
	  {
	  if(nrow==0)
	  {
	  bzero(buf,sizeof(buf));
	  printf("帐号或密码错误!\n");
	  strcpy(buf,"get_table err");
	  send(acceptfd,buf,sizeof(buf),0);
	  return 0;
	  }
	  if(nrow==1)
	  {
	  printf("select * from stu where name =\"%s\" and password =\"%s\";\n",msg.name,msg.password);
	  bzero(buf,sizeof(buf));
	  strcpy(buf,"1");
	  send(acceptfd,buf,sizeof(buf),0);
	  printf("登陆成功\n");
	  return 1;

	  }

	  }
	  sqlite3_close(db);
	  */
	strcpy(buf,"1");
	send(acceptfd,buf,2,0);
	flag = 1;
	return 1;
}
//修改数据函数
void update_func()
{

	char buf[128] = {0};
	sqlite3 *db;
	char *errmsg = NULL;	
	sprintf(buf,"update env set temMAX=%d,temMIN=%d,humMAX=%d,humMIN=%d,illMAX=%d,illMIN=%d where dev_no=2;",envdata.temMAX,envdata.temMIN,\
			envdata.humMAX,envdata.humMIN,envdata.illMAX,envdata.illMIN);
	if(sqlite3_open(SQLITE_WAREHOUSE,&db) != 0)
	{
		//fprintf(stderr,"sqlite3_open failed: %s\n",sqlite3_errmsg(db));
		printf("open err\n");
		exit(-1);
	}
	if(sqlite3_exec(db,buf,NULL,NULL,&errmsg) != 0)
	{
		//fprintf(stderr,"select failed: %s\n",errmsg);
		printf("exec err\n");
		exit(-1);
	}
	printf("update  ok\n");

}

//通信连接线程
void *pthread_signal(void *arg)
{
	enum MO_CMD cmd;
	int recvbytes;
	MSG msg = {0};
	char buf[32] = {0};
	printf("这里是通信线程\n");
	while(1)
	{

		recvbytes=recv(acceptfd,&msg,sizeof(msg),0);		
		//recvbytes=recv(acceptfd,buf,sizeof(buf),0);
		//printf("%s   %s\n",msg.name,msg.password);
		if(recvbytes == -1)
		{
			printf("re failed\n");
			continue;
		}
		else if(recvbytes == 0)
		{
			printf("client shutdown\n");
			close(acceptfd);
			break;
		}
		else
		{
			if(do_login(acceptfd,msg))
			{
				printf("通信成功111\n");
				switch(msg.cmd)
				{
				case 1:led_on();break;
				case 2:led_off();break;
				case 3:buzzer_on();break;
				case 4:buzzer_off();break;
				case 5:cmd = LED_ON,write(dev_uart_fd,&cmd,1);break;
				case 6:cmd = LED_OFF,write(dev_uart_fd,&cmd,1);break;
				case 7:cmd = BEEP_ON,write(dev_uart_fd,&cmd,1);break;
				case 8:cmd = BEEP_OFF,write(dev_uart_fd,&cmd,1);break;
				case 9:cmd = FAN_OFF,write(dev_uart_fd,&cmd,1);break;
				case 10:cmd = FAN_1,write(dev_uart_fd,&cmd,1);break;
				case 11:cmd = FAN_2,write(dev_uart_fd,&cmd,1);break;
				case 12:cmd = FAN_3,write(dev_uart_fd,&cmd,1);break;
				default:break;
				}				
			}
		}
	}
	close(acceptfd);	
}

//创建接收M0数据线程
void *pthread_transfer(void *arg)
{
	struct env_info devdata = {0};
	struct conver_env_info data = {0};
	dev_uart_fd = open(DEV_UART,O_RDWR);
	if(dev_uart_fd < 0)
	{
		printf("打开USB串口失败\n");
		perror("error");
		exit(-1);
	}
	serial_init (dev_uart_fd);
	while(1)
	{
		read(dev_uart_fd,&devdata,sizeof(devdata));

		data.snum = devdata.snum;
		data.temperature = devdata.temp[0] + dota_atof(devdata.temp[1]);
		data.humidity = devdata.hum[0] + dota_atof(devdata.hum[1]);
		data.ill = dota_atof(devdata.ill);
		data.bet = dota_atof(devdata.bet);
		data.adc = dota_adc(devdata.adc);
		data.x = devdata.x;
		data.y = devdata.y;
		data.z = devdata.z;

		//printf("snum:%d temp:%.2f hum:%.2f ill:%.2f bet:%.2f adc:%.2f x:%d y:%d z:%d \n",data.snum,data.temperature,\
		//		data.humidity,data.ill,data.bet,data.adc,data.x,data.y,data.z);
		if(flag == 1)
		{
			send(acceptfd,&data,sizeof(data),0);
		}
		sem_p(semid,0);
		//把数据发送到共享内存		
		shm_buf->rt_status.storage_no[STORAGE_NO].storage_status = 1;
		shm_buf->rt_status.storage_no[STORAGE_NO].x = data.x;
		shm_buf->rt_status.storage_no[STORAGE_NO].y = data.y;
		shm_buf->rt_status.storage_no[STORAGE_NO].z = data.z;
		shm_buf->rt_status.storage_no[STORAGE_NO].temperature = data.temperature;
		shm_buf->rt_status.storage_no[STORAGE_NO].humidity = data.humidity;
		shm_buf->rt_status.storage_no[STORAGE_NO].illumination = data.ill;
		shm_buf->rt_status.storage_no[STORAGE_NO].battery = data.bet;
		shm_buf->rt_status.storage_no[STORAGE_NO].adc = data.adc;

		//最大值最小值设置
		if(flag_update == 0)
		{
			shm_buf->rt_status.storage_no[STORAGE_NO].temperatureMIN = 11;
			shm_buf->rt_status.storage_no[STORAGE_NO].temperatureMAX = 49;
			shm_buf->rt_status.storage_no[STORAGE_NO].humidityMIN = 11;
			shm_buf->rt_status.storage_no[STORAGE_NO].humidityMAX = 49;
			shm_buf->rt_status.storage_no[STORAGE_NO].illuminationMIN = 11;
			shm_buf->rt_status.storage_no[STORAGE_NO].illuminationMAX = 200;
			
		}
		if(flag_update == 1)
		{
			shm_buf->rt_status.storage_no[STORAGE_NO].temperatureMIN = envdata.temMIN;
			shm_buf->rt_status.storage_no[STORAGE_NO].temperatureMAX = envdata.temMAX;
			shm_buf->rt_status.storage_no[STORAGE_NO].humidityMIN = envdata.humMIN;
			shm_buf->rt_status.storage_no[STORAGE_NO].humidityMAX = envdata.humMAX;
			shm_buf->rt_status.storage_no[STORAGE_NO].illuminationMIN = envdata.illMIN;
			shm_buf->rt_status.storage_no[STORAGE_NO].illuminationMAX = envdata.illMAX;
		}
		sem_v (semid, 0);

		memset(&data,0,sizeof(data));
		sleep(1);
	}
}

//处理消息队列里请求的线程
void *pthread_client_request(void *arg)
{
	key_t key; 
	int msgid; 
	char device[6] = {0};
	msg_buf.text[0] = 0;
	memset(&msg_buf,0,sizeof(msg_buf));

	if((key = ftok("./app", 'g')) < 0)
	{
		perror("ftok");
		exit(1);
	}

	if((msgid = msgget(key,IPC_CREAT|IPC_EXCL|0666)) < 0)
	{
		if(errno == EEXIST)
		{
			msgid = msgget(key,0666);
		}
		else
		{
			perror("msgget err");
			exit(-1);
		}
	}
	while(1)
	{
		int retrcv=msgrcv(msgid,&msg_buf,sizeof(msg_buf)-sizeof(long),0,0);
		printf("text[0] = %d\n",msg_buf.text[0]);
		printf("hhhhhhhh\n");
		if(retrcv==-1)
		{
			printf("Not recv this msg\n");
		}
		//下是控制A53板
		if(msg_buf.msgtype == 1L)
		{
			if(msg_buf.text[0] == 17)
			{
				led_on();
			}
			if(msg_buf.text[0] == 16)
			{
				led_off();
			}

		}
		if(msg_buf.msgtype == 2L)
		{
			if(msg_buf.text[0] == 1)
			{
				buzzer_on();
			}
			if(msg_buf.text[0] == 0)
			{
				buzzer_off();
			}

		}
		//以下是控制M0板
		if(msg_buf.msgtype == 4L)
		{
			write(dev_uart_fd,msg_buf.text,1);
		} 
		if(msg_buf.msgtype == 5L)
		{
			envdata = *(struct setEnv *)(msg_buf.text + 1);
			flag_update = 1;
		//	printf("flag_update=%d\n",flag_update);
		//	printf("update  endl1\n");
			update_func();
			printf("update  endl2\n");
			//printf("text[0]:%c text[1]:%d text[2]:%d text[3]:%d text[4]:%d text[5]:%d\n",msg_buf.text[0],\
			//		new.temMAX,new.temMIN,new.humMAX,new.humMIN,new.illMAX);
		}
	}
}

//更新共享内存里的实时数据线程.
void *pthread_refresh(void *arg)
{
	key_t key;
	int shmid;
	//创建共享内存
	key = ftok("./app",'i');
	if(key < 0)
	{
		perror("ftok err");
		exit(-1);
	}

	if((semid  = semget(key,1,IPC_CREAT|IPC_EXCL|0666)) < 0)   //	信号量
	{
		if(errno == EEXIST)
			semid = semget(key, 1, 0666);
		else
		{
			perror("semget err");
			exit(-1);
		}
	}
	init_sem(semid,0,1);


	shmid =shmget(key,32,IPC_CREAT|IPC_EXCL|0666);
	if(shmid < 0)
	{
		if(errno==EEXIST)
		{
			shmid=shmget(key,32,0666);
		}
		else
		{
			perror("shmget err");
			exit(-1);
		}
	}
	//2.映射
	if((shm_buf = (struct shm_addr*)shmat(shmid, NULL, 0)) == (void*)-1)
	{
		perror("shmat");
		exit(1);
	}
}


//高级设置模块




/*
//创建发送命令线程.
void *phtread_led_buzzer(void *arg)
{

enum MO_CMD cmd;
//sem_wait(&sem_cmd);//申请资源
//recv(acceptfd,&type,sizeof(type),0);

switch(type)
{
case 1:led_on();break;
case 2:led_off();break;
case 3:buzzer_on();break;
case 4:buzzer_off();break;
case 5:cmd = LED_ON,write(dev_uart_fd,&cmd,1);break;
case 6:cmd = LED_OFF,write(dev_uart_fd,&cmd,1);break;
case 7:cmd = BEEP_ON,write(dev_uart_fd,&cmd,1);break;
case 8:cmd = BEEP_OFF,write(dev_uart_fd,&cmd,1);break;
case 9:cmd = FAN_OFF,write(dev_uart_fd,&cmd,1);break;
case 10:cmd = FAN_1,write(dev_uart_fd,&cmd,1);break;
case 11:cmd = FAN_2,write(dev_uart_fd,&cmd,1);break;
case 12:cmd = FAN_3,write(dev_uart_fd,&cmd,1);break;
default:break;
}
if(type == 0)
{
break;
}

}
*/

//A53板的LED模块和蜂鸣器模块
void led_on()
{
	printf("LED灯亮\n");
	int led_on_fd;
	char type = 'a';
	led_on_fd = open("./ledfeng",O_RDWR);
	if(led_on_fd < 0)
	{
		perror("led_on error");
		exit(-1);
	}
	write(led_on_fd,&type,sizeof(type));

}
void led_off()
{
	printf("LED灯不亮\n");
	int led_off_fd;
	char type = 'b';
	led_off_fd = open("./ledfeng",O_RDWR);
	if(led_off_fd < 0)
	{
		perror("led_off error");
		exit(-1);
	}
	write(led_off_fd,&type,sizeof(type));
}
void buzzer_on()
{
	printf("蜂鸣器响\n");
	int buzzer_on_fd;
	char type = 'c';
	buzzer_on_fd = open("./ledfeng",O_RDWR);
	if(buzzer_on_fd < 0)
	{
		perror("buzzer_on error");
		exit(-1);
	}
	write(buzzer_on_fd,&type,sizeof(type));
}
void buzzer_off()
{
	printf("蜂鸣器不响\n");	
	int buzzer_off_fd;
	char type = 'd';
	buzzer_off_fd = open("./ledfeng",O_RDWR);
	if(buzzer_off_fd < 0)
	{
		perror("led_off error");
		exit(-1);
	}
	write(buzzer_off_fd,&type,sizeof(type));	
}

