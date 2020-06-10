#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
    

int main()
{
    size_t ret;
    char buf[100] = {0};
    int i;
    int fd0, fd1, fd2;
    
    fd0 = open("/dev/s5pled0", O_RDWR);
    fd1 = open("/dev/s5pled1", O_RDWR);
    fd2 = open("/dev/s5pled2", O_RDWR);
    if(fd0 < 0 || fd1 < 0 || fd2 < 0)
    {
        printf("open fail\n");
        perror("");
        return -1;
    }

    while(1)
    {
        printf("write test\n");
        
        buf[0] = 0;
        ret = write(fd0, buf, 1);
        
        buf[0] = 0;
        ret = write(fd1, buf, 1);
        
        buf[0] = 0;
        ret = write(fd2, buf, 1);
        
        printf("read test\n");
        
        ret = read(fd0, buf, 1);
        printf("led0 = %d\n", buf[0]);
        
        ret = read(fd1, buf, 1);
        printf("led1 = %d\n", buf[1]);
        
        ret = read(fd2, buf, 1);
        printf("led2 = %d\n", buf[2]);
        
        sleep(1);
        
        buf[0] = 1;
        ret = write(fd0, buf, 1);
        
        buf[0] = 1;
        ret = write(fd1, buf, 1);
        
        buf[0] = 1;
        ret = write(fd2, buf, 1);
        
        printf("read test\n");
        
        ret = read(fd0, buf, 1);
        printf("led0 = %d\n", buf[0]);
        
        ret = read(fd1, buf, 1);
        printf("led1 = %d\n", buf[0]);
        
        ret = read(fd2, buf, 1);
        printf("led2 = %d\n", buf[0]);
        
        sleep(1); 
    }
    
    close(fd0);
    close(fd1);
    close(fd2);
    
    return 0;
}
