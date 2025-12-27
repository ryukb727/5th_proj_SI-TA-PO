/*
 * ds1302.h
 *
 *  Created on: Dec 17, 2025
 *      Author: User
 */

#ifndef _DS1302_H_
#define _DS1302_H_
#include <linux/types.h>
#include <linux/workqueue.h>


#define ADDR_SECONDS 	0x80
#define ADDR_MINUTES 	0x82
#define ADDR_HOURS 	 	0x84
#define ADDR_DATE	 	0x86
#define ADDR_MONTH	 	0x88
#define ADDR_DAYOFWEEK 	0x8a
#define ADDR_YEAR 		0x8c
#define ADDR_WRITEPROC 	0x8e

typedef struct ds1302_time
{
	u8 seconds;
	u8 minutes;
	u8 hours;
	u8 date;
	u8 month;
	u8 dayofweek;/// 1 ~ 7
	u8 year;
	u8 ampm; // 1: pm 0: am
	u8 hourMode; // 0: 24 1: 12

} ds1302_time;

typedef struct ds1302 {
    u8 gpio_sda;
    u8 gpio_scl;
    u8 gpio_rst;
}ds1302;


// 외부에서 사용할 워크큐 구조체 선언 (extern)
extern struct work_struct ds1302_work;

// 워크큐 초기화 및 데이터 읽기 함수 프로토타입
void ds1302_update_work_func(struct work_struct *work);

unsigned char bcd2dec(unsigned char byte);
unsigned char dec2bcd(unsigned char byte);

int ds1302_gpio_init(struct ds1302 _ds1302  );
void ds1302_gpio_free(struct ds1302 _ds1302);
void ds1302_init_time_date(struct ds1302 _ds1302 ,ds1302_time ds_time);
void ds1302_write(struct ds1302 _ds1302 ,u8 addr, u8 data);
void ds1302_DataLine_Input(struct ds1302 _ds1302 );
void ds1302_DataLine_Output(struct ds1302 _ds1302 ,int level);
void ds1302_clock(struct ds1302 _ds1302);
void ds1302_tx(struct ds1302 _ds1302, u8 tx);
void ds1302_rx(struct ds1302 _ds1302, u8 *data8);
u8 ds1302_read(struct ds1302 _ds1302, u8 addr);
void ds1302_read_time(struct ds1302 _ds1302d, struct ds1302_time *_ds_time);
void ds1302_read_date(struct ds1302 _ds1302, struct ds1302_time *_ds_time);

char* ds1302_get_strTime(struct ds1302_time _ds_time);
char* ds1302_get_strDate(struct ds1302_time _ds_time);

#endif /* _DS1302_H_ */
