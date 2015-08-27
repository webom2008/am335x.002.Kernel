#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

int main(int argc, char * argv)
{
	int i, n, fd;
	fd = open("/dev/led_driver", O_RDWR);
	if(fd < 0)
	{
		printf("can't open /dev/led_driver \n");
		exit(1);
	}

	while(1)
	{
		printf("lever_one \n");
		ioctl(fd, 1, 1);
		sleep(10);
		printf("lever_two \n");
		ioctl(fd, 3, 1);
		sleep(10);
		printf("lever_three \n");
		ioctl(fd, 5, 1);
		sleep(10);
		printf("lever_off \n");
		ioctl(fd, 0, 1);
		sleep(3);
		
	
	}
	close(fd);
	return 0;
}
