#include <linux/leds.h>
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

#include<linux/timer.h>
#include<linux/spinlock.h>

/************************/
#define GPIO_TO_PIN(bank, gpio) (32*(bank)+(gpio))

int light_open(struct inode *inode, struct file *filp);
int light_release(struct inode *inode, struct file *filp);
long light_ioctl( struct file *filp,unsigned int cmd, unsigned long arg);

static struct timer_list led_low_timer;   //led 定时器
static struct timer_list self_check_timer;   //自检 定时器

typedef enum _LED_LEVEL
{
	LEVEL_NONE = 0,
	LEVEL_LOW,
	LEVEL_MID,
	LEVEL_HIGH,
	LEVEL_OFF,	
} LED_LEVEL;

static LED_LEVEL led_level = LEVEL_NONE;
static struct cdev light_cdev;
static struct class *light_class;
static dev_t light_ndev;
static spinlock_t led_lock;   //自旋锁

#define SPEED_LOW  1
#define SPEED_MID  3
#define SPEED_HIGH 5
#define LED_OFF    0
#define LED_SELF_CHECK 7

void low_timer_fn(unsigned long arg)
{
	   int val, temp;
	   unsigned long flag = arg;    //根据传进来的值确定级别
//	   spin_lock_irq(&led_lock);    //临界保护
	  
	  if (flag == 1)
		{
			 temp = gpio_get_value(GPIO_TO_PIN(1,23));
			 gpio_set_value(GPIO_TO_PIN(1,23), !temp); 
			 led_low_timer.expires = jiffies + HZ;
		}
    else if(flag == 2)
    {
    	 val = gpio_get_value(GPIO_TO_PIN(1,22));
       gpio_set_value(GPIO_TO_PIN(1,22), !val);
       
    	 led_low_timer.expires = jiffies + HZ/4;
    }
    
     add_timer(&led_low_timer);
     
//     spin_unlock_irq(&led_lock);
}

void self_led_timer_fn(unsigned long arg)
{
	  static int led_cnt = 0;
	
	  if (led_cnt == 0)
		{
			 gpio_set_value(GPIO_TO_PIN(1,22), 1);
			 gpio_set_value(GPIO_TO_PIN(1,23), 0); 
			 self_check_timer.expires = jiffies + HZ;
			 add_timer(&self_check_timer);
			     led_cnt++;
		}
    else if(led_cnt == 1)
    {
       gpio_set_value(GPIO_TO_PIN(1,23), 1);
       gpio_set_value(GPIO_TO_PIN(1,22), 0);       
    	 self_check_timer.expires = jiffies + HZ;
    	 add_timer(&self_check_timer);
    	     led_cnt++;
    }	
    else 
    {
    	 gpio_set_value(GPIO_TO_PIN(1,23), 0);
       gpio_set_value(GPIO_TO_PIN(1,22), 0);
       del_timer(&self_check_timer); 
       led_cnt = 0;  	
    }
}

void low_timer_init(unsigned long arg)
{
	init_timer(&led_low_timer);
	led_low_timer.expires = jiffies + 1;
	led_low_timer.function = low_timer_fn;
	led_low_timer.data = arg;
	
	add_timer(&led_low_timer);
}

void self_check_timer_init(void)
{
	init_timer(&self_check_timer);
	self_check_timer.expires = jiffies + 1;
	self_check_timer.function = self_led_timer_fn;
	
	add_timer(&self_check_timer);	
}

void led_on(void)
{
 //         gpio_set_value(GPIO_TO_PIN(1,23), 1);
          gpio_set_value(GPIO_TO_PIN(1,23), 1);
}

void led_off(void)
{
           gpio_set_value(GPIO_TO_PIN(1,23), 0);
           gpio_set_value(GPIO_TO_PIN(1,22), 0);
}

//申请GPIO口
void led_init(void)
{
	int result;
	result = gpio_request(GPIO_TO_PIN(1, 23), "leddriver");
	if(result != 0)
		printk("gpio_request(1_22) failed \n");
	
	result = gpio_request(GPIO_TO_PIN(1, 22), "leddriver");
	if(result != 0)
		printk("gpio_request(1_23) failed \n");
	
	result = gpio_direction_output(GPIO_TO_PIN(1, 23), 0);	
	if(result != 0)
		printk("gpio_direction(1_22) failed \n");

	result = gpio_direction_output(GPIO_TO_PIN(1, 22), 0);	
	if(result != 0)
		printk("gpio_direction(1_23) failed \n");
}

static const struct file_operations light_fops = 
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = light_ioctl,
	.open = light_open,
	.release = light_release,
};


int light_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int light_release(struct inode *inode, struct file *filp)
{
	return 0;
}

long light_ioctl( struct file *filp,unsigned int cmd, unsigned long v)
{
	unsigned long arg;    //定时器中断函数里面的级别参数
	
	switch(cmd)
	{
		case SPEED_LOW :
		{
			if(led_level != LEVEL_LOW)
			{
				del_timer(&led_low_timer);
				led_on();
				led_level = LEVEL_LOW;
			}
			break;
		}	
		case SPEED_MID :	
		{ 
			if(led_level != LEVEL_MID)
			{	
				arg = 1;
		    led_off();
				del_timer(&led_low_timer);
				low_timer_init(arg);
				led_level = LEVEL_MID;
			}
			break;
		}		
		case SPEED_HIGH:	
		{	
			if(led_level != LEVEL_HIGH)
			{
				arg = 2;
		    led_off();
				del_timer(&led_low_timer);
				low_timer_init(arg);
				led_level = LEVEL_HIGH;
			}
			break;
		}
		case LED_SELF_CHECK:
		{
			self_check_timer_init();
		}
		break;		
		case LED_OFF:	
		{
			del_timer(&led_low_timer);
			led_off();
			led_level = LEVEL_NONE;
			break;
		}
				
		default:
			return -ENOTTY;
	}
	
	return 0;
}


static int __init light_init(void)
{

	int ret;
	led_init();
	
	spin_lock_init(&led_lock);
	cdev_init(&light_cdev, &light_fops);
	ret = alloc_chrdev_region(&light_ndev, 0, 1, "led_driver");
	if(ret < 0)
	{
		printk("alloc_chrdev_region fail \n");
	}

	printk("major = %d, minor = %d \n", MAJOR(light_ndev), MINOR(light_ndev));
	
	ret = cdev_add(&light_cdev, light_ndev, 1);
	if(ret < 0)
	{
		printk("cdev_add fail \n");
		return ret;
	}

	light_class = class_create(THIS_MODULE, "led_driver");
	device_create(light_class, NULL, light_ndev, NULL, "led_driver");
	
	return 0;
}

void light_cleanup(void)
{
	cdev_del(&light_cdev);
	unregister_chrdev_region(light_ndev, 1);
	device_destroy(light_class, light_ndev);
	class_destroy(light_class);
}
module_init(light_init);
module_exit(light_cleanup);

MODULE_AUTHOR("xufeng");
MODULE_LICENSE("GPL");



