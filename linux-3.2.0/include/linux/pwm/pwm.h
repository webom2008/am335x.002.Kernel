/*
 * Copyright (C) 2011 Bill Gatliff < bgat@billgatliff.com>
 * Copyright (C) 2011 Arun Murthy <arun.murth@stericsson.com>
 *
 * This program is free software; you may redistribute and/or modify
 * it under the terms of the GNU General Public License version 2, as
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
#ifndef __LINUX_PWM_H
#define __LINUX_PWM_H

enum {
	FLAG_REGISTERED		= 0,
	FLAG_REQUESTED		= 1,
	FLAG_STOP		= 2,
	FLAG_RUNNING		= 3,
};

enum {
	PWM_CONFIG_DUTY_TICKS	= 0,
	PWM_CONFIG_PERIOD_TICKS	= 1,
	PWM_CONFIG_POLARITY	= 2,
	PWM_CONFIG_START	= 3,
	PWM_CONFIG_STOP		= 4,

	PWM_CONFIG_HANDLER	= 5,

	PWM_CONFIG_DUTY_NS	= 6,
	PWM_CONFIG_DUTY_PERCENT	= 7,
	PWM_CONFIG_PERIOD_NS	= 8,
};
/* 我添加的数据 */
// 不同报警级别的控制
typedef enum _HIGH_STATUS          //控制高级报警的不同状态
{
	HIGH_NONE_STATUS = 0,
	HIGH_ONE_RISEONE_STATUS = 0,
	HIGH_ONE_RISETWO_STATUS,
	HIGH_ONE_RISETHREE_STATUS,
	HIGH_ONE_KEEP_STATUS,
	HIGH_ONE_FALLONE_STATUS,
	HIGH_ONE_FALLTWO_STATUS,
	HIGH_ONE_FALLTHREE_STATUS,
	HIGH_ONE_END_STATUS,
	
	HIGH_TWO_RISEONE_STATUS,
	HIGH_TWO_RISETWO_STATUS,
	HIGH_TWO_RISETHREE_STATUS,
	HIGH_TWO_KEEP_STATUS,
	HIGH_TWO_FALLONE_STATUS,
	HIGH_TWO_FALLTWO_STATUS,
	HIGH_TWO_FALLTHREE_STATUS,
	HIGH_TWO_END_STATUS,
	
	HIGH_THREE_RISEONE_STATUS,
	HIGH_THREE_RISETWO_STATUS,
	HIGH_THREE_RISETHREE_STATUS,
	HIGH_THREE_KEEP_STATUS,
	HIGH_THREE_FALLONE_STATUS,
	HIGH_THREE_FALLTWO_STATUS,
	HIGH_THREE_FALLTHREE_STATUS,
	HIGH_THREE_END_STATUS,
	
	HIGH_FOUR_RISEONE_STATUS,
	HIGH_FOUR_RISETWO_STATUS,
	HIGH_FOUR_RISETHREE_STATUS,
	HIGH_FOUR_KEEP_STATUS,
	HIGH_FOUR_FALLONE_STATUS,
	HIGH_FOUR_FALLTWO_STATUS,
	HIGH_FOUR_FALLTHREE_STATUS,
	HIGH_FOUR_END_STATUS, 
	
	HIGH_FIVE_RISEONE_STATUS,
	HIGH_FIVE_RISETWO_STATUS,
	HIGH_FIVE_RISETHREE_STATUS,
	HIGH_FIVE_KEEP_STATUS,
	HIGH_FIVE_FALLONE_STATUS,
	HIGH_FIVE_FALLTWO_STATUS,
	HIGH_FIVE_FALLTHREE_STATUS,
	HIGH_FIVE_END_STATUS, 
	
	HIGH_SIX_RISEONE_STATUS,
	HIGH_SIX_RISETWO_STATUS,
	HIGH_SIX_RISETHREE_STATUS,
	HIGH_SIX_KEEP_STATUS,
	HIGH_SIX_FALLONE_STATUS,
	HIGH_SIX_FALLTWO_STATUS,
	HIGH_SIX_FALLTHREE_STATUS,
	HIGH_SIX_END_STATUS, 
	
	HIGH_SEVEN_RISEONE_STATUS,
	HIGH_SEVEN_RISETWO_STATUS,
	HIGH_SEVEN_RISETHREE_STATUS,
	HIGH_SEVEN_KEEP_STATUS,
	HIGH_SEVEN_FALLONE_STATUS,
	HIGH_SEVEN_FALLTWO_STATUS,
	HIGH_SEVEN_FALLTHREE_STATUS,
	HIGH_SEVEN_END_STATUS, 
	
	HIGH_EIGHT_RISEONE_STATUS,
	HIGH_EIGHT_RISETWO_STATUS,
	HIGH_EIGHT_RISETHREE_STATUS,
	HIGH_EIGHT_KEEP_STATUS,
	HIGH_EIGHT_FALLONE_STATUS,
	HIGH_EIGHT_FALLTWO_STATUS,
	HIGH_EIGHT_FALLTHREE_STATUS,
	HIGH_EIGHT_END_STATUS, 
	
	HIGH_NINE_RISEONE_STATUS,
	HIGH_NINE_RISETWO_STATUS,
	HIGH_NINE_RISETHREE_STATUS,
	HIGH_NINE_KEEP_STATUS,
	HIGH_NINE_FALLONE_STATUS,
	HIGH_NINE_FALLTWO_STATUS,
	HIGH_NINE_FALLTHREE_STATUS,
	HIGH_NINE_END_STATUS, 
	
	HIGH_TEN_RISEONE_STATUS,
	HIGH_TEN_RISETWO_STATUS,
	HIGH_TEN_RISETHREE_STATUS,
	HIGH_TEN_KEEP_STATUS,
	HIGH_TEN_FALLONE_STATUS,
	HIGH_TEN_FALLTWO_STATUS,
	HIGH_TEN_FALLTHREE_STATUS,
	HIGH_TEN_END_STATUS,
	
	HIGH_END_STATUS,
	
}HIGH_STATUS;

typedef enum _MID_STATUS          //控制中级报警状态
{
  MID_NONE_STATUS = 0,
	MID_ONE_RISEONE_STATUS = 0,
	MID_ONE_RISETWO_STATUS,
	MID_ONE_RISETHREE_STATUS,
	MID_ONE_KEEP_STATUS,
	MID_ONE_FALLONE_STATUS,
	MID_ONE_FALLTWO_STATUS,
	MID_ONE_FALLTHREE_STATUS,
	MID_ONE_END_STATUS,
	
	MID_TWO_RISEONE_STATUS,
	MID_TWO_RISETWO_STATUS,
	MID_TWO_RISETHREE_STATUS,
	MID_TWO_KEEP_STATUS,
	MID_TWO_FALLONE_STATUS,
	MID_TWO_FALLTWO_STATUS,
	MID_TWO_FALLTHREE_STATUS,
	MID_TWO_END_STATUS,
	
	MID_THREE_RISEONE_STATUS,
	MID_THREE_RISETWO_STATUS,
	MID_THREE_RISETHREE_STATUS,
	MID_THREE_KEEP_STATUS,
	MID_THREE_FALLONE_STATUS,
	MID_THREE_FALLTWO_STATUS,
	MID_THREE_FALLTHREE_STATUS,
	MID_THREE_END_STATUS,
	
	MID_END_STATUS

}MID_STATUS;

typedef enum _SOUND_STATUS
{
	RISE_ONE_STATUS = 1,
	RISE_TWO_STATUS = 2,
	RISE_THREE_STATUS = 3,
	KEEP_STATUS = 4,
	FALL_ONE_STATUS = 5,
	FALL_TWO_STATUS = 6,
	FALL_THREE_STATUS = 7,
	END_STATUS = 8,
		
}SOUND_STATUS;
 
typedef enum           //控制声音大小
{
	LEVEL_NONE = 0,
	LEVEL_ONE,  
	LEVEL_TWO , 
	LEVEL_THREE,
	LEVEL_FOUR ,
	LEVEL_FIVE  ,
	LEVEL_SIX ,
	LEVEL_SEVEN, 
	LEVEL_EIGHT ,
	LEVEL_NINE ,
	LEVEL_TEN  ,
	
} SOUND_LEVEL;

typedef enum  _LOW_STATUS      //控制低级报警状态                       
{
	LOW_NONE_STATUS = 0,
	
	LOW_RISEONE_STATUS = 0,
	LOW_RISETWO_STATUS,
	LOW_RISETHREE_STATUS,
	LOW_KEEP_STATUS,
	LOW_FALLONE_STATUS ,
	LOW_FALLTWO_STATUS,
	LOW_FALLTHREE_STATUS,
	LOW_FINISH_STATUS,
	
	LOW_END_STATUS,
		
} LOW_STATUS;

typedef enum                    
{
	ALARM_NONE = 0,
	ALARM_LOW,
	ALARM_MID,
	ALARM_HIGH,
	
}ALARM_LEVEL;

typedef enum
{
	ALARM_REMIND_NONE,
	ALARM_REMIND_START,
	ALARM_REMIND_DECREASE,
	ALARM_REMIND_KEEP,
	ALARM_REMIND_END,
		
}ALARM_REMIND_STATUS;


typedef struct
{
	unsigned long lowlevel_time;
	unsigned long midlevel_time;
	unsigned long highlevel_time;

}TIMER_CONTROL;

typedef struct _BEEP_CONTROL          /*报警音控制 */
{
	HIGH_STATUS  high_status;
	MID_STATUS   mid_status;
	LOW_STATUS   low_status;
	TIMER_CONTROL timer_control;  //设置不同报警级别的间隔
	ALARM_LEVEL  alarm_level;
	SOUND_LEVEL  low_volume_control;
	SOUND_LEVEL  mid_volume_control;
	SOUND_LEVEL  high_volume_control;
	struct pwm_device * beep_device;
	struct timer_list beep_timer;     
	int      cnt;           //计数
	int     storecnt;
	unsigned char once_alarm_switch;      /*一次报警的开关 */
	unsigned char master_alarm_switch;    /*报警的开到关闭*/
	
}BEEP_CONTROL;


typedef struct _COMMON_SOUND_CONTROL         /*通用接口配置*/
{	
	SOUND_STATUS       common_sound_status;
	SOUND_LEVEL        common_sound_level;
	struct pwm_device *common_sound_device;
	struct timer_list  common_sound_timer;
	unsigned char      common_sound_switch;
	int                common_sound_length;      
	int                common_sound_cnt;
	
}COMMON_SOUND_CONTROL;

typedef struct _ALARM_REMIND_CONTROL         /*报警提示音*/
{	
	ALARM_REMIND_STATUS       alarm_remind_status;
	SOUND_LEVEL        				alarm_remind_level;
	struct pwm_device        *alarm_remind_device ;
	struct timer_list         alarm_remind_timer;
	unsigned char             alarm_remind_switch; 
	int                       alarm_remind_duty;    
	int                       alarm_remind_cnt;
	
}ALARM_REMIND_CONTROL;

/*   我的数据完毕  */

struct pwm_config;
struct pwm_device;

typedef int (*pwm_handler_t)(struct pwm_device *p, void *data);
typedef void (*pwm_callback_t)(struct pwm_device *p);

struct pwm_device_ops {
	int	(*request)		(struct pwm_device *p);
	void	(*release)		(struct pwm_device *p);
	int	(*config)		(struct pwm_device *p,
					 struct pwm_config *c);
	int	(*config_nosleep)	(struct pwm_device *p,
					 struct pwm_config *c);
	int	(*synchronize)		(struct pwm_device *p,
					 struct pwm_device *to_p);
	int	(*unsynchronize)	(struct pwm_device *p,
					 struct pwm_device *from_p);
	int	(*set_callback)		(struct pwm_device *p,
					 pwm_callback_t callback);
	int	(*freq_transition_notifier_cb) (struct pwm_device *p);
};

struct pwm_config {
	unsigned long config_mask;
	unsigned long duty_ticks;
	unsigned long period_ticks;
	int polarity;

	pwm_handler_t handler;

	unsigned long duty_ns;
	unsigned long period_ns;
	int duty_percent;
};

struct pwm_device {
	struct list_head list;

	struct device *dev;
	struct pwm_device_ops *ops;

	void *data;

	const char *label;
	pid_t pid;

	volatile unsigned long flags;

	unsigned long tick_hz;

	pwm_callback_t callback;

	struct work_struct handler_work;
	pwm_handler_t handler;
	void *handler_data;

	int active_high;
	unsigned long period_ticks;
	unsigned long duty_ticks;
	unsigned long period_ns;
	unsigned long duty_ns;
	struct notifier_block freq_transition;
	unsigned long max_period_ticks;
	spinlock_t pwm_lock;
};

#include <linux/semaphore.h>
#include <linux/pwm/ehrpwm.h>

enum {
	PWM_VERSION_0,
	PWM_VERSION_1,
};

struct pwm_chan_attrib {
	int max_freq;
	bool inverse_pol;
};

#define PWM_CHANNEL NCHAN

struct pwmss_platform_data {
	int channel_mask;
	u8 version;
	struct pwm_chan_attrib chan_attrib[PWM_CHANNEL];
};

struct pwm_device *pwm_request_byname(const char *name, const char *label);
struct pwm_device *pwm_request(const char *bus_id, int id, const char *label);
void pwm_release(struct pwm_device *p);

static inline int pwm_is_registered(struct pwm_device *p)
{
	return test_bit(FLAG_REGISTERED, &p->flags);
}

static inline int pwm_is_requested(struct pwm_device *p)
{
	return test_bit(FLAG_REQUESTED, &p->flags);
}

static inline int pwm_is_running(struct pwm_device *p)
{
	return test_bit(FLAG_RUNNING, &p->flags);
}

static inline void pwm_set_drvdata(struct pwm_device *p, void *data)
{
	p->data = data;
}

static inline void *pwm_get_drvdata(const struct pwm_device *p)
{
	return p->data;
}

unsigned long pwm_ns_to_ticks(struct pwm_device *p, unsigned long nsecs);
unsigned long pwm_ticks_to_ns(struct pwm_device *p, unsigned long ticks);

int pwm_config_nosleep(struct pwm_device *p, struct pwm_config *c);
int pwm_config(struct pwm_device *p, struct pwm_config *c);

int pwm_set_period_ns(struct pwm_device *p, unsigned long period_ns);
unsigned long pwm_get_period_ns(struct pwm_device *p);
int pwm_set_duty_ns(struct pwm_device *p, unsigned long duty_ns);
unsigned long pwm_get_duty_ns(struct pwm_device *p);
int pwm_set_duty_percent(struct pwm_device *p, int percent);
int pwm_set_polarity(struct pwm_device *p, int active_high);

int pwm_start(struct pwm_device *p);
int pwm_stop(struct pwm_device *p);

int pwm_synchronize(struct pwm_device *p, struct pwm_device *to_p);
int pwm_unsynchronize(struct pwm_device *p, struct pwm_device *from_p);
int pwm_set_handler(struct pwm_device *p, pwm_handler_t handler, void *data);

int pwm_register(struct pwm_device *p, struct device *parent, int id);
int pwm_register_byname(struct pwm_device *p, struct device *parent,
			const char *name);
int pwm_unregister(struct pwm_device *p);

#ifdef CONFIG_GPIO_PWM
struct pwm_device *gpio_pwm_create(int gpio);
int gpio_pwm_destroy(struct pwm_device *p);
#endif
int pwm_set_frequency(struct pwm_device *p, unsigned long freq);
unsigned long pwm_get_frequency(struct pwm_device *p);
int pwm_set_period_ticks(struct pwm_device *p,
					unsigned long ticks);
unsigned long pwm_get_duty_percent(struct pwm_device *p);
int pwm_set_duty_ticks(struct pwm_device *p,
					unsigned long ticks);
#endif
