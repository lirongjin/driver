#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int LedWrite(void);
int LedRead(void);


int fd;
	

int main()
{
// 	size_t ret;
	char buf[100] = {0};
	int i;
	
	fd = open("/dev/s5pled", O_RDWR);
	if(fd < 0)
	{
		printf("open fail\n");
		perror("");
		return -1;
	}
	
	printf("write test\n");
	buf[0] = 1;
	buf[1] = 1;
	buf[2] = 1;   
	ret = write(fd, buf, 3);
	if(ret < 0)
	{
	  	perror("write");

	  	return -1;
	}
	sleep(1); 
	
	printf("read test\n");
	ret = read(fd, buf, 100);
	if(ret < 0)
	{
		printf("read fail\n");
		perro("");
		
		return -1;
	}
	printf("read success\n");
	for(i=0; i<3; i++)
	{
		printf("buf[%d] = %d\n", i, buf[i]);
	}
	
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;   
	ret = write(fd, buf, 3);
	
	while(1)
	{
		LedRead();
	}
	
	close(fd);
	
	return 0;
}

int LedWrite(void)
{
  	int i;
	char buf[100] = {0};
	  
	for(i=0; i<3; i++)
	{
		memset(buf, 0, sizeof(buf));
		buf[i] = 1;
		write(fd, buf, 3);
		sleep(1);
	}
	
	return 0;
}

int LedRead(void)
{
    	int i;
	size_t ret;
	char buf[100] = {0};
	
	
	buf[0] = 1;
	buf[1] = 1;
	buf[2] = 1;   
	write(fd, buf, 3);
	read(fd, buf, 100);
	for(i=0; i<3; i++)
	{
		printf("buf[%d] = %d\n", i, buf[i]);
	}
	sleep(1);
	
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;   
	write(fd, buf, 3);
	memset(buf, 0, sizeof(buf));
	read(fd, buf, 100);
	for(i=0; i<3; i++)
	{
		printf("buf[%d] = %d\n", i, buf[i]);
	}
	sleep(1);
	
	return 0;
}
