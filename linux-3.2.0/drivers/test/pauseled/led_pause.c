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


/************************/
#define GPIO_TO_PIN(bank, gpio) (32*(bank)+(gpio))

int pasue_open(struct inode *inode, struct file *filp);
int pasue_release(struct inode *inode, struct file *filp);
long pasue_ioctl( struct file *filp,unsigned int cmd, unsigned long arg);

static struct cdev pasue_cdev;
static struct class *pasue_class;
static dev_t pasue_ndev;

#define LED_ON  1
#define LED_OFF 0


static void led_on(void)
{
          gpio_set_value(GPIO_TO_PIN(1,25), 1);
}

static void led_off(void)
{
           gpio_set_value(GPIO_TO_PIN(1,25), 0);
}

//…Í«ÎGPIOø⁄
static void led_init(void)
{
	int result;
	result = gpio_request(GPIO_TO_PIN(1, 25), "pausedriver");
	if(result != 0)
		printk("gpio_request(1_25) failed \n");
	
	result = gpio_direction_output(GPIO_TO_PIN(1, 25), 0);	
	if(result != 0)
		printk("gpio_direction(1_25) failed \n");

}

static const struct file_operations pasue_fops = 
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = pasue_ioctl,
	.open = pasue_open,
	.release = pasue_release,
};


int pasue_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int pasue_release(struct inode *inode, struct file *filp)
{
	return 0;
}

long pasue_ioctl( struct file *filp,unsigned int cmd, unsigned long v)
{
	switch(cmd)
	{
		case LED_ON:
			 led_on();
		   break;

		case LED_OFF:
			 led_off();
		   break;
		
		default:
			return -ENOTTY;
	}
	return 0;
}


static int __init pasue_init(void)
{

	int ret;
	led_init();
	
	cdev_init(&pasue_cdev, &pasue_fops);
	ret = alloc_chrdev_region(&pasue_ndev, 0, 1, "pause_driver");
	if(ret < 0)
	{
		printk("alloc_chrdev_region fail \n");
	}

	printk("major = %d, minor = %d \n", MAJOR(pasue_ndev), MINOR(pasue_ndev));
	
	ret = cdev_add(&pasue_cdev, pasue_ndev, 1);
	if(ret < 0)
	{
		printk("cdev_add fail \n");
		return ret;
	}

	pasue_class = class_create(THIS_MODULE, "pauseled_driver");
	device_create(pasue_class, NULL, pasue_ndev, NULL, "pauseled_driver");
	
	return 0;
}

void pasue_cleanup(void)
{
	cdev_del(&pasue_cdev);
	unregister_chrdev_region(pasue_ndev, 1);
	device_destroy(pasue_class, pasue_ndev);
	class_destroy(pasue_class);
}
module_init(pasue_init);
module_exit(pasue_cleanup);

MODULE_AUTHOR("xufeng");
MODULE_LICENSE("GPL");



