/******************************************************************************

  Copyright (C), 2005-2014, CVTE.

 ******************************************************************************
  File Name     : ext_hw_watchdog.c
  Version       : Initial Draft
  Author        : qiuweibo
  Created       : 2015/6/5
  Last Modified :
  Description   : driver for exteranl watchdog
  Function List :
  History       :
  1.Date        : 2015/6/5
    Author      : qiuweibo
    Modification: Created file

******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/watchdog.h>

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio)     (32 * (bank) + (gpio))
#define PIN_EXT_WDT                 GPIO_TO_PIN(1,30)
#define DEVICE                      "ext_wdt"
#define EXT_WDT_CMD_BASE            'W'
#define EXT_WDT_CMD_KEEPALIVE       _IOR(EXT_WDT_CMD_BASE, 5, int)

//#define _EXT_WDT_DEBUG_
#ifdef _EXT_WDT_DEBUG_
#define EXT_WDT_DEB(fmt, arg...)    printk("[ExtWDT] "fmt, ##arg)
#else
#define EXT_WDT_DEB(fmt, arg...)    do{}while(0)
#endif

#define GPIO1_BASE                  (unsigned int)(0x4804C000)
#define GPIO1_CLEARDATAOUT          (unsigned int)(GPIO1_BASE+0x190)
#define GPIO1_SETDATAOUT            (unsigned int)(GPIO1_BASE+0x194)
void early_ext_wdt_reset(void)
{
    *(volatile unsigned int *)GPIO1_SETDATAOUT = (unsigned int)(0x1 << 30);
    *(volatile unsigned int *)GPIO1_CLEARDATAOUT = (unsigned int)(0x1 << 30);
}

void ext_watchdog_reset(void)
{
    static int flag = 0;
    if (flag)
    {
        gpio_direction_output(PIN_EXT_WDT,0);
    }
    else
    {
        gpio_direction_output(PIN_EXT_WDT,1);
    }
    flag = ~flag;
}
EXPORT_SYMBOL(ext_watchdog_reset);

//@return 0: success other: error
int ext_watchdog_init(void)
{
    int ret = -1;
    ret = gpio_request_one(PIN_EXT_WDT,
                (GPIOF_DIR_OUT | GPIOF_OUT_INIT_LOW), "pin_ext_wdt");
    if (ret < 0)
    {
         EXT_WDT_DEB("-QWB-ext_watchdog_init error.\n");
    }
    
    return 0;
}
EXPORT_SYMBOL(ext_watchdog_init);

static int ext_wdt_open(struct inode *inode, struct file *filp)
{
    int ret = ext_watchdog_init();
    if (0 == ret)
    {
        ext_watchdog_reset();
        ext_watchdog_reset();
    }
    return ret;
}

static int ext_wdt_release(struct inode *inode, struct file *filp)
{
    gpio_free(PIN_EXT_WDT);
    EXT_WDT_DEB("-QWB-pin_ext_wdt dev release.\n");
    return 0;
}

static long ext_wdt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case EXT_WDT_CMD_KEEPALIVE:
         ext_watchdog_reset();
         EXT_WDT_DEB("-QWB-EXT_WDT_CMD_KEEPALIVE.\n");
         break;
    default:
         EXT_WDT_DEB("-QWB-Error: unvalid cmd.\n");
         break;
    }
    return 0;
}

struct file_operations ext_wdt_fops = {
    .owner          = THIS_MODULE,
    .open           = ext_wdt_open,
    .release        = ext_wdt_release,
    .unlocked_ioctl = ext_wdt_ioctl,
};

struct miscdevice ext_wdt_dev = {
    .minor          = MISC_DYNAMIC_MINOR,
    .name           = DEVICE,
    .fops           = &ext_wdt_fops,
};

static int __init ext_wdt_init(void)
{
    int ret;
    ret = misc_register(&ext_wdt_dev);
    if (ret) {
         printk("-QWB-Error: cannot register misc.\n");
         return ret;
    }
    printk("-QWB-misc_register %s\n", DEVICE);
    return 0;
}

static void __exit ext_wdt_exit(void)
{
    misc_deregister(&ext_wdt_dev);
    printk("-QWB-misc_deregister %s\n", DEVICE);
}

module_init(ext_wdt_init);
module_exit(ext_wdt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("QiuWeibo <qiuweibo@cvte.com>");
