#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "ssd1306.h"
#include "ds1302.h"
extern int mode;
extern int set_mode;
extern int clock_h, clock_m;
extern int timer_m, timer_s;
extern int pomo_work, pomo_rest, pomo_repeat;
extern int pomo_state, pomo_cur_repeat;
extern int timer_finished_alert;
extern struct ds1302_time _ds1302_date;
struct i2c_client* gClient = NULL;
struct work_struct oled_update_work;

extern int need_clear;


// --- OLED 제어 함수 ---
int oled_write(struct i2c_client *client, bool is_cmd, uint8_t data)
{
    uint8_t buf[2];

    buf[0] = (is_cmd == true) ? 0x00 : 0x40;   // control byte -> command : data
    buf[1] = data;

    return i2c_master_send(client, buf, 2);
}

 void oled_init(struct i2c_client* client)
{
    msleep(100);               // delay

    oled_write(client, true, 0xAE); // display off
    // 화면 방향
    oled_write(client, true, 0xA1); // 0xA0으로 바꾸면 가로 반전
    oled_write(client, true, 0xC8); // 0xC0으로 바꾸면 세로 반전

    oled_write(client, true, 0x8D); // charge pump ready
    oled_write(client, true, 0x14); // enable charge pump during display on

    oled_write(client, true, 0x20); // Set memory addressing mode
	oled_write(client, true, 0x00); // Horizontal addressing mode 가로 128 다 채우면 자동으로 다음 줄로 넘어감
    oled_write(client, true, 0xAF); // display on

    printk(KERN_INFO "OLED: 초기화 명령 전송 완료!\n");
}

 void oled_clear_screen(struct i2c_client* client)
{
#if 0
    
    int i;
    unsigned int total = 128*8;     // 전체 128*64 픽셀 / 한번에 8bit씩 전송
    for(i=0; i<total; i++)
        oled_write(client, false, 0xff);
    
    /* 반복문을 쓰니 시작 신호 -> 주소 전송 -> 데이터 전송 -> 정지 신호를 보내는 과정이
    1024번 반복됨. 데이터시트의 Co 비트를 이용해 control byte는 한번만 보내고
    이후에는 모두 data byte로 써서 1024바이트를 묶어서
    i2c_master_send를 한번만 호출해 효율을 높이고자 함
    */

    uint8_t buf[1025];
    buf[0] = 0x40;
    int i;
    
    for(i=1; i<1025; i++)
        buf[i] = 0x00;
    
    i2c_master_send(client, buf, 1025);

    /* 마지막 페이지에 클리어 되지 않는 부분 발생
    warning: the frame size of 1040 bytes is larger than 1024 bytes
    위 컴파일 경고 메세지를 보고 찾아보니
    커널 함수가 한 번에 쓸 수 있는 메모리(스택) 크기는 보통 8KB ~ 4KB.
    그런데 uint8_t buf[1025]를 선언하면서 함수 하나가 1KB를 넘으니 경고를 보낸 것
    오버플로우가 발생해 배열 1025개가 온전히 전달되지 않고
    중간에 데이터가 잘리거나 오염되어 화면이 제대로 클리어 되지 않은 것으로 판단하고
    스택 영역이 아닌 데이터 영역에 미리 할당하도록
     uint8_t buf[1025]로 변수 선언
    */
    uint8_t buf[1025];
    buf[0] = 0x40;
    int i;

    for(i=1; i<1025; i++)
        buf[i] = 0xff;
    
    i2c_master_send(client, buf, 1025);

    /* 정적 영역에 배열을 할당해 컴파일 경고는 사라졌으나 화면이 클리어 되지 않는 같은 문제 반복
        ftrace를 이용해 i2c 관련 이벤트를 추적, 로그를 확인한 결과
        데이터는 잘리지 않고 온전히 전송됨을 확인.
        커널 메세지로도 확인할 수 있도록 코드 추가 및 1025바이트 전부 전송 된 것 확인 함*/
    uint8_t buf[1025];
    buf[0] = 0x40;
    int i;
    int ret;
    for(i=1; i<1025; i++)
        buf[i] = 0xff;
    
    ret = i2c_master_send(client, buf, 1025);

    if(ret != 1025){
        // 실제 전송된 바이트 수가 1025가 아니면 에러 메시지 출력
        printk(KERN_ERR "OLED: 전송 실패! 요청: 1025, 실제전송: %d\n", ret);
    }
	else{
        // 정확히 1025바이트를 다 보냈다면 성공 메시지 출력
        printk(KERN_INFO "OLED: 1025바이트 전송 성공 반환\n");
    }
#else
    /*
    1바이트, 1025바이트가 아닌 페이지 단위로 전송
    */
    int page;
    uint8_t buf[129]; // 제어 바이트(1) + 데이터(128)
    
    buf[0] = 0x40; // Data 모드 설정
    
    for(page = 0; page < 8; page++){
        // 1. 매 페이지 시작마다 주소 재지정(가장 중요!)
        oled_write(client, true, 0xB0 + page); // 페이지 번호 (0xB0 ~ 0xB7)
        oled_write(client, true, 0x00);        // 컬럼 시작 위치 (하위 4비트)
        oled_write(client, true, 0x10);        // 컬럼 시작 위치 (상위 4비트)

        // 2. 현재 페이지를 위한 128바이트 데이터 준비 (0x00은 검은색, 0xff는 흰색)
        memset(&buf[1], 0x00, 128); 

        // 3. 129바이트씩 끊어서 전송
        i2c_master_send(client, buf, 129);
    }

#endif
}

 void oled_put_char(struct i2c_client* client, int x, int y, char c)
{
#if 0
    int font_idx = c - 32;  // SSD1306_font 배열이 아스키코드 32인 공백부터 시작하므로 32를 빼서 SSD1306_font상 순서(인덱스)를 구함
    int i;

    // 1. 위치 설정(cmd)
    oled_write(client, true, 0xB0 + y);                // 0xB0 = set page address : y축 즉 세로 위치 원점
    oled_write(client, true, 0x00 + (x & 0x0F));       // 0x00 = set lower column start address for page addressing mode : x축 즉 가로 위치의 하위 4비트
    oled_write(client, true, 0x10 + ((x>>4) & 0x0F));  // 0x10 = set higher column start address for page addressing mode : x축 즉 가로 위치의 상위 4비트

    // 2. 폰트 데이터 5바이트 전송(data)
    for(i=0; i<5; i++){
        oled_write(client, false, SSD1306_font[font_idx][i]);
    }

    // 글자 사이 간격을 위해 빈 줄 하나 추가
    oled_write(client, false, 0x00);
#else
    int font_idx = c - 32;  // SSD1306_font 배열이 아스키코드 32인 공백부터 시작하므로 32를 빼서 SSD1306_font상 순서(인덱스)를 구함
    uint8_t buf[7];     // 컨트롤바이트 1 + 폰트 5 + 간격 1

    // 1. 위치 설정(cmd)
    oled_write(client, true, 0xB0 + y);                // 0xB0 = set page address : y축 즉 세로 위치 원점
    oled_write(client, true, 0x00 + (x & 0x0F));       // 0x00 = set lower column start address for page addressing mode : x축 즉 가로 위치의 하위 4비트
    oled_write(client, true, 0x10 + ((x>>4) & 0x0F));  // 0x10 = set higher column start address for page addressing mode : x축 즉 가로 위치의 상위 4비트

    // 2. 폰트 데이터를 5바이트 묶어서 한 번에 전송
    buf[0] = 0x40;
    memcpy(&buf[1], SSD1306_font[font_idx], 5);
    buf[6] = 0x00;

    i2c_master_send(client, buf, 7);
#endif
}

 void oled_put_string(struct i2c_client* client, int x, int y, char* str)
{
    while(*str){
        oled_put_char(client, x, y, *str++);
        x += 6;     // 한 글자 당 5픽셀 + 간격 1픽셀
    }
}

 void oled_put_num_16x10(struct i2c_client* client, int x, int y, int idx)
{
    uint8_t buf[11];
    buf[0] = 0x40; // 데이터 모드

    // 1. 위쪽 절반 (y 페이지)
    oled_write(client, true, 0xB0 + y);
    oled_write(client, true, 0x00 + (x & 0x0F));
    oled_write(client, true, 0x10 + ((x >> 4) & 0x0F));
    memcpy(&buf[1], SSD1306_font_16x10[idx][0], 10);
    i2c_master_send(client, buf, 11);

    // 2. 아래쪽 절반 (y + 1 페이지)
    oled_write(client, true, 0xB0 + y + 1);
    oled_write(client, true, 0x00 + (x & 0x0F));
    oled_write(client, true, 0x10 + ((x >> 4) & 0x0F));
    memcpy(&buf[1], SSD1306_font_16x10[idx][1], 10);
    i2c_master_send(client, buf, 11);
}

 void oled_put_string_16x10(struct i2c_client* client, int x, int y, char* str)
{
    while(*str){
        int idx = -1;

        if(*str>='0' && *str<='9'){
            idx = *str - '0';
        }
        else if(*str == ':'){
            idx = 10;
        }
        else if(*str == ' '){
            idx = 11;
        }
		
        if(idx != -1){
            oled_put_num_16x10(client, x, y, idx);
            x += 12;
        }
        str++;
    }
}

// 워크큐에서 oled_put 함수 호출
void oled_update_work_func(struct work_struct *work)
{
    char time_buf[16];
    char status_buf[24];

    if(gClient == NULL) return;

    // 모드 바뀔 때 한번 화면 클리어
    if(need_clear){
        oled_clear_screen(gClient);
        need_clear = 0; // 지운 후 플래그 초기화
    }

    // 1. [상단] 모드 제목 출력
    if(mode == 0)
		oled_put_string(gClient, 28, 0, "  [ CLOCK ]  ");
    else if(mode == 1)
		oled_put_string(gClient, 28, 0, "  [ TIMER ]  ");
    else
		oled_put_string(gClient, 28, 0, "[ POMODORO ]");

    // 2. [중앙 & 하단] 모드별 상세 로직
    if(mode == 0){ // 시계 모드
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", _ds1302_date.hours, _ds1302_date.minutes, _ds1302_date.seconds);
        oled_put_string_16x10(gClient, 16, 3, time_buf);
        
        oled_draw_underline(gClient, 16, 5, 22, (set_mode == 2)); // 시 수정
        oled_draw_underline(gClient, 52, 5, 22, (set_mode == 1)); // 분 수정
        
        if(set_mode > 0)
			oled_put_string(gClient, 10, 7, "  Setting...    ");
        else
			oled_put_string(gClient, 10, 7, "                ");
    } 
    else if(mode == 1){ // 타이머 모드
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d", timer_m, timer_s);
        oled_put_string_16x10(gClient, 34, 3, time_buf);
        
        oled_draw_underline(gClient, 34, 5, 22, (set_mode == 1)); // 분 수정
        oled_draw_underline(gClient, 70, 5, 22, (set_mode == 0)); // 초 수정
        
        if(set_mode < 2)
            oled_put_string(gClient, 10, 7, "  Setting...    ");
        else{
            if(timer_finished_alert)
                oled_put_string(gClient, 10, 7, "   Time's up!!     ");
            else
                oled_put_string(gClient, 10, 7, "   Timer Start!!     ");
        }
    } 
    else{ // 뽀모도로 모드 (mode == 2)
        if(set_mode == 0){ // 집중 시간 설정
            snprintf(time_buf, sizeof(time_buf), "%02d:00", pomo_work);
            oled_put_string_16x10(gClient, 34, 3, time_buf);
            oled_put_string(gClient, 10, 7, "Set Work Time   ");
            oled_draw_underline(gClient, 34, 5, 22, true); 
            oled_draw_underline(gClient, 70, 5, 22, false);
        } 
        else if(set_mode == 1){ // 휴식 시간 설정
            snprintf(time_buf, sizeof(time_buf), "%02d:00", pomo_rest);
            oled_put_string_16x10(gClient, 34, 3, time_buf);
            oled_put_string(gClient, 10, 7, "Set Rest Time   ");
            oled_draw_underline(gClient, 34, 5, 22, true);
            oled_draw_underline(gClient, 70, 5, 22, false);
        } 
        else if(set_mode == 2){ // 반복 횟수 설정
            snprintf(time_buf, sizeof(time_buf), "  %02d   ", pomo_repeat);
            oled_put_string_16x10(gClient, 34, 3, time_buf);
            oled_put_string(gClient, 10, 7, "Set Repeat Count");
            oled_draw_underline(gClient, 34, 5, 58, true); 
        } 
        else{ // set_mode == 3 (작동 중)
            snprintf(time_buf, sizeof(time_buf), "%02d:%02d", timer_m, timer_s);
            oled_put_string_16x10(gClient, 34, 3, time_buf);
            
            const char* state_name = (pomo_state == 0) ? "Focus" : "Rest ";
            int display_round;

            if(pomo_state == 0){
                display_round = pomo_cur_repeat + 1;
            }
            else{
                display_round = pomo_cur_repeat;
            }
            snprintf(status_buf, sizeof(status_buf), "%s Time! %d/%d", state_name, display_round, pomo_repeat);
            oled_put_string(gClient, 10, 7, status_buf);
            
            oled_draw_underline(gClient, 0, 5, 128, false); // 밑줄 모두 지우기
        }
    }
}

// x 위치부터 len만큼 밑줄을 긋거나 지우는 함수
void oled_draw_underline(struct i2c_client* client, int x, int y, int len, bool show)
{
    uint8_t buf[129];
    buf[0] = 0x40; // Data mode
    memset(&buf[1], show ? 0x01 : 0x00, len); // 0x01은 가장 윗 픽셀 하나만 켬 (밑줄 효과)

    oled_write(client, true, 0xB0 + y); // 밑줄을 그을 페이지
    oled_write(client, true, 0x00 + (x & 0x0F));
    oled_write(client, true, 0x10 + ((x >> 4) & 0x0F));
    i2c_master_send(client, buf, len + 1);
}

// --- 커널 드라이버 구조 ---
 int oled_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
    gClient = client;
    printk(KERN_INFO "OLED: 장치를 찾았습니다! 주소: 0x%x\n", client->addr);
    printk(KERN_INFO "OLED: gClient에 주소를 할당했습니다: %p\n", gClient);

    oled_init(client);
    oled_clear_screen(client);

    return 0;
}

 const struct i2c_device_id oled_id[] = {
    { "ssd1306", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, oled_id);

 struct i2c_driver oled_driver = {
    .driver = {
        .name = "ssd1306"
    },
    .probe = oled_probe,
    .id_table = oled_id,
};


// 불러서 쓸 초기화 함수와 free 함수
int init_ssd1306(void)
{
	printk(KERN_INFO "=== ssd1306 initializing ===");
    return i2c_add_driver(&oled_driver);
}
void free_ssd1306(void)
{
	printk(KERN_INFO "=== ssd1306 exit ===");
    i2c_del_driver(&oled_driver);
}

//module_i2c_driver(oled_driver);
//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("RGB");
