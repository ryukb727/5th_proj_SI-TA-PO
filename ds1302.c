#include "ds1302.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include "ssd1306.h"

// 1. 워크 구조체 정의
struct work_struct ds1302_work;
extern struct work_struct oled_update_work;
extern int is_running;

// device_driver.c에 있는 변수들을 사용하기 위해 extern 선언
extern int data_update_finish;
extern wait_queue_head_t rotary_wait_queue;
extern struct ds1302 _ds1302;
extern struct ds1302_time _ds1302_date;
// 2. 워크큐가 실제로 하는 일
void ds1302_update_work_func(struct work_struct *work)
{
    // 인터럽트 밖(프로세스 문맥)에서 안전하게 읽기
    ds1302_read_time(_ds1302, &_ds1302_date);
    ds1302_read_date(_ds1302, &_ds1302_date);
    
    // 데이터 읽기 완료 후 대기 중인 유저 앱 깨우기
    data_update_finish = 1;
    wake_up_interruptible(&rotary_wait_queue);
	int running = READ_ONCE(is_running);
	if(running)
		schedule_work(&oled_update_work);

    printk(KERN_INFO "DS1302: %02d:%02d:%02d isrunning: %d\n",_ds1302_date.hours,_ds1302_date.minutes,_ds1302_date.seconds, running);
}

u8 bcd2dec(u8 byte)
{
	u8 high, low;

	low = byte & 0x0f;
	high = (byte >> 4) * 10;

	return (high+low);
}

u8 dec2bcd(u8 byte)
{
	u8 high, low;

	high = (byte / 10) << 4;
    low  = byte % 10;

	return (high+low);
}


int ds1302_gpio_init(struct ds1302 _ds1302)
{
	printk(KERN_INFO "=== ds1302 initializing ===\n");
    if(gpio_request(_ds1302.gpio_sda,"ds1302_sda")){
        printk(KERN_ERR "ERROR: gpio_request() sda : %d\n", _ds1302.gpio_sda);
        return  -1;
    }
    if(gpio_request(_ds1302.gpio_scl,"ds1302_scl")){
        printk(KERN_ERR "ERROR: gpio_request() scl : %d\n", _ds1302.gpio_scl);
        return  -1;
    }
    if(gpio_request(_ds1302.gpio_rst,"ds1302_rst")){
        printk(KERN_ERR "ERROR: gpio_request() rst : %d\n", _ds1302.gpio_rst);
        return  -1;
    }
    
    
    gpio_direction_output(_ds1302.gpio_rst, 0);  
    gpio_direction_output(_ds1302.gpio_scl, 0);  
    gpio_direction_output(_ds1302.gpio_sda, 0); 
    return 0;
}


void ds1302_gpio_free(struct ds1302 _ds1302){
    gpio_free(_ds1302.gpio_sda);
    gpio_free(_ds1302.gpio_scl);
    gpio_free(_ds1302.gpio_rst);
}

void ds1302_init_time_date(struct ds1302 _ds1302 , ds1302_time ds_time)
{
    ds1302_write(_ds1302, ADDR_WRITEPROC, 0x00);

	ds1302_write(_ds1302 ,ADDR_SECONDS, ds_time.seconds);
	ds1302_write(_ds1302 ,ADDR_MINUTES, ds_time.minutes);
	ds1302_write(_ds1302 ,ADDR_HOURS, ds_time.hours);
	ds1302_write(_ds1302 ,ADDR_DATE, ds_time.date);
	ds1302_write(_ds1302 ,ADDR_MONTH, ds_time.month);
	ds1302_write(_ds1302 ,ADDR_DAYOFWEEK, ds_time.dayofweek);
	ds1302_write(_ds1302 ,ADDR_YEAR, ds_time.year);
}

void ds1302_write(struct ds1302 _ds1302, u8 addr, u8 data)
{
	// 1. CE High
    gpio_set_value(_ds1302.gpio_rst, 1);

	// 2. addr 전송
	ds1302_tx(_ds1302, addr);
	// 3. data 전송
	ds1302_tx(_ds1302, dec2bcd(data));     // SIKWON_1217
	// 4. CE low
    gpio_set_value(_ds1302.gpio_rst, 0);
}


// sda mode change
void ds1302_DataLine_Input(struct ds1302 _ds1302)
{
    gpio_direction_input(_ds1302.gpio_sda);
}


void ds1302_DataLine_Output(struct ds1302 _ds1302, int level)
{
	gpio_direction_output(_ds1302.gpio_sda, level);
}

void ds1302_clock(struct ds1302 _ds1302)
{
    gpio_set_value(_ds1302.gpio_scl,1); 
    udelay(5);
    gpio_set_value(_ds1302.gpio_scl,0); 
    udelay(5);
}

void ds1302_tx(struct ds1302 _ds1302, u8 tx)
{
	ds1302_DataLine_Output(_ds1302,0);
	for (int i=0; i < 8; i++)
	{
		if (tx & (1 << i))   // 1이상이면
		{
			// bit가 set상태
			gpio_set_value(_ds1302.gpio_sda, 1);
		}
		else  // bit가 reset상태
		{
			gpio_set_value(_ds1302.gpio_sda, 0);
		}
		ds1302_clock(_ds1302);
	}
}
void ds1302_rx(struct ds1302 _ds1302, uint8_t *data8)
{
    uint8_t temp = 0;
    ds1302_DataLine_Input(_ds1302); 
    udelay(10); // 주소 전송 후 DS1302가 응답할 시간을 넉넉히 줌

    for (int i = 0; i < 8; i++)
    {
        // 1. 현재 라인에 있는 값을 읽음
        if (gpio_get_value(_ds1302.gpio_sda))
        {
            temp |= (1 << i);
        }

        // 2. 마지막 비트가 아니라면 클럭을 주어 다음 데이터를 요청함
        if (i != 7)
        {
            gpio_set_value(_ds1302.gpio_scl, 1);
            udelay(5);
            gpio_set_value(_ds1302.gpio_scl, 0);
            udelay(5);
        }
    }
    *data8 = temp;
}

uint8_t ds1302_read(struct ds1302 _ds1302, uint8_t addr)
{
	uint8_t data8bits = 0;

	// 1. CE High
    gpio_set_value(_ds1302.gpio_rst, 1);
    udelay(5);
	// 2. addr 전송
	ds1302_tx(_ds1302,addr + 1);
    udelay(5);

	// 3. data 읽어 들인다.
	ds1302_rx(_ds1302, &data8bits);
	// 4. CE low
    gpio_set_value(_ds1302.gpio_rst, 0);
    udelay(5);

	return bcd2dec(data8bits);
}

void ds1302_read_time(struct ds1302 _ds1302, struct ds1302_time *_ds_time)
{
	_ds_time->seconds = ds1302_read(_ds1302,ADDR_SECONDS);
	_ds_time->minutes = ds1302_read(_ds1302,ADDR_MINUTES);
	_ds_time->hours = ds1302_read(_ds1302,ADDR_HOURS);
}

void ds1302_read_date(struct ds1302 _ds1302, struct ds1302_time *_ds_time)
{
	_ds_time->date = ds1302_read(_ds1302,ADDR_DATE);
	_ds_time->month = ds1302_read(_ds1302,ADDR_MONTH);
	_ds_time->dayofweek = ds1302_read(_ds1302,ADDR_DAYOFWEEK);
	_ds_time->year = ds1302_read(_ds1302,ADDR_YEAR);
}

char* ds1302_get_strTime(struct ds1302_time _ds_time){
	static char temp[8];
	temp[0] = _ds_time.hours/10 + '0';
	temp[1] = _ds_time.hours%10 + '0';
	temp[2] = '/';
	temp[3] = _ds_time.minutes/10 + '0';
	temp[4] = _ds_time.minutes%10 + '0';
	temp[5] = '/';
	temp[6] = _ds_time.seconds/10 + '0';
	temp[7] = _ds_time.seconds%10 + '0';
	
	return 	temp;
;
}
char* ds1302_get_strDate(struct ds1302_time _ds_time){

	static char temp[10];
	temp[0] = '2';
	temp[1] = '0';
	temp[2] = _ds_time.year/10 + '0';
	temp[3] = _ds_time.year%10 + '0';
	temp[4] = '/';
	temp[5] = _ds_time.month/10 + '0';
	temp[6] = _ds_time.month%10 + '0';
	temp[7] = '/';
	temp[8] = _ds_time.date/10 + '0';
	temp[9] = _ds_time.date%10 + '0';
	return 	temp;

}
/*
	ds1302_gpio_init();   // CLK IO CE을 LOW로 만든다.
	// 시간을 setting
    ds1302_init_time_date();

	while (1)
	{
		pc_command_processing();
		// 1. ds1302 시간을 읽고
		ds1302_read_time();
		// 2. ds1302 날짜를 읽고
		ds1302_read_date();
		// 3. 시간과 날짜를 printf
		printf("***%4d-%2d-%2d %2d:%2d:%2d\n",
				ds_time.year+2000,
				ds_time.month,
				ds_time.date,
				ds_time.hours,
				ds_time.minutes,
				ds_time.seconds);
		// 4. 1초 delay
		HAL_Delay(1000);
	}
*/