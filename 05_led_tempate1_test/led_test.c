
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include<stdlib.h>

static int fd;
int led_on(int witch);//待实现
/*
* 使用方法：  //尖括号一般表示不可省略，中括号表示可写可不写
*./led_test <0|1|2..> on  //写高电平
*./led_test <0|1|2..> off  //写低电平
*./led_test <0|1|2..>	//读电平
*/
int main(int argc, char **argv)
{

	int ret;
	char buf[2];

	int i;
	
	/* 1. 判断参数 */
	if (argc < 2) 
	{
		printf("Usage: %s <0|1|2> {on | off}\n", argv[0]);
		return -1;
	}


	/* 2. 打开文件 */
	fd = open("/dev/100ask_led", O_RDWR);
	if (fd == -1)
	{
		printf("can not open file /dev/100ask_led\n");
		return -1;
	}
	//写电平
	if(argc==3){
		buf[0]=strtol(argv[1],NULL,0);//字符串转成长整型
		if(strcmp(argv[2],"on")==0)
			buf[1]=0;
		else
			buf[1]=1;
		ret=write(fd,buf,2);
	}else{//读电平
		buf[0]=strtol(argv[1],NULL,0);//字符串转成长整型
		ret=read(fd,buf,2);
		if(ret==2){
			printf("led %d status is %s\n",buf[0],buf[1]==0 ? "on":"off");
		}
	}
	
	close(fd);
	
	return 0;
}


