
#include <linux/io.h>

#include <linux/semaphore.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/limits.h>
#include <linux/ioctl.h>
#include <linux/blk_types.h>
#include <linux/types.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <mach/gpio.h>
#include <plat/mux.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>


#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/watchdog.h>

#include<linux/timer.h>
#include<linux/spinlock.h>

/************************/
#define GPIO_TO_PIN(bank, gpio) (32*(bank)+(gpio))


int nurse_call_open(struct inode *inode, struct file *filp);
int nurse_call_release(struct inode *inode, struct file *filp);
long nurse_call_ioctl( struct file *filp,unsigned int cmd, unsigned long arg);

static struct timer_list call_nurse_timer;      //callnurse ¶¨Ê±Æ÷

typedef enum _CALL_STATUS
{
	PULSE_ON = 10,
	PULSE_OFF = 11,
	CONTINUOUS_ON = 20,
	CONTINUOUS_OFF = 21,
	INVALID_STATUS,	
} CALL_STATUS;

#define ON          1
#define OFF         0

#define ALARM_ON    1
#define ALARM_OFF   0

static CALL_STATUS call_status = CONTINUOUS_ON;
static unsigned  pulse_status = OFF;
static unsigned  continuous_status;
static struct cdev nurse_call_cdev;
static struct class *nurse_call_class;
static dev_t nurse_call_ndev;

void call_timer_fn(unsigned long arg)
{
	int temp;
	pulse_status = OFF;
	temp = gpio_get_value(GPIO_TO_PIN(3,14));
	gpio_set_value(GPIO_TO_PIN(3,14), !temp);
	del_timer(&call_nurse_timer); 
}

void handle_alarm_task(int alarm_switch)
{
	int temp;
	switch(call_status)
	{
		case PULSE_ON:
		{
			if(alarm_switch == ALARM_ON && pulse_status != ON)
			{
				pulse_status = ON;
			  temp = gpio_get_value(GPIO_TO_PIN(3,14));
			  gpio_set_value(GPIO_TO_PIN(3,14), !temp);
			  init_timer(&call_nurse_timer);
				call_nurse_timer.expires = jiffies + HZ;
				call_nurse_timer.function = call_timer_fn;
				add_timer(&call_nurse_timer);
			}
		}
		break;
		case PULSE_OFF:
		{
			if(alarm_switch == ALARM_ON && pulse_status != ON)
			{
				pulse_status = ON;
				temp = gpio_get_value(GPIO_TO_PIN(3,14));
			  gpio_set_value(GPIO_TO_PIN(3,14), !temp);
			  init_timer(&call_nurse_timer);
				call_nurse_timer.expires = jiffies + HZ;
				call_nurse_timer.function = call_timer_fn;
				add_timer(&call_nurse_timer);
			}
		}
		break;		
		case CONTINUOUS_ON:
		{
				temp = gpio_get_value(GPIO_TO_PIN(3,14));
				if(alarm_switch == ALARM_ON)
				{
					if(temp == continuous_status)
					{
				 		 gpio_set_value(GPIO_TO_PIN(3,14), !temp);						
					}	
				}
				else
				{
					if(temp != continuous_status)
					{
				 		 gpio_set_value(GPIO_TO_PIN(3,14), !temp);						
					}						
				}
	
		}
		break;	
		case CONTINUOUS_OFF:
		{
				temp = gpio_get_value(GPIO_TO_PIN(3,14));
				if(alarm_switch == ALARM_ON)
				{
					if(temp == continuous_status)
					{
				 		 gpio_set_value(GPIO_TO_PIN(3,14), !temp);						
					}	
				}
				else
				{
					if(temp != continuous_status)
					{
				 		 gpio_set_value(GPIO_TO_PIN(3,14), !temp);						
					}						
				}	
		}
		break;
		default:
		break;	
	}

}

//ÉêÇëGPIO¿Ú
void call_init(void)
{
	int result;
	result = gpio_request(GPIO_TO_PIN(3,14), "calldriver");
	if(result != 0)
		printk("gpio_request(3,14) failed \n");
	
	result = gpio_direction_output(GPIO_TO_PIN(3,14), 0);	
	if(result != 0)
		printk("gpio_direction(3,14) failed \n");
}

static const struct file_operations nurse_call_fops = 
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = nurse_call_ioctl,
	.open = nurse_call_open,
	.release = nurse_call_release,
};


int nurse_call_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int nurse_call_release(struct inode *inode, struct file *filp)
{
	return 0;
}

long nurse_call_ioctl( struct file *filp,unsigned int cmd, unsigned long v)
{
	
	switch(cmd)
	{
		case PULSE_ON :
		{
			call_status = PULSE_ON;	
			pulse_status = OFF;
			gpio_set_value(GPIO_TO_PIN(3,14), 0);			
		}	
		break;
		case PULSE_OFF :	
		{ 
			call_status = PULSE_OFF;
			pulse_status = OFF;
			gpio_set_value(GPIO_TO_PIN(3,14), 1);	
		}	
		break;	
		case CONTINUOUS_ON:	
		{	
			call_status = CONTINUOUS_ON;
			continuous_status = 0;
			gpio_set_value(GPIO_TO_PIN(3,14), 0);	
		}
		break;
		case CONTINUOUS_OFF:
		{
			call_status = CONTINUOUS_OFF;
			continuous_status = 1;
			gpio_set_value(GPIO_TO_PIN(3,14), 1);	
		}
		break;		
		case ALARM_ON:	
		{
			handle_alarm_task(ALARM_ON);
		}
		break;
		case ALARM_OFF:	
		{
			handle_alarm_task(ALARM_OFF);	
		}
		break;
				
		default:
			return -ENOTTY;
	}
	
	return 0;
}


static int __init nurse_call_init(void)
{

	int ret;
    WATCHDOG_RESET();
	call_init();
	
	cdev_init(&nurse_call_cdev, &nurse_call_fops);
	ret = alloc_chrdev_region(&nurse_call_ndev, 0, 1, "nurse_call_driver");
	if(ret < 0)
	{
		printk("alloc_chrdev_region fail \n");
	}

	printk("nurse_call_major = %d, nurse_call_minor = %d \n", MAJOR(nurse_call_ndev), MINOR(nurse_call_ndev));
	
	ret = cdev_add(&nurse_call_cdev, nurse_call_ndev, 1);
	if(ret < 0)
	{
		printk("cdev_add fail \n");
		return ret;
	}

	nurse_call_class = class_create(THIS_MODULE, "nurse_call_driver");
	device_create(nurse_call_class, NULL, nurse_call_ndev, NULL, "nurse_call_driver");
	
	return 0;
}

void nurse_call_cleanup(void)
{
	cdev_del(&nurse_call_cdev);
	unregister_chrdev_region(nurse_call_ndev, 1);
	device_destroy(nurse_call_class, nurse_call_ndev);
	class_destroy(nurse_call_class);
}
module_init(nurse_call_init);
module_exit(nurse_call_cleanup);

MODULE_AUTHOR("xufeng");
MODULE_LICENSE("GPL");
