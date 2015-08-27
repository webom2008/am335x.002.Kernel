
/*
 * PWM API implementation
 *
 * Copyright (C) 2011 Bill Gatliff <bgat@billgatliff.com>
 * Copyright (C) 2011 Arun Murthy <arun.murthy@stericsson.com>
 *
 * This program is free software; you may redistribute and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/pwm/pwm.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#define GPIO_TO_PIN(bank, gpio) (32*(bank)+(gpio))
#define ON        1
#define OFF       0
#define ALARM_FRED 260

int sound_duty[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 95, 0};

static const char *REQUEST_SYSFS = "sysfs";
static LIST_HEAD(pwm_device_list);
static DEFINE_MUTEX(device_list_mutex);
static struct class pwm_class;
static struct workqueue_struct *pwm_handler_workqueue;
//TEST
static struct pwm_device *g_p = NULL;


static BEEP_CONTROL           beep_control;      //beep控制结构体
static COMMON_SOUND_CONTROL    common_sound_control;
static ALARM_REMIND_CONTROL    alarm_remind_control;

static ssize_t pwm_change_freq(unsigned long freq);

void beep_timer_fn(unsigned long arg)
{
     BEEP_CONTROL * data = (BEEP_CONTROL *)arg;

     if(data->alarm_level == ALARM_LOW)
     {
        if(data->low_status == LOW_RISEONE_STATUS)
        {
            pwm_change_freq((unsigned long)ALARM_FRED);
            data->low_status = LOW_RISETWO_STATUS;
            data->once_alarm_switch = ON;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->low_status == LOW_RISETWO_STATUS && data->cnt <= 0)
        {
            data->low_status = LOW_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->low_volume_control]*3/7));
        }
        else if(data->low_status == LOW_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->low_status = LOW_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->low_volume_control]*5/7));
        }
        else if(data->low_status == LOW_KEEP_STATUS && data->cnt <= 0)
        {
            data->low_status = LOW_FALLONE_STATUS;
            data->cnt = 15;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->low_volume_control]));
        }
        else if(data->low_status == LOW_FALLONE_STATUS && data->cnt <= 0)
        {
            data->low_status = LOW_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->low_volume_control]*5/7));
        }
        else if(data->low_status == LOW_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->low_status = LOW_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->low_volume_control]*3/7));
        }
        else if(data->low_status == LOW_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->low_status = LOW_FINISH_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->low_volume_control]/7));
        }
        else if(data->low_status == LOW_FINISH_STATUS && data->cnt <= 0)
        {
            data->once_alarm_switch = OFF;
            data->low_status = LOW_END_STATUS;
            data->cnt = (data->timer_control.lowlevel_time)*100;
            data->storecnt = data->cnt;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }
        else if(data->low_status == LOW_END_STATUS && data->cnt <= 0)
        {
            data->low_status = LOW_RISEONE_STATUS;
        }

        if((data->low_status == LOW_END_STATUS) && data->storecnt != (data->timer_control.lowlevel_time)*25*4)
        {
                data->cnt = (int)((data->timer_control.lowlevel_time)*25*4 -(data->storecnt - data->cnt));
                data->storecnt = data->timer_control.lowlevel_time*25*4;
        }
        --data->cnt;
        data->beep_timer.expires =  jiffies + 1;     //10ms软中断一次
        add_timer(&data->beep_timer);
     }
     else if(data->alarm_level == ALARM_MID)    //200MS叫200MS停  叫3次   周期20MS
     {
        if(data->mid_status == MID_ONE_RISEONE_STATUS)
        {
            pwm_change_freq((unsigned long)ALARM_FRED);
            data->once_alarm_switch = ON;
            data->mid_status = MID_ONE_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->mid_status == MID_ONE_RISETWO_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_ONE_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*3/7));
        }
        else if(data->mid_status == MID_ONE_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_ONE_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*5/7));
        }
        else if(data->mid_status == MID_ONE_KEEP_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_ONE_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,sound_duty[data->mid_volume_control]);
        }
        else if(data->mid_status == MID_ONE_FALLONE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_ONE_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*5/7));
        }
        else if(data->mid_status == MID_ONE_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_ONE_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*3/7));
        }
        else if(data->mid_status == MID_ONE_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_ONE_END_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]/7));
        }
        else if(data->mid_status == MID_ONE_END_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_RISEONE_STATUS;
            data->cnt = 17;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }
        else if(data->mid_status == MID_TWO_RISEONE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->mid_status == MID_TWO_RISETWO_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*3/7));
        }
        else if(data->mid_status == MID_TWO_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*5/7));
        }
        else if(data->mid_status == MID_TWO_KEEP_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,sound_duty[data->mid_volume_control]);
        }
        else if(data->mid_status == MID_TWO_FALLONE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*5/7));
        }
        else if(data->mid_status == MID_TWO_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*3/7));
        }
        else if(data->mid_status == MID_TWO_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_TWO_END_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]/7));
        }
        else if(data->mid_status == MID_TWO_END_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_THREE_RISEONE_STATUS;
            data->cnt = 17;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }
        else if(data->mid_status == MID_THREE_RISEONE_STATUS && data->cnt <= 0)
        {
          data->mid_status = MID_THREE_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->mid_status == MID_THREE_RISETWO_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_THREE_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*3/7));
        }
        else if(data->mid_status == MID_THREE_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_THREE_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*5/7));
        }
        else if(data->mid_status == MID_THREE_KEEP_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_THREE_FALLONE_STATUS;
            data->cnt = 14;
          pwm_set_duty_percent(data->beep_device,sound_duty[data->mid_volume_control]);
        }
        else if(data->mid_status == MID_THREE_FALLONE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_THREE_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*5/7));
        }
        else if(data->mid_status == MID_THREE_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_THREE_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]*3/7));
        }
        else if(data->mid_status == MID_THREE_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_THREE_END_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->mid_volume_control]/7));
        }
        else if(data->mid_status == MID_THREE_END_STATUS && data->cnt <= 0)
        {
            data->once_alarm_switch = OFF;
            data->mid_status = MID_END_STATUS;
            data->cnt = (data->timer_control.midlevel_time)*25*4;
            data->storecnt = data->cnt;                   //19720MS
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }
        else if(data->mid_status == MID_END_STATUS && data->cnt <= 0)
        {
            data->mid_status = MID_ONE_RISEONE_STATUS;
        }
        if((data->mid_status == MID_END_STATUS) && data->storecnt != (data->timer_control.midlevel_time)*25*4)
        {
                data->cnt = (int)((data->timer_control.midlevel_time)*25*4 -(data->storecnt - data->cnt));
                data->storecnt = (data->timer_control.midlevel_time)*25*4;
        }
        --data->cnt;
        data->beep_timer.expires =  jiffies + 1;
        add_timer(&data->beep_timer);
     }
     else if(data->alarm_level == ALARM_HIGH)                /* 高级报警 */
     {
        if(data->high_status == HIGH_ONE_RISEONE_STATUS)
        {
            pwm_change_freq((unsigned long)ALARM_FRED);
            data->once_alarm_switch = ON;
            data->high_status = HIGH_ONE_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_ONE_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_ONE_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_ONE_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_ONE_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_ONE_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_ONE_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_ONE_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_ONE_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_ONE_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_ONE_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_ONE_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_ONE_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_ONE_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_RISEONE_STATUS;
            data->cnt = 4;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_TWO_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_TWO_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_TWO_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_TWO_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_TWO_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_TWO_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_TWO_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TWO_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_TWO_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_RISEONE_STATUS;
            data->cnt = 4;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_THREE_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_THREE_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_THREE_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_THREE_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_THREE_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_THREE_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_THREE_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_THREE_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_THREE_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_RISEONE_STATUS;
            data->cnt = 28;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_FOUR_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_FOUR_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_FOUR_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_FOUR_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_FOUR_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_FOUR_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_FOUR_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FOUR_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_FOUR_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_RISEONE_STATUS;
            data->cnt = 4;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_FIVE_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_FIVE_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_FIVE_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_FIVE_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_FIVE_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_FIVE_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_FIVE_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_FIVE_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_FIVE_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_RISEONE_STATUS;
            data->cnt = 60;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_SIX_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_SIX_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_SIX_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_SIX_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_SIX_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_SIX_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_SIX_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SIX_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_SIX_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_RISEONE_STATUS;
            data->cnt = 4;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_SEVEN_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_SEVEN_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_SEVEN_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_SEVEN_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_SEVEN_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_SEVEN_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_SEVEN_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_SEVEN_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_SEVEN_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_RISEONE_STATUS;
            data->cnt = 4;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_EIGHT_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_EIGHT_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_EIGHT_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_EIGHT_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_EIGHT_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_EIGHT_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_EIGHT_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_EIGHT_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_EIGHT_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_RISEONE_STATUS;
            data->cnt = 28;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_NINE_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_NINE_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_NINE_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_NINE_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_NINE_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_NINE_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_NINE_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_NINE_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_NINE_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_RISEONE_STATUS;
            data->cnt = 4;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }

        if(data->high_status == HIGH_TEN_RISEONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_RISETWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(0));
            pwm_start(data->beep_device);
            pwm_start(g_p);
        }
        else if(data->high_status == HIGH_TEN_RISETWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_RISETHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_TEN_RISETHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_KEEP_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_TEN_KEEP_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_FALLONE_STATUS;
            data->cnt = 14;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]));
        }
        else if(data->high_status == HIGH_TEN_FALLONE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_FALLTWO_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*5/7));
        }
        else if(data->high_status == HIGH_TEN_FALLTWO_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_FALLTHREE_STATUS;
            data->cnt = 1;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]*3/7));
        }
        else if(data->high_status == HIGH_TEN_FALLTHREE_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_TEN_END_STATUS;
            data->cnt = 1 ;
            pwm_set_duty_percent(data->beep_device,(unsigned long)(sound_duty[data->high_volume_control]/7));
        }
        else if(data->high_status == HIGH_TEN_END_STATUS && data->cnt <= 0)
        {
            data->once_alarm_switch = OFF;
            data->high_status = HIGH_END_STATUS;
            data->cnt = (data->timer_control.highlevel_time)*100;
            data->storecnt = data->cnt;
            pwm_stop(data->beep_device);
            pwm_stop(g_p);
        }
        else if(data->high_status == HIGH_END_STATUS && data->cnt <= 0)
        {
            data->high_status = HIGH_ONE_RISEONE_STATUS;
        }

        --data->cnt;
        if((data->high_status == HIGH_END_STATUS) && data->storecnt != (data->timer_control.highlevel_time)*100)
        {
                data->cnt = (int)((data->timer_control.highlevel_time)*100 -(data->storecnt - data->cnt));
                data->storecnt = (data->timer_control.highlevel_time)*100;
        }
        data->beep_timer.expires =  jiffies + 1;               //大概10ms进一次
        add_timer(&data->beep_timer);

     }
}

//3.23 通用声音接口

void common_sound_timer_fn(unsigned long arg)
{
            COMMON_SOUND_CONTROL * data = (COMMON_SOUND_CONTROL *)arg;
            
            if(data->common_sound_status == RISE_ONE_STATUS)
            {
                data->common_sound_cnt = 1;
                data->common_sound_switch = ON;
                pwm_set_duty_percent(data->common_sound_device,(unsigned long)(0));
                data->common_sound_status = RISE_TWO_STATUS;
                pwm_start(data->common_sound_device);
                pwm_start(g_p);
            }
            else if(data->common_sound_status == RISE_TWO_STATUS && data->common_sound_cnt <= 0)
            {
                data->common_sound_cnt = 1;
                pwm_set_duty_percent(data->common_sound_device,(unsigned long)(sound_duty[data->common_sound_level]*3/7));
                data->common_sound_status = RISE_THREE_STATUS;
            }
            else if(data->common_sound_status == RISE_THREE_STATUS && data->common_sound_cnt <= 0)
            {
                data->common_sound_cnt = 1;
                pwm_set_duty_percent(data->common_sound_device,(unsigned long)(sound_duty[data->common_sound_level]*5/7));
                data->common_sound_status = KEEP_STATUS;
            }
            else if(data->common_sound_status == KEEP_STATUS && data->common_sound_cnt <= 0)
            {
                data->common_sound_cnt = data->common_sound_length;    //时间长度                                  
                pwm_set_duty_percent(data->common_sound_device,(unsigned long)(sound_duty[data->common_sound_level]));
                data->common_sound_status = FALL_ONE_STATUS;
            }
            else if(data->common_sound_status == FALL_ONE_STATUS && data->common_sound_cnt <= 0)
            {
                data->common_sound_cnt = 1;
                pwm_set_duty_percent(data->common_sound_device,(unsigned long)(sound_duty[data->common_sound_level]*5/7));
                data->common_sound_status = FALL_TWO_STATUS;
            }
            else if(data->common_sound_status == FALL_TWO_STATUS && data->common_sound_cnt <= 0)
            {
                data->common_sound_cnt = 1;
                pwm_set_duty_percent(data->common_sound_device,(unsigned long)(sound_duty[data->common_sound_level]*3/7));
                data->common_sound_status = FALL_THREE_STATUS;
            }
            else if(data->common_sound_status == FALL_THREE_STATUS && data->common_sound_cnt <= 0)
            {
                data->common_sound_cnt = 1;
                pwm_set_duty_percent(data->common_sound_device,(unsigned long)(sound_duty[data->common_sound_level]*1/7));
                data->common_sound_status = END_STATUS;
            }
            else if(data->common_sound_status == END_STATUS && data->common_sound_cnt <= 0)
            {
                data->common_sound_status = RISE_ONE_STATUS;
                data->common_sound_switch = OFF;
                pwm_stop(g_p);
                pwm_stop(data->common_sound_device);
                del_timer_sync(&data->common_sound_timer);
            }

            if(data->common_sound_switch == ON)
            {
                --data->common_sound_cnt;
                data->common_sound_timer.expires =  jiffies + 1;     //10ms软中断一次
                add_timer(&data->common_sound_timer);
            }	
}

void alarm_remind_timer_fn(unsigned long arg)
{
	        ALARM_REMIND_CONTROL * data = (ALARM_REMIND_CONTROL *)arg;
          if(data->alarm_remind_status == ALARM_REMIND_START)
            {
                data->alarm_remind_cnt = 1;
                data->alarm_remind_switch = ON;
                pwm_set_duty_percent(data->alarm_remind_device,(unsigned long)(data->alarm_remind_duty));
                data->alarm_remind_status = ALARM_REMIND_DECREASE;
                pwm_start(data->alarm_remind_device);
                pwm_start(g_p);
            }
            else if(data->alarm_remind_status == ALARM_REMIND_DECREASE && data->alarm_remind_cnt <= 0)
            {
                data->alarm_remind_cnt = 20;
                --data->alarm_remind_duty;            //递减
                pwm_set_duty_percent(data->alarm_remind_device,(unsigned long)(data->alarm_remind_duty));
                data->alarm_remind_status = ALARM_REMIND_KEEP;
            }
            else if(data->alarm_remind_status == ALARM_REMIND_KEEP && data->alarm_remind_cnt <= 0)
            {
                data->alarm_remind_cnt = 10;
                pwm_set_duty_percent(data->alarm_remind_device,(unsigned long)(data->alarm_remind_duty));
                data->alarm_remind_status = ALARM_REMIND_END;
            }
            else if(data->alarm_remind_status == ALARM_REMIND_END && data->alarm_remind_cnt <= 0)
            {
                data->alarm_remind_status = ALARM_REMIND_START;
                data->alarm_remind_switch = OFF;
                pwm_stop(g_p);
                pwm_stop(data->alarm_remind_device);
                del_timer_sync(&data->alarm_remind_timer);
            }

            if(data->alarm_remind_switch == ON)
            {
                --data->alarm_remind_cnt;
                data->alarm_remind_timer.expires =  jiffies + 1;     //10ms软中断一次
                add_timer(&data->alarm_remind_timer);
            }	

}

static void beep_timer_init(void)
{
    beep_control.beep_timer.expires = jiffies + 1;       //1ms 进入定时器函数  减少不必要的延时
    beep_control.beep_timer.function = beep_timer_fn;
    beep_control.beep_timer.data = (unsigned long)&beep_control;
    pwm_change_freq((unsigned long)ALARM_FRED);

    add_timer(&beep_control.beep_timer);
}



static int pwm_match_name(struct device *dev, void *name)
{
    return !strcmp(name, dev_name(dev));
}

static struct pwm_device *__pwm_request(struct pwm_device *p, const char *label)
{
    int ret;

    ret = test_and_set_bit(FLAG_REQUESTED, &p->flags);
    if (ret) {
        p = ERR_PTR(-EBUSY);
        goto done;
    }

    p->label = label;
    p->pid = current->pid;

    if (p->ops->request) {
        ret = p->ops->request(p);
        if (ret) {
            p = ERR_PTR(ret);
            clear_bit(FLAG_REQUESTED, &p->flags);
            goto done;
        }
    }

done:
    return p;
}

static struct pwm_device *__pwm_request_byname(const char *name,
                           const char *label)
{
    struct device *d;
    struct pwm_device *p;

    d = class_find_device(&pwm_class, NULL, (char *)name, pwm_match_name);
    if (!d) {
        p = ERR_PTR(-EINVAL);
        goto done;
    }
    if (IS_ERR(d)) {
        p = (struct pwm_device *)d;
        goto done;
    }

    p = __pwm_request(dev_get_drvdata(d), label);

done:
    return p;
}

struct pwm_device *pwm_request_byname(const char *name, const char *label)
{
    struct pwm_device *p;

    mutex_lock(&device_list_mutex);
    p = __pwm_request_byname(name, label);
    mutex_unlock(&device_list_mutex);
    return p;
}
EXPORT_SYMBOL(pwm_request_byname);

struct pwm_device *pwm_request(const char *bus_id, int id, const char *label)
{
    char name[256];
    int ret;

    if (id == -1)
        ret = scnprintf(name, sizeof name, "%s", bus_id);
    else
        ret = scnprintf(name, sizeof name, "%s:%d", bus_id, id);
    if (ret <= 0 || ret >= sizeof name)
        return ERR_PTR(-EINVAL);

    return pwm_request_byname(name, label);
}
EXPORT_SYMBOL(pwm_request);

void pwm_release(struct pwm_device *p)
{
    mutex_lock(&device_list_mutex);

    if (!test_and_clear_bit(FLAG_REQUESTED, &p->flags)) {
        pr_debug("%s pwm device is not requested!\n",
                dev_name(p->dev));
        goto done;
    }

    pwm_stop(p);
    pwm_unsynchronize(p, NULL);
    pwm_set_handler(p, NULL, NULL);

    p->label = NULL;
    p->pid = -1;

    if (p->ops->release)
        p->ops->release(p);
done:
    mutex_unlock(&device_list_mutex);
}
EXPORT_SYMBOL(pwm_release);

unsigned long pwm_ns_to_ticks(struct pwm_device *p, unsigned long nsecs)
{
    unsigned long long ticks;

    ticks = nsecs;
    ticks *= p->tick_hz;
    do_div(ticks, 1000000000);
    return ticks;
}
EXPORT_SYMBOL(pwm_ns_to_ticks);

unsigned long pwm_ticks_to_ns(struct pwm_device *p, unsigned long ticks)
{
    unsigned long long ns;

    if (!p->tick_hz) {
        pr_debug("%s: frequency is zero\n", dev_name(p->dev));
        return 0;
    }

    ns = ticks;
    ns *= 1000000000UL;
    do_div(ns, p->tick_hz);
    return ns;
}
EXPORT_SYMBOL(pwm_ticks_to_ns);

static void pwm_config_ns_to_ticks(struct pwm_device *p, struct pwm_config *c)
{
    if (test_bit(PWM_CONFIG_PERIOD_NS, &c->config_mask)) {
        c->period_ticks = pwm_ns_to_ticks(p, c->period_ns);
        clear_bit(PWM_CONFIG_PERIOD_NS, &c->config_mask);
        set_bit(PWM_CONFIG_PERIOD_TICKS, &c->config_mask);
    }

    if (test_bit(PWM_CONFIG_DUTY_NS, &c->config_mask)) {
        c->duty_ticks = pwm_ns_to_ticks(p, c->duty_ns);
        clear_bit(PWM_CONFIG_DUTY_NS, &c->config_mask);
        set_bit(PWM_CONFIG_DUTY_TICKS, &c->config_mask);
    }
}

static void pwm_config_percent_to_ticks(struct pwm_device *p,
                    struct pwm_config *c)
{
    if (test_bit(PWM_CONFIG_DUTY_PERCENT, &c->config_mask)) {
        if (test_bit(PWM_CONFIG_PERIOD_TICKS, &c->config_mask))
            c->duty_ticks = c->period_ticks;
        else
            c->duty_ticks = p->period_ticks;

        c->duty_ticks *= c->duty_percent;
        c->duty_ticks /= 100;
        clear_bit(PWM_CONFIG_DUTY_PERCENT, &c->config_mask);
        set_bit(PWM_CONFIG_DUTY_TICKS, &c->config_mask);
    }
}

int pwm_config_nosleep(struct pwm_device *p, struct pwm_config *c)
{
    if (!p->ops->config_nosleep)
        return -EINVAL;

    pwm_config_ns_to_ticks(p, c);
    pwm_config_percent_to_ticks(p, c);

    return p->ops->config_nosleep(p, c);
}
EXPORT_SYMBOL(pwm_config_nosleep);

int pwm_config(struct pwm_device *p, struct pwm_config *c)
{
    int ret = 0;
		unsigned long flags;
		
    pwm_config_ns_to_ticks(p, c);
    pwm_config_percent_to_ticks(p, c);

    switch (c->config_mask & (BIT(PWM_CONFIG_PERIOD_TICKS)
                  | BIT(PWM_CONFIG_DUTY_TICKS))) {
    case BIT(PWM_CONFIG_PERIOD_TICKS):
        if (p->duty_ticks > c->period_ticks) {
            ret = -EINVAL;
            goto err;
        }
        break;
    case BIT(PWM_CONFIG_DUTY_TICKS):
        if (p->period_ticks < c->duty_ticks) {
            ret = -EINVAL;
            goto err;
        }
        break;
    case BIT(PWM_CONFIG_DUTY_TICKS) | BIT(PWM_CONFIG_PERIOD_TICKS):
        if (c->duty_ticks > c->period_ticks) {
            ret = -EINVAL;
            goto err;
        }
        break;
    default:
        break;
    }

err:
    dev_dbg(p->dev, "%s: config_mask %lu period_ticks %lu duty_ticks %lu"
        " polarity %d duty_ns %lu period_ns %lu duty_percent %d\n",
        __func__, c->config_mask, c->period_ticks, c->duty_ticks,
        c->polarity, c->duty_ns, c->period_ns, c->duty_percent);

    if (ret)
        return ret;
    spin_lock_irqsave(&p->pwm_lock, flags);
    ret = p->ops->config(p, c);
    spin_unlock_irqrestore(&p->pwm_lock, flags);
    return ret;
}
EXPORT_SYMBOL(pwm_config);

int pwm_set_period_ns(struct pwm_device *p, unsigned long period_ns)
{
	  unsigned long flags;
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_PERIOD_TICKS),
        .period_ticks = pwm_ns_to_ticks(p, period_ns),
    };

    spin_lock_irqsave(&p->pwm_lock, flags);
    p->period_ns = period_ns;
    spin_unlock_irqrestore(&p->pwm_lock, flags);
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_set_period_ns);

unsigned long pwm_get_period_ns(struct pwm_device *p)
{
    return pwm_ticks_to_ns(p, p->period_ticks);
}
EXPORT_SYMBOL(pwm_get_period_ns);

int pwm_set_frequency(struct pwm_device *p, unsigned long freq)
{
	  unsigned long flags;
    struct pwm_config c;

    if (!freq)
        return -EINVAL;

    c.config_mask = BIT(PWM_CONFIG_PERIOD_TICKS);
    c.period_ticks = pwm_ns_to_ticks(p, (NSEC_PER_SEC / freq));
    spin_lock_irqsave(&p->pwm_lock, flags);           // spin_lock(&p->pwm_lock);
    p->period_ns = NSEC_PER_SEC / freq;
    spin_unlock_irqrestore(&p->pwm_lock, flags);     // spin_unlock(&p->pwm_lock); 
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_set_frequency);

unsigned long pwm_get_frequency(struct pwm_device *p)
{
    unsigned long period_ns;

     period_ns = pwm_ticks_to_ns(p, p->period_ticks);

    if (!period_ns) {
        pr_debug("%s: frequency is zero\n", dev_name(p->dev));
        return 0;
    }

    return	NSEC_PER_SEC / period_ns;
}
EXPORT_SYMBOL(pwm_get_frequency);

int pwm_set_period_ticks(struct pwm_device *p,
                    unsigned long ticks)
{
  	unsigned long flags;
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_PERIOD_TICKS),
        .period_ticks = ticks,
    };

  	spin_lock_irqsave(&p->pwm_lock, flags);
    p->period_ns = pwm_ticks_to_ns(p, ticks);
   	spin_unlock_irqrestore(&p->pwm_lock, flags);
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_set_period_ticks);

int pwm_set_duty_ns(struct pwm_device *p, unsigned long duty_ns)
{
	  unsigned long flags;
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_DUTY_TICKS),
        .duty_ticks = pwm_ns_to_ticks(p, duty_ns),
    };
    spin_lock_irqsave(&p->pwm_lock, flags); 
    p->duty_ns = duty_ns;
    spin_unlock_irqrestore(&p->pwm_lock, flags);
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_set_duty_ns);

unsigned long pwm_get_duty_ns(struct pwm_device *p)
{
    return pwm_ticks_to_ns(p, p->duty_ticks);
}
EXPORT_SYMBOL(pwm_get_duty_ns);

int pwm_set_duty_percent(struct pwm_device *p, int percent)
{
	unsigned long flags;
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_DUTY_PERCENT),
        .duty_percent = percent,
    };

   spin_lock_irqsave(&p->pwm_lock, flags);
    p->duty_ns = p->period_ns * percent;
    p->duty_ns /= 100;
   spin_unlock_irqrestore(&p->pwm_lock, flags);
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_set_duty_percent);

unsigned long pwm_get_duty_percent(struct pwm_device *p)
{
    unsigned long long duty_percent;

    if (!p->period_ns) {
        pr_debug("%s: frequency is zero\n", dev_name(p->dev));
        return 0;
    }

    duty_percent = pwm_ticks_to_ns(p, p->duty_ticks);
    duty_percent *= 100;
    do_div(duty_percent, p->period_ns);
    return duty_percent;
}
EXPORT_SYMBOL(pwm_get_duty_percent);

int pwm_set_duty_ticks(struct pwm_device *p,
                    unsigned long ticks)
{
	unsigned long flags;
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_DUTY_TICKS),
        .duty_ticks = ticks,
    };

   	spin_lock_irqsave(&p->pwm_lock, flags);
    p->duty_ns = pwm_ticks_to_ns(p, ticks);
   spin_unlock_irqrestore(&p->pwm_lock, flags);
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_set_duty_ticks);

int pwm_set_polarity(struct pwm_device *p, int active_high)
{
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_POLARITY),
        .polarity = active_high,
    };
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_set_polarity);

int pwm_start(struct pwm_device *p)
{
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_START),
    };
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_start);

int pwm_stop(struct pwm_device *p)
{
    struct pwm_config c = {
        .config_mask = BIT(PWM_CONFIG_STOP),
    };
    return pwm_config(p, &c);
}
EXPORT_SYMBOL(pwm_stop);

int pwm_synchronize(struct pwm_device *p, struct pwm_device *to_p)
{
    if (!p->ops->synchronize)
        return -EINVAL;

    return p->ops->synchronize(p, to_p);
}
EXPORT_SYMBOL(pwm_synchronize);

int pwm_unsynchronize(struct pwm_device *p, struct pwm_device *from_p)
{
    if (!p->ops->unsynchronize)
        return -EINVAL;

    return p->ops->unsynchronize(p, from_p);
}
EXPORT_SYMBOL(pwm_unsynchronize);

static void pwm_handler(struct work_struct *w)
{
    struct pwm_device *p = container_of(w, struct pwm_device,
                        handler_work);
    if (p->handler && p->handler(p, p->handler_data))
        pwm_stop(p);
}

static void __pwm_callback(struct pwm_device *p)
{
    queue_work(pwm_handler_workqueue, &p->handler_work);
}

int pwm_set_handler(struct pwm_device *p, pwm_handler_t handler, void *data)
{
    if (p->ops->set_callback) {
        p->handler_data = data;
        p->handler = handler;
        INIT_WORK(&p->handler_work, pwm_handler);
        return p->ops->set_callback(p, handler ? __pwm_callback : NULL);
    }
    return -EINVAL;
}
EXPORT_SYMBOL(pwm_set_handler);

static ssize_t pwm_run_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", pwm_is_running(p));
}

static ssize_t pwm_run_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    int ret;

    if (sysfs_streq(buf, "1"))
        ret = pwm_start(p);
    else if (sysfs_streq(buf, "0"))
        ret = pwm_stop(p);
    else
        ret = -EINVAL;

    if (ret < 0)
        return ret;
    return len;
}
static DEVICE_ATTR(run, S_IRUGO | S_IWUSR, pwm_run_show, pwm_run_store);

static ssize_t pwm_tick_hz_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    return sprintf(buf, "%lu\n", p->tick_hz);
}
static DEVICE_ATTR(tick_hz, S_IRUGO, pwm_tick_hz_show, NULL);

/*  设置音量大小级别  */
static ssize_t pwm_set_sound_level_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
//	int ret = 0;       //这个初始化很重要
    unsigned long level;
    unsigned long sound_size;
    level = simple_strtoul(buf, NULL, 10);
    sound_size = level%100;
    level = level/100;

    beep_control.beep_device = dev_get_drvdata(dev);        //这个值得传进去

    switch(sound_size)
    {
    	case 0:
    		    if(level == 1)
            {
                beep_control.low_volume_control = LEVEL_NONE;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_NONE;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_NONE;
            }
            break;
    		
        case 1:
            if(level == 1)
            {
                beep_control.low_volume_control = LEVEL_ONE;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_ONE;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_ONE;
            }
            break;
        case 2:
            if(level == 1)
            {
                beep_control.low_volume_control = LEVEL_TWO;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_TWO;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_TWO;
            }
            break;
        case 3:
            if(level == 1)
            {
                beep_control.low_volume_control = LEVEL_THREE;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_THREE;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_THREE;
            }
            break;
        case 4:
              if(level == 1)
            {
                beep_control.low_volume_control = LEVEL_FOUR;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_FOUR;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_FOUR;
            }
            break;
        case 5:
            if(level == 1)
            {
                beep_control.low_volume_control = LEVEL_FIVE;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_FIVE;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_FIVE;
            }
            break;
        case 6:
              if(level == 1)
                {
                beep_control.low_volume_control = LEVEL_SIX;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_SIX;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_SIX;
            }
            break;
        case 7:
              if(level == 1)
                {
                beep_control.low_volume_control = LEVEL_SEVEN;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_SEVEN;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_SEVEN;
            }
            break;
        case 8:
              if(level == 1)
                {
                beep_control.low_volume_control = LEVEL_EIGHT;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_EIGHT;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_EIGHT;
            }
            break;
        case 9:
              if(level == 1)
                {
                beep_control.low_volume_control = LEVEL_NINE;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_NINE;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_NINE;
            }
            break;
        case 10:
              if(level == 1)
                {
                beep_control.low_volume_control = LEVEL_TEN;
            }
            else if(level == 3)
            {
                beep_control.mid_volume_control = LEVEL_TEN;
            }
            else if(level == 5)
            {
                beep_control.high_volume_control = LEVEL_TEN;
            }
            break;
        default:
            beep_control.low_volume_control = LEVEL_FOUR;
            beep_control.mid_volume_control = LEVEL_FOUR;
            beep_control.high_volume_control = LEVEL_FOUR;
            break;
    }

    return len;
}
static DEVICE_ATTR(set_sound_level,  S_IWUSR, NULL, pwm_set_sound_level_store);

/*  设置报警时间间隔  */
static ssize_t pwm_set_time_interval_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    int ret = 0;       //这个初始化很重要
    unsigned long time;
    size_t size = 1;
    time = simple_strtoul(buf+1, NULL, 10);
    beep_control.beep_device = dev_get_drvdata(dev);        //这个值得传进去

    if(time != 0)
    {
        if(!strncmp(buf, "1", size))
        {
            beep_control.timer_control.lowlevel_time = time;
        }
        else if(!strncmp(buf, "3", size))
        {
            beep_control.timer_control.midlevel_time = time;
        }
        else if(!strncmp(buf, "5", size))
        {
            beep_control.timer_control.highlevel_time = time;
        }
        else
        {
            ret = -EINVAL;
        }
    }
    else
    {
            ret = -EINVAL;
    }
    if(ret < 0)
        return ret;
    return len;
}
static DEVICE_ATTR(set_time_interval,  S_IWUSR, NULL, pwm_set_time_interval_store);

//12.25

static ssize_t pwm_change_freq(unsigned long freq)
{
    int ret = 0;
    if(NULL != g_p)
    {
            pwm_stop(g_p);
            pwm_set_duty_percent(g_p, 0);
            pwm_set_frequency(g_p, freq);
            pwm_set_duty_percent(g_p, 80);   //固定为80 温健决定的！！！
            pwm_start(g_p);

            return ret;
    }
    return  -EINVAL;
}

static void close_common_timer(void)
{
     if(common_sound_control.common_sound_switch == ON)
     {
         common_sound_control.common_sound_switch = OFF;
         pwm_stop(common_sound_control.common_sound_device);
         pwm_stop(g_p);
         del_timer_sync(&common_sound_control.common_sound_timer);
     }
}

/* 设置报警级别  */
static ssize_t pwm_set_level_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    int ret = 0;       //这个初始化很重要
    
     if(beep_control.beep_device == NULL)
     {
    	    beep_control.beep_device = dev_get_drvdata(dev);        //这个值得传进去
     }
    
//		pwm_change_freq((unsigned long)ALARM_FRED);
    if(sysfs_streq(buf, "1"))    //低级报警
    {
        beep_control.master_alarm_switch = ON;
        if(beep_control.alarm_level != ALARM_LOW && beep_control.once_alarm_switch == OFF)             //级别改变
        {
        	  close_common_timer();
            del_timer_sync(&beep_control.beep_timer);
            beep_control.high_status = HIGH_NONE_STATUS;
            beep_control.mid_status = MID_NONE_STATUS;
            beep_control.alarm_level = ALARM_LOW;
            pwm_stop(beep_control.beep_device);
            pwm_stop(g_p);
            beep_timer_init();
        }
    }
    else if (sysfs_streq(buf, "3"))  //中级报警
    {
        beep_control.master_alarm_switch = ON;
        if(beep_control.alarm_level != ALARM_MID && beep_control.once_alarm_switch == OFF)
        {
        	  close_common_timer();
            del_timer_sync(&beep_control.beep_timer);
            beep_control.high_status = HIGH_NONE_STATUS;
            beep_control.low_status = LOW_NONE_STATUS;
            beep_control.alarm_level = ALARM_MID;
            pwm_stop(beep_control.beep_device);
            pwm_stop(g_p);
            beep_timer_init();
        }

    }
    else if (sysfs_streq(buf, "5")) //高级报警
    {
        beep_control.master_alarm_switch = ON;
        if(beep_control.alarm_level != ALARM_HIGH && beep_control.once_alarm_switch == OFF)
        {
        	  close_common_timer();
            del_timer_sync(&beep_control.beep_timer);
            beep_control.low_status = LOW_NONE_STATUS;
            beep_control.mid_status = MID_NONE_STATUS;
            beep_control.alarm_level = ALARM_HIGH;
            pwm_stop(beep_control.beep_device);
            pwm_stop(g_p);
            beep_timer_init();
        }
    }
    else if (sysfs_streq(buf, "0")) //关闭报警
    {
        if(beep_control.alarm_level != ALARM_NONE && beep_control.once_alarm_switch == OFF)   //脉搏音丢失问题
        {
	        del_timer_sync(&beep_control.beep_timer);
	        beep_control.low_status = LOW_NONE_STATUS;
	        beep_control.mid_status = MID_NONE_STATUS;
	        beep_control.high_status = HIGH_NONE_STATUS;
	        beep_control.alarm_level = ALARM_NONE;
//	        beep_control.once_alarm_switch = OFF;
	        beep_control.master_alarm_switch = OFF;
	        if(common_sound_control.common_sound_switch == OFF) //脉搏音丢失原因就在这里
	        {
		        pwm_stop(beep_control.beep_device);
		        pwm_stop(g_p);		        	
	        }
        }

    }
    else
    {
        ret = -EINVAL;
    }

    if (ret < 0)
        return ret;
    return len;

}
static DEVICE_ATTR(set_level,  S_IWUSR, NULL, pwm_set_level_store);


//3.23报警提示音
static ssize_t alarm_remind_sound_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
	  int ret = 0;                        //这个初始化很重要
    unsigned long freq;
    unsigned char sound_level;

    freq = simple_strtoul(buf, NULL, 10);
    sound_level = freq/100000;
    freq = freq%100000;
    
    if(alarm_remind_control.alarm_remind_device == NULL)
    {
    	 alarm_remind_control.alarm_remind_device = dev_get_drvdata(dev);         //这个值得传进去
    }
    
     if(beep_control.once_alarm_switch == OFF && alarm_remind_control.alarm_remind_switch == OFF)
     	{
     		  switch(sound_level)
          {
            case 1:
                alarm_remind_control.alarm_remind_level = LEVEL_ONE;
                break;
            case 2:
                alarm_remind_control.alarm_remind_level = LEVEL_TWO;
                break;
            case 3:
                alarm_remind_control.alarm_remind_level = LEVEL_THREE;
                break;
            case 4:
                alarm_remind_control.alarm_remind_level = LEVEL_FOUR;
                break;
            case 5:
                alarm_remind_control.alarm_remind_level = LEVEL_FIVE;
                break;
            case 6:
                alarm_remind_control.alarm_remind_level = LEVEL_SIX;
                break;
            case 7:
                alarm_remind_control.alarm_remind_level = LEVEL_SEVEN;
                break;
            case 8:
                alarm_remind_control.alarm_remind_level = LEVEL_EIGHT;
                break;
            case 9:
                alarm_remind_control.alarm_remind_level = LEVEL_NINE;
                break;
            case 10:
                alarm_remind_control.alarm_remind_level = LEVEL_TEN;
                break;
            default:
                alarm_remind_control.alarm_remind_level = LEVEL_NONE;
                break;
          }
        alarm_remind_control.alarm_remind_duty = sound_duty[alarm_remind_control.alarm_remind_level];
        del_timer_sync(&alarm_remind_control.alarm_remind_timer);
        alarm_remind_control.alarm_remind_status = ALARM_REMIND_START;
        pwm_change_freq(freq);                                           /*改变音调*/
        pwm_stop(alarm_remind_control.alarm_remind_device);
        pwm_stop(g_p);
        
        alarm_remind_control.alarm_remind_timer.expires = jiffies + 1;
        alarm_remind_control.alarm_remind_timer.function = alarm_remind_timer_fn;
        alarm_remind_control.alarm_remind_timer.data = (unsigned long)&alarm_remind_control;

        add_timer(&alarm_remind_control.alarm_remind_timer);
     		
     	}
			else
			{
				ret = 1;
				return ret;	
			}
			
			return len;
}
static DEVICE_ATTR(alarm_remind_sound,  S_IWUSR, NULL, alarm_remind_sound_store);

//3.23 通用音接口
static ssize_t common_sound_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{	
	  int ret = 0;                        //这个初始化很重要
	  char * endpoint = NULL;
    unsigned long  sound_freq;
    unsigned long  sound_level;
    unsigned long  sound_length;
    
    sound_level = simple_strtoul(buf, &endpoint, 10);
    endpoint++;
    sound_length = simple_strtoul(endpoint, &endpoint, 10);
    endpoint++;
    sound_freq = simple_strtoul(endpoint, &endpoint, 10);
    
    if(common_sound_control.common_sound_device == NULL)         //防止和中断里面产生竞态20150326(后续其他的device也要这样做)
    {
    	    common_sound_control.common_sound_device = dev_get_drvdata(dev);         //这个值得传进去
    }

    if((beep_control.master_alarm_switch == OFF ||(beep_control.once_alarm_switch == OFF && beep_control.cnt >= 15))
    	&& alarm_remind_control.alarm_remind_switch == OFF
    	 && common_sound_control.common_sound_switch == OFF)
    {    
    	  switch(sound_level)
        {
            case 1:
                common_sound_control.common_sound_level = LEVEL_ONE;
                break;
            case 2:
                common_sound_control.common_sound_level = LEVEL_TWO;
                break;
            case 3:
                common_sound_control.common_sound_level = LEVEL_THREE;
                break;
            case 4:
                common_sound_control.common_sound_level = LEVEL_FOUR;
                break;
            case 5:
                common_sound_control.common_sound_level = LEVEL_FIVE;
                break;
            case 6:
                common_sound_control.common_sound_level = LEVEL_SIX;
                break;
            case 7:
                common_sound_control.common_sound_level = LEVEL_SEVEN;
                break;
            case 8:
                common_sound_control.common_sound_level = LEVEL_EIGHT;
                break;
            case 9:
                common_sound_control.common_sound_level = LEVEL_NINE;
                break;
            case 10:
                common_sound_control.common_sound_level = LEVEL_TEN;
                break;
            default:
                common_sound_control.common_sound_level = LEVEL_NONE;
                break;
        } 
        
        common_sound_control.common_sound_length = sound_length/10;              //传进来的是ms转化成10ms               
        common_sound_control.common_sound_status = RISE_ONE_STATUS;
        
        del_timer_sync(&common_sound_control.common_sound_timer);  
 
        pwm_change_freq(sound_freq);                                           /*改变音调*/
        pwm_stop(common_sound_control.common_sound_device);
        pwm_stop(g_p);
        
        common_sound_control.common_sound_timer.expires = jiffies + 1;
        common_sound_control.common_sound_timer.function = common_sound_timer_fn;
        common_sound_control.common_sound_timer.data = (unsigned long)&common_sound_control;

        add_timer(&common_sound_control.common_sound_timer);
    }
    else
    {
        ret = 1;
        return ret;
    }      
    return len;
}

static DEVICE_ATTR(common_sound,  S_IWUSR, NULL, common_sound_store);

static ssize_t pwm_control_sound_mute_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    int ret = 0;         //这个初始化很重要

    if(sysfs_streq(buf, "0"))
    {
        gpio_direction_output(GPIO_TO_PIN(3, 16), 0);     //置低 暂停
    }
        else if(sysfs_streq(buf, "1"))
        {
                gpio_direction_output(GPIO_TO_PIN(3, 16), 1);     //置高  开启
        }
        else
        {
                ret = -EINVAL;
        }
    if(ret < 0)
        return ret;
    return len;
}

static DEVICE_ATTR(control_sound_mute,  S_IWUSR, NULL, pwm_control_sound_mute_store);

static ssize_t pwm_duty_ns_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    return sprintf(buf, "%lu\n", pwm_get_duty_ns(p));
}

static ssize_t pwm_duty_ns_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    unsigned long duty_ns;
    struct pwm_device *p = dev_get_drvdata(dev);
    int ret;

    if (!kstrtoul(buf, 10, &duty_ns)) {
        ret = pwm_set_duty_ns(p, duty_ns);

        if (ret < 0)
            return ret;
    }
    
    return len;
}
static DEVICE_ATTR(duty_ns, S_IRUGO | S_IWUSR, pwm_duty_ns_show,
           pwm_duty_ns_store);

static ssize_t pwm_duty_percent_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    return sprintf(buf, "%lu\n", pwm_get_duty_percent(p));
}

static ssize_t pwm_duty_percent_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf,
                 size_t len)
{
    unsigned long duty_ns;
    struct pwm_device *p = dev_get_drvdata(dev);
    int ret;

    if (!kstrtoul(buf, 10, &duty_ns)) {
        ret = pwm_set_duty_percent(p, duty_ns);

        if (ret < 0)
            return ret;
    }

    return len;
}

static DEVICE_ATTR(duty_percent, S_IRUGO | S_IWUSR, pwm_duty_percent_show,
           pwm_duty_percent_store);

static ssize_t pwm_period_ns_show(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    return sprintf(buf, "%lu\n", pwm_get_period_ns(p));
}

static ssize_t pwm_period_ns_store(struct device *dev,
                   struct device_attribute *attr,
                   const char *buf, size_t len)
{
    unsigned long period_ns;
    struct pwm_device *p = dev_get_drvdata(dev);
    int ret;

    if (!kstrtoul(buf, 10, &period_ns)) {
        ret = pwm_set_period_ns(p, period_ns);

        if (ret < 0)
            return ret;
    }

    return len;
}
static DEVICE_ATTR(period_ns, S_IRUGO | S_IWUSR, pwm_period_ns_show,
           pwm_period_ns_store);

static ssize_t pwm_period_freq_show(struct device *dev,
                  struct device_attribute *attr,
                  char *buf)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    return sprintf(buf, "%lu\n", pwm_get_frequency(p));
}

static ssize_t pwm_period_freq_store(struct device *dev,
                   struct device_attribute *attr,
                   const char *buf,
                   size_t len)
{
    unsigned long freq_hz;
    int ret;

    struct pwm_device *p = dev_get_drvdata(dev);
    if (!kstrtoul(buf, 10, &freq_hz)) {
        ret = pwm_set_frequency(p, freq_hz);

        if (ret < 0)
            return ret;
    }
    return len;
}

static DEVICE_ATTR(period_freq, S_IRUGO | S_IWUSR, pwm_period_freq_show,
           pwm_period_freq_store);

static ssize_t pwm_polarity_show(struct device *dev,
                 struct device_attribute *attr,
                 char *buf)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", p->active_high ? 1 : 0);
}

static ssize_t pwm_polarity_store(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t len)
{
    unsigned long polarity;
    struct pwm_device *p = dev_get_drvdata(dev);
    int ret;

    if (!kstrtoul(buf, 10, &polarity)) {
        ret = pwm_set_polarity(p, polarity);

        if (ret < 0)
            return ret;
    }

    return len;
}
static DEVICE_ATTR(polarity, S_IRUGO | S_IWUSR, pwm_polarity_show,
           pwm_polarity_store);

static ssize_t pwm_request_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{

    struct pwm_device *p = dev_get_drvdata(dev);
    int ret;

    ret = test_bit(FLAG_REQUESTED, &p->flags);

    if (ret)
        return sprintf(buf, "%s requested by %s\n",
                dev_name(p->dev), p->label);
    else
        return sprintf(buf, "%s is free\n",
                dev_name(p->dev));
}

static ssize_t pwm_request_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    struct pwm_device *p = dev_get_drvdata(dev);
    unsigned long request;
    struct pwm_device *ret;

    //TEST
    if(strncmp(dev_name(p->dev), "ecap.2", 6) == 0)
    {
        if(NULL == g_p)
        {
            g_p = dev_get_drvdata(dev);
        }
    }

    if (!kstrtoul(buf, 10, &request)) {
        if (request) {
            mutex_lock(&device_list_mutex);
            ret = __pwm_request(p, REQUEST_SYSFS);
            mutex_unlock(&device_list_mutex);

            if (IS_ERR(ret))
                return PTR_ERR(ret);
        } else
            pwm_release(p);
    }
    return len;
}
static DEVICE_ATTR(request, S_IRUGO | S_IWUSR, pwm_request_show,
           pwm_request_store);

static const struct attribute *pwm_attrs[] = {
    &dev_attr_tick_hz.attr,
    &dev_attr_run.attr,
    &dev_attr_polarity.attr,
    &dev_attr_duty_ns.attr,
    &dev_attr_period_ns.attr,
    &dev_attr_request.attr,
    &dev_attr_duty_percent.attr,
    &dev_attr_set_level.attr,
    &dev_attr_common_sound.attr,
    &dev_attr_alarm_remind_sound.attr,
    &dev_attr_set_time_interval.attr,
    &dev_attr_set_sound_level.attr,
    &dev_attr_control_sound_mute.attr,
    &dev_attr_period_freq.attr,
    NULL,
};


static const struct attribute_group pwm_device_attr_group = {
    .attrs = (struct attribute **) pwm_attrs,
};

static struct class_attribute pwm_class_attrs[] = {
    __ATTR_NULL,
};

static struct class pwm_class = {
    .name = "pwm",
    .owner = THIS_MODULE,

    .class_attrs = pwm_class_attrs,
};

static int pwm_freq_transition_notifier_cb(struct notifier_block *nb,
        unsigned long val, void *data)
{
    struct pwm_device *p;

    p = container_of(nb, struct pwm_device, freq_transition);

    if (val == CPUFREQ_POSTCHANGE && pwm_is_requested(p))
        p->ops->freq_transition_notifier_cb(p);

    return 0;
}

static inline int pwm_cpufreq_notifier_register(struct pwm_device *p)
{
    p->freq_transition.notifier_call = pwm_freq_transition_notifier_cb;

    return cpufreq_register_notifier(&p->freq_transition,
               CPUFREQ_TRANSITION_NOTIFIER);
}

int pwm_register_byname(struct pwm_device *p, struct device *parent,
            const char *name)
{
    struct device *d;
    int ret;

    if (!p->ops || !p->ops->config)
        return -EINVAL;

    mutex_lock(&device_list_mutex);

    d = class_find_device(&pwm_class, NULL, (char *)name, pwm_match_name);
    if (d) {
        ret = -EEXIST;
        goto err_found_device;
    }

    p->dev = device_create(&pwm_class, parent, MKDEV(0, 0), NULL, name);
    if (IS_ERR(p->dev)) {
        ret = PTR_ERR(p->dev);
        goto err_device_create;
    }

    ret = sysfs_create_group(&p->dev->kobj, &pwm_device_attr_group);
    if (ret)
        goto err_create_group;

    dev_set_drvdata(p->dev, p);
    p->flags = BIT(FLAG_REGISTERED);

    ret = pwm_cpufreq_notifier_register(p);

    if (ret < 0)
        printk(KERN_ERR "Failed to add cpufreq notifier\n");

    spin_lock_init(&p->pwm_lock);
    goto done;

err_create_group:
    device_unregister(p->dev);
    p->flags = 0;

err_device_create:
err_found_device:
done:
    mutex_unlock(&device_list_mutex);

    return ret;
}
EXPORT_SYMBOL(pwm_register_byname);

int pwm_register(struct pwm_device *p, struct device *parent, int id)
{
    int ret;
    char name[256];

    if (IS_ERR_OR_NULL(parent))
        return -EINVAL;

    if (id == -1)
        ret = scnprintf(name, sizeof name, "%s", dev_name(parent));
    else
        ret = scnprintf(name, sizeof name, "%s:%d",
                   dev_name(parent), id);
    if (ret <= 0 || ret >= sizeof name)
        return -EINVAL;

    return pwm_register_byname(p, parent, name);
}
EXPORT_SYMBOL(pwm_register);

int pwm_unregister(struct pwm_device *p)
{
    int ret = 0;

    mutex_lock(&device_list_mutex);

    if (pwm_is_running(p) || pwm_is_requested(p)) {
        ret = -EBUSY;
        goto done;
    }

    sysfs_remove_group(&p->dev->kobj, &pwm_device_attr_group);
    device_unregister(p->dev);
    p->flags = 0;

done:
    mutex_unlock(&device_list_mutex);

    return ret;
}
EXPORT_SYMBOL(pwm_unregister);

static int __init pwm_init(void)
{
    /* beep_control 初始化 */
    beep_control.alarm_level= ALARM_NONE;
    beep_control.low_status = LOW_NONE_STATUS;
    beep_control.mid_status = MID_NONE_STATUS;
    beep_control.high_status = HIGH_NONE_STATUS;
    beep_control.low_volume_control = LEVEL_FIVE;    //默认五档
    beep_control.mid_volume_control = LEVEL_FIVE;    //默认五档
    beep_control.high_volume_control = LEVEL_FIVE;   //默认五档
    beep_control.timer_control. lowlevel_time = 20;
    beep_control.timer_control. midlevel_time = 20;
    beep_control.timer_control. highlevel_time = 10;
    beep_control.cnt = 0;
    beep_control.storecnt = 0;
    beep_control.once_alarm_switch = OFF;
    beep_control.master_alarm_switch = OFF;
    beep_control.beep_device = NULL;
  
    common_sound_control.common_sound_cnt = 0;
    common_sound_control.common_sound_switch = OFF;
    common_sound_control.common_sound_device = NULL;           //后面其他的device也要加
    
    alarm_remind_control.alarm_remind_cnt = 0;
    alarm_remind_control.alarm_remind_switch = OFF;
    alarm_remind_control.alarm_remind_device = NULL;

    init_timer(&beep_control.beep_timer);
    init_timer(&common_sound_control.common_sound_timer);
    init_timer(&alarm_remind_control.alarm_remind_timer);

    return class_register(&pwm_class);
}

static void __exit pwm_exit(void)
{
    class_unregister(&pwm_class);
}

#ifdef MODULE
module_init(pwm_init);
module_exit(pwm_exit);
MODULE_LICENSE("GPL");
#else
postcore_initcall(pwm_init);
#endif

