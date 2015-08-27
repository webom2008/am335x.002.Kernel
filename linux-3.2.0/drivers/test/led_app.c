#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

#define LED_MAGIC  'w'

#define LED_LEVEL_LOW   _IO(LED_MAGIC, 1)
#define LED_LEVEL_MID   _IO(LED_MAGIC, 2)
#define LED_LEVEL_HIGH  _IO(LED_MAGIC, 3)
#define LED_LEVEL_OFF   _IO(LED_MAGIC, 0)

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
		ioctl(fd, LED_LEVEL_LOW);
		sleep(10);
		printf("lever_two \n");
		ioctl(fd,LED_LEVEL_MID);
		sleep(10);
		printf("lever_three \n");
		ioctl(fd, LED_LEVEL_HIGH);
		sleep(10);
		printf("lever_off \n");
		ioctl(fd,LED_LEVEL_OFF);
		sleep(3);
		
	
	}
	close(fd);
	return 0;
}
