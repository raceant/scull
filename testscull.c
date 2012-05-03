#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<error.h>
#include<fcntl.h>
#include<string.h>
#include<sys/types.h>
#include "testscull.h"

int main()
{
	int fd,len;
	char inbuf[20]="fuck&&shit";
	char outbuf[20];
	int quantum;
	int Ret;

	scanf("%d",&quantum);
	fd=open("/dev/scull0",O_WRONLY);
	if(fd<=0) {
		printf("Error openning the device of scull for writing!\n");
		exit(1);
	} else
		printf("Open1 scull yes!\n");

	Ret = ioctl(fd,SCULL_IOCSQSET, &quantum);
	//Ret = ioctl(fd,90, &quantum);
	if (!Ret)
		printf("successful!\n");
	close(fd);
	/*len=write(fd,inbuf,strlen(inbuf));
	if(len<0) {
		printf("Error writing to the device!\n");
		close(fd);
		exit(1);
	} else
		printf("write scull yes!\n");

	printf("writing %d bytes to the device!\n",len);
	close(fd);
	fd=open("/dev/scull0",O_RDONLY);
	if(fd<0) {
		printf("Error openning the device of scull for reading!\n");
		exit(1);
	}else
		printf("Open2 scull yes!\n");

	len=read(fd,outbuf,len);
	if(len<0) {
		printf("Error reading from the device!\n ");
		close(fd);
		exit(1);
	} else
		printf("Read scull yes!\n");

	printf("%s\n",outbuf);
*/
	return Ret;
}
