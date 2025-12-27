#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#define DRIVER_NAME "rotary_device_driver"
#define CLASS_NAME "rotary_device_class"

// DS1302 
#define DS1302_SDA 2
#define DS1302_SCL 3 
#define DS1302_RST 4

// OLED
#define OLED_SDA 0
#define OLED_SCL 1
#define OLED_RES 5

// LED bar
#define LED00 7
#define LED01 8
#define LED02 9
#define LED03 10 
#define LED04 11
#define LED05 12
#define LED06 13
#define LED07 14

// rotary encoder 
#define ROTARY_S1 23
#define ROTARY_S2 24
#define ROTARY_KEY 25

// TACT 
#define TACT_SW 20

#define DEBOUNCE_MS 6

static int mode = 0; 		// 0:clock, 1:timer, 2:pomodoro
static int set_mode = 0;	// 0 - 4 각 mode에서 필요한 만큼만 사용

/* 디바운싱을 위한 변수 */
static unsigned long last_rotary_s1_time = 0;
static unsigned long last_rotary_key_time = 0;
static unsigned long last_tact_time = 0;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kkk");
MODULE_DESCRIPTION("rotary driver");
static dev_t device_number;
static struct cdev rotary_cdev;
static struct class *rotary_class;

static int rotary_s1_irq; 	// int number of rotary s1
static int rotary_key_irq;		// int number of rotary key
static int tact_irq; 			// int number of tact
static long rotary_value = 0; // record rotary encoder Value 
static long pre_rotary_value = 0;
static int data_update_finish = 0; // data ready 
// MACRO DECLARE_WAIT_QUEUE_HEAD for make Wait Queue
static DECLARE_WAIT_QUEUE_HEAD(rotary_wait_queue);
// struct wait_queue_head rotary_wait_queue = {
//		.lock,
//		.head
// }

// 각 모드에서 사용하는 실제 설정 값
static int clock_h = 0, clock_m = 0;
static int timer_m = 0, timer_s = 0;
static int pomo_work = 25, pomo_rest = 5, pomo_repeat = 3;
static int pomo_state = 0;   		 // 0: 집중 시간, 1: 쉬는 시간
static int pomo_cur_repeat = 0;		// 현재 몇 번째 반복인지

// 상태 관리용 (타이머 작동 여부 등)
static int is_running = 0; 
struct timer_list my_kernel_timer; // 카운트다운용 커널 타이머

static void clock_handler(void);
static void timer_handler(void);
static void pomodoro_handler(void);

static void my_timer_callback(struct timer_list *t) {
    if (!is_running) return;

    if (mode == 1) { 		// 타이머 모드
        if (timer_s > 0) timer_s--;
        else if (timer_m > 0)
		{
			timer_m--; timer_s = 59;
			printk(KERN_INFO "Timer: %d min left\n", timer_m); // 분 단위 로그
		}
        else 
		{ 
            is_running = 0; 
            printk(KERN_INFO "### Timer Finished! ###\n"); 
        }
    } 
    else if (mode == 2) { // 뽀모도로 모드
        if (timer_s > 0)
		{
            timer_s--;
        }
		else
		{
            if (timer_m > 0)
			{
                timer_m--;
                timer_s = 59;
				printk(KERN_INFO "Pomo [%s]: %d min left\n", (pomo_state == 0 ? "WORK" : "REST"), timer_m);
            }
			else
			{
                // 분:초가 모두 0이 된 상황 -> 상태 전환
                if (pomo_state == 0) 	// 집중 끝 -> 휴식 시작
				{
					pomo_state = 1;
                    timer_m = pomo_rest;
                    timer_s = 0;
                    printk(KERN_INFO ">>> Pomodoro: WORK Done! REST Starts (%d min) <<<\n", pomo_rest);
                } 
                else	 // 휴식 끝 -> 다음 회차 집중 또는 종료
                {
					pomo_cur_repeat++;
                    if (pomo_cur_repeat < pomo_repeat)
					{
                        pomo_state = 0;
                        timer_m = pomo_work;
                        timer_s = 0;
                        printk(KERN_INFO ">>> Pomodoro: REST Done! Round %d/%d Starts <<<\n", pomo_cur_repeat + 1, pomo_repeat);
                    }
					else
					{
                        is_running = 0; // 모든 반복 완료
                        pomo_cur_repeat = 0;
                        pomo_state = 0;
                        printk(KERN_INFO "### Pomodoro: ALL SETS FINISHED! ###\n");
                    }
                }
            }
        }
    }

    // 데이터 업데이트 알림
    data_update_finish = 1;
    wake_up_interruptible(&rotary_wait_queue);

    // 타이머 재등록
    if (is_running) {
        mod_timer(&my_kernel_timer, jiffies + HZ);
    }
}

static irqreturn_t rotary_key_irq_handler(int irq, void *dev_id) {
    unsigned long current_time = jiffies;		// jiffies는 jiffies.h에 선언되어 있음
    int cur_state;
    
    // 1. 초기 상태 변경: Pull-up이므로 안 눌렸을 때 1
    static int prev_state = 1; 

    // 디바운싱 (10ms)
    if ((current_time - last_rotary_key_time) < msecs_to_jiffies(20)) {
        return IRQ_HANDLED;
    }
    last_rotary_key_time = current_time;

    // 눌렀을 때 0(Low)
   if (gpio_get_value(ROTARY_KEY) == 0) {
        if (mode == 0) set_mode = (set_mode + 1) % 2;      // Clock: 2단계
        else if (mode == 1) set_mode = (set_mode + 1) % 3; // Timer: 3단계
        else if (mode == 2) set_mode = (set_mode + 1) % 4; // Pomo: 4단계

        // [핵심] '시작' 단계(Step)에 진입했을 때의 동작만 여기서 직접 처리
        if (mode == 1 && set_mode == 2) { // 타이머 시작
            is_running = 1;
            mod_timer(&my_kernel_timer, jiffies + HZ);
            printk(KERN_INFO "Timer: Started via Key\n");
        }
        else if (mode == 2 && set_mode == 3) { // 뽀모도로 시작
            is_running = 1;
            pomo_state = 0; pomo_cur_repeat = 0;
            timer_m = pomo_work; timer_s = 0;
            mod_timer(&my_kernel_timer, jiffies + HZ);
            printk(KERN_INFO "Pomodoro: Started via Key\n");
        }

        // 단계 전환 시 로터리 회전 값은 초기화하여 새로운 설정 준비
        rotary_value = 0; 

		// // ### 뽀모도로 3번 스템으로 넘어갔지만 뽀모도로 타이머 시작 안함 문제
		// // 버튼을 눌러 Step이 바뀌었으므로, 해당 모드의 핸들러를 즉시 실행하도록 아래 3줄 추가
		// // 하니까 스텝 안넘어감
        // if (mode == 0) clock_handler();
        // else if (mode == 1) timer_handler();
        // else if (mode == 2) pomodoro_handler();
		// 인터럽트 핸들러 내부가 너무 복잡해져서 생긴 문제인 것 같아 빼고 위에 else if 추가하고 뽀모도로 시작 로직 넣음

        // 유저 프로그램(OLED)에 상태 변경 알림
        data_update_finish = 1;
        wake_up_interruptible(&rotary_wait_queue);
        
        printk(KERN_INFO "Key Pressed! Mode:%d, Step:%d\n", mode, set_mode);
    }

    // 버튼을 떼는 순간 (Rising Edge: 0 -> 1)
    else if (prev_state == 1 && cur_state == 0) {
        prev_state = 1;  // '뗌' 상태로 업데이트
    }
    
    return IRQ_HANDLED;
}

static void set_num(int* num, int max)
{
	int old_val = *num;
	*num +=(int)rotary_value;

	if(*num > max)
		*num = 0;

	else if(*num < 0)
		*num = max;

	if (old_val != *num) {
        printk(KERN_INFO "Setting Change: %d -> %d (Max: %d)\n", old_val, *num, max);
    }

	rotary_value = 0;
}

static void clock_handler(void)
{
	switch (set_mode) {
		case 0:						// 분 설정
			set_num(&clock_m, 59);
			break;
		case 1:						// 시 설정
			set_num(&clock_h, 23);
			break;
		default:					// 시계 화면 상태 (RTC 업데이트 등)
        	printk(KERN_INFO "Clock: Settings Saved\n");
			break;
    }
}

static void timer_handler(void)
{
	switch (set_mode) {
		case 0:					// 초 설정
			is_running = 0;
			del_timer(&my_kernel_timer); // 설정 중엔 타이머 중지	
			set_num(&timer_s, 59);
			break;
		case 1:						// 분 설정
			set_num(&timer_m, 99);
			break;
		case 2:						// 타이머 동작 시작
			is_running = 1;
			mod_timer(&my_kernel_timer, jiffies + HZ); // 1초 뒤 작동
			printk(KERN_INFO "Timer: Started\n");
			break;
	}
}

static void pomodoro_handler(void)
{
	switch (set_mode) {
		case 0:						// 집중 시간 설정 (최대 99분)
			is_running = 0;
			del_timer(&my_kernel_timer); 	// 설정 중엔 타이머 중지	
			set_num(&pomo_work, 99);
			break;
		case 1:						// 쉬는 시간 설정 (최대 99분)
			set_num(&pomo_rest, 99);
			break;
		case 2:						// 반복 횟수 설정 (최대 10번)
			set_num(&pomo_repeat, 10);
			break;
		case 3:						// 뽀모도로 시작
			is_running = 1;
			pomo_state = 0;      	// 집중(Work)부터 시작
            pomo_cur_repeat = 0; 	// 반복 횟수 초기화
            timer_m = pomo_work; 	// 사용자가 설정한 집중 시간으로 세팅
            timer_s = 0;         	// 초는 0으로 시작
			mod_timer(&my_kernel_timer, jiffies + HZ); // 1초 뒤 작동
			printk(KERN_INFO "Pomodoro: Started\n");
			break;
	}
}

// ============ TACT Switch ==============
/* 인터럽트 핸들러 (Pull-down 저항 연결 기준 ) */
static irqreturn_t tact_irq_handler(int irq, void *dev_id) {
    unsigned long current_time = jiffies;		// jiffies는 jiffies.h에 선언되어 있음
    int cur_state;
    
    // 1. 초기 상태 변경: Pull-down이므로 안 눌렸을 때 0
    static int prev_state = 0; 

    // 디바운싱 (10ms)
    if ((current_time - last_tact_time) < msecs_to_jiffies(10)) {
        return IRQ_HANDLED;
    }
    last_tact_time = current_time;

    // 현재 핀 상태 읽기 (Pull-down: 누르면 1, 떼면 0)
    cur_state = gpio_get_value(TACT_SW); 

    // 버튼을 누르는 순간 (Rising Edge: 0 -> 1)
    if (cur_state == 1) { // Rising Edge
        // 모드를 0, 1, 2로 순환 (0:Clock, 1:Timer, 2:Pomo)
        mode = (mode + 1) % 3; 
        set_mode = 0;
        rotary_value = 0;
        is_running = 0; // 모드 변경 시 타이머 정지
        del_timer(&my_kernel_timer);

        printk(KERN_INFO "!!! MODE CHANGED !!! Mode: %d\n", mode);

        // LED 즉각 제어 (밀림 방지)
        gpio_set_value(LED00, (mode == 0));
        gpio_set_value(LED01, (mode == 1));
        gpio_set_value(LED02, (mode == 2));

        data_update_finish = 1;
        wake_up_interruptible(&rotary_wait_queue);

        prev_state = 1; // '눌림' 상태로 업데이트
    }
    // 버튼을 떼는 순간 (Falling Edge: 1 -> 0)
    else if (prev_state == 1 && cur_state == 0) {
        prev_state = 0;  // '뗌' 상태로 업데이트
    }
    
    return IRQ_HANDLED;
}

// -------- interrupt handler (ISR(Interrupt Service Routine))
static irqreturn_t rotary_s1_irq_handler(int irq, void * dev_id)
{
	// #### 로터리 방향 문제 (항상 ++만 됨)
	// 1. 인터럽트 발생 즉시 상태(시계 or 반시계)를 읽도록 순서 변경
    int val_s1 = gpio_get_value(ROTARY_S1);
    int val_s2 = gpio_get_value(ROTARY_S2);

	unsigned long current_time = jiffies; // read current clock(Hz) 
	unsigned long debounce_jiffies = msecs_to_jiffies(DEBOUNCE_MS);

	if(time_before(current_time, last_rotary_s1_time + debounce_jiffies))
	{
		return IRQ_HANDLED;//5ms under then noise
	}
	last_rotary_s1_time = current_time;
	// if s1 falling edge then INT occurs
	// S1(0) & S2(1) --> clock(CW)
	// S1(1) & S2(0) --> reverse clock(CCW)
	// int val_s1 = gpio_get_value(ROTARY_S1);
	// int val_s2 = gpio_get_value(ROTARY_S2);
	if(val_s1 == 0)
	{
		if(val_s2 == 1)
		{
			rotary_value++;
			printk(KERN_INFO "Rotary: Clockwise (++) -> %ld\n", rotary_value);
		}
		else
		{
			rotary_value--;
			printk(KERN_INFO "Rotary: Counter-Clockwise (--) -> %ld\n", rotary_value);
		}
	}

	switch(mode) {
		case 0:
			clock_handler();
			break;
		case 1:
			timer_handler();
			break;
		case 2:
			pomodoro_handler();
			break;
	}

	// alert data update signal to user program
	data_update_finish = 1;
	wake_up_interruptible(&rotary_wait_queue); // linux/wait.h 

	return IRQ_HANDLED;
}

// ------------ read function ----------------
/*
	int main()
	{
		int fd;
		char buff[200];
		while(1)
		{
			........
			read(fd,buff,200);
		}
	}
 */
static ssize_t rotary_read(struct file*file, char __user* user_buff, size_t count, loff_t *ppos)
{
	char buffer[64];
	int len;
	// blocking i/o: wait while data
	wait_event_interruptible(rotary_wait_queue, data_update_finish != 0);
		

	data_update_finish = 0;
	len = snprintf(buffer, sizeof(buffer), "%d:%d:%ld\n", mode, set_mode, rotary_value);
	if(copy_to_user(user_buff,buffer,len))
	{
		return -EFAULT;
	}
	return len;

}
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = rotary_read
};

static int __init rotary_driver_init(void)
{
	
	int ret = 0;

	printk(KERN_INFO "=== rotatry initializing ===\n");
	// 1. alloc device number
	if(( ret = alloc_chrdev_region(&device_number, 0, 1, DRIVER_NAME )) == -1)
	{

		printk(KERN_ERR "ERROR: alloc_chrdev_regin()\n");

		return ret;
	}
	// 2. register char device 
	cdev_init(&rotary_cdev, &fops);
	if((ret = cdev_add(&rotary_cdev,device_number, 1)) == -1)
	{
	
		printk(KERN_ERR "ERROR: chrdev_add()\n");
		unregister_chrdev_region(device_number,1);
		return ret;
	}
	// 3. create class & create device ex) /dev/rotary_driver
	rotary_class = class_create(THIS_MODULE, CLASS_NAME);	
	if(IS_ERR(rotary_class))
	{
		printk(KERN_ERR "ERROR: class_create()\n");
		cdev_del(&rotary_cdev);
		unregister_chrdev_region(device_number,1);
		return PTR_ERR(rotary_class);
	}
	device_create(rotary_class, NULL, device_number, NULL, DRIVER_NAME);

	// 1. GPIO 유효성 검사		// 다른 곳에서 이미 쓰고 있지 않은지 확인
    if (!gpio_is_valid(DS1302_SDA) || !gpio_is_valid(DS1302_SCL) || !gpio_is_valid(DS1302_RST)
		 || !gpio_is_valid(OLED_SDA) || !gpio_is_valid(OLED_SCL) || !gpio_is_valid(OLED_RES)
		 || !gpio_is_valid(LED00) || !gpio_is_valid(LED01) || !gpio_is_valid(LED02)
		 || !gpio_is_valid(LED03) || !gpio_is_valid(LED04) || !gpio_is_valid(LED05)
		 || !gpio_is_valid(LED06) || !gpio_is_valid(LED07) || !gpio_is_valid(ROTARY_S1)
		 || !gpio_is_valid(ROTARY_S2) || !gpio_is_valid(ROTARY_KEY) || !gpio_is_valid(TACT_SW)) {
        printk(KERN_ALERT "GPIO Invalid!\n");
        return -ENODEV;
    }

	// 4. request GPIO
	if(gpio_request(DS1302_SDA , "RTC_SDA") || gpio_request(DS1302_SCL, "RTC_SCL") || gpio_request(DS1302_RST, "RTC_RST")
		 || gpio_request(OLED_SDA, "OLED_SDA") || gpio_request(OLED_SCL, "OLED_SCL") || gpio_request(OLED_RES, "OLED_RES")
		 || gpio_request(LED00, "LED00") || gpio_request(LED01, "LED01") || gpio_request(LED02, "LED02")
		 || gpio_request(LED03, "LED03") || gpio_request(LED04, "LED04") || gpio_request(LED05, "LED05")
		 || gpio_request(LED06, "LED06") || gpio_request(LED07, "LED07") || gpio_request(ROTARY_S1, "ROTARY_S1")
		 || gpio_request(ROTARY_S2, "ROTARY_S2") || gpio_request(ROTARY_KEY, "ROTARY_KEY") || gpio_request(TACT_SW, "TACT_SW"))
	{
			
		printk(KERN_ERR "ERROR: gpio_request()\n");

		return -1;
	}

	// set input mode
	gpio_direction_input(ROTARY_S1);
	gpio_direction_input(ROTARY_S2);
	gpio_direction_input(ROTARY_KEY);
	gpio_direction_input(TACT_SW);

	// set output mode
	gpio_direction_output(LED00, 0);
	gpio_direction_output(LED01, 0);
	gpio_direction_output(LED02, 0);
	gpio_direction_output(LED03, 0);
	gpio_direction_output(LED04, 0);
	gpio_direction_output(LED05, 0);
	gpio_direction_output(LED06, 0);
	gpio_direction_output(LED07, 0);

	// timer 초기화
	timer_setup(&my_kernel_timer, my_timer_callback, 0);

	// 5. assign GPIO to irq
	rotary_s1_irq = gpio_to_irq(ROTARY_S1);
	ret = request_irq(rotary_s1_irq, rotary_s1_irq_handler /* int handler */ , IRQF_TRIGGER_FALLING,"my_rotary_S1_irq",NULL); 
	if(ret)
	{

		printk(KERN_ERR "ERROR: request_irq()\n");
		return ret;
	}

	rotary_key_irq = gpio_to_irq(ROTARY_KEY);
	ret = request_irq(rotary_key_irq, rotary_key_irq_handler /* int handler */ , IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"my_rotary_key_irq",NULL); 
	if(ret)
	{

		printk(KERN_ERR "ERROR: request_irq()\n");
		return ret;
	}

	tact_irq = gpio_to_irq(TACT_SW);
	ret = request_irq(tact_irq, tact_irq_handler /* tact handler */ , IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"my_tact_irq",NULL); 
	if(ret)
	{

		printk(KERN_ERR "ERROR: request_irq()\n");
		return ret;
	}

	printk(KERN_INFO "Info: device driver init Success\n");
	return ret;
}

static void __exit rotary_driver_exit(void)
{
	del_timer_sync(&my_kernel_timer);
	free_irq(rotary_s1_irq,NULL);
	free_irq(rotary_key_irq,NULL);
	free_irq(tact_irq,NULL);
	gpio_free(DS1302_SDA);
	gpio_free(DS1302_SCL);
	gpio_free(DS1302_RST);
	gpio_free(OLED_SDA);
	gpio_free(OLED_SCL);
	gpio_free(OLED_RES);
	
	gpio_free(ROTARY_S2);
	gpio_free(ROTARY_S1);
	gpio_free(ROTARY_KEY);

	for(int i = 0; i < 8; i++)
	{
		gpio_free(LED00 + i);
	}

	gpio_free(TACT_SW);

	device_destroy(rotary_class,device_number);
	class_destroy(rotary_class);
	cdev_del(&rotary_cdev);
	unregister_chrdev_region(device_number,1);
	printk(KERN_INFO"Info : rotary_driver_exit\n");
}

module_init(rotary_driver_init);
module_exit(rotary_driver_exit);
