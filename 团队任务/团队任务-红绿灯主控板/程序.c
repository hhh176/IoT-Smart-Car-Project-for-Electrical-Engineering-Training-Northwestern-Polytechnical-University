/* 
   主控板代码：车路协同智能控制主控（修复夜间报站与数码管归属）
   单片机型号：STC8H3K64S4  晶振频率：11.0592MHz
*/

#include "STC8H.h"             
#include "intrins.h"           
#include "tm1650.h"

#define uchar unsigned char                         
#define uint  unsigned int     

#define HEADLEN     5    
#define BKM_OFFSET  4    
#define LEN_OFFSET  2    
sbit BUSY = P1^2;        
uchar nBkm = 0x00;
uchar code head[HEADLEN] = {0xfd,0x00,0x00,0x01,0x00};

bit busy;    

sbit PWM1  = P2^5;             // 舵机1
sbit PWM2  = P2^6;             // 舵机2
sbit IR_IN = P3^2;             // 红外传感器
sbit    k1 = P3^7;             // 按键k1
sbit    k2 = P3^6;             // 按键k2
sbit   led = P5^5;             // 运行指示灯

uint Servo1PwmDat = 1700;      // 默认开机抬杆位置

/* 
   Mode 状态定义：
   0: 日间运行状态 (无车到站，数码管显示 000)
   1: 夜间运行状态 (无车到站，数码管显示 000)
   2: 小车到站倒计时状态 (不论日夜，数码管显示 00X)
*/
uchar Mode = 0;                
uchar SavedMode = 0;           // 用于暂存进入倒计时前的日/夜状态，放行后好恢复

// --- 小车无线交互核心变量 ---
uchar current_station = 0;     // 当前小车所在的站点ID (1-5)，0表示没有车到站
uint station_timer = 0;        // 5秒放行倒计时器 (由于20ms心跳，250次 = 5秒)
bit start_countdown = 0;       // 启动5秒放行倒计时标志

// TM1650 共阳/共阴数码管数字段码表（'0'-'9'）
uchar code num_table[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};

void delayms(uint ms)
{
    uint i, j;
    for(i=0; i<85; i++)         
        for(j=0; j<ms; j++);
}

void Uart2Send(uchar dat)
{
    while (busy);
    busy = 1;
    S2BUF = dat;
}

void Speech(uchar *buf)
{
    uchar i = 0, xor = 0x00, ch = 0x00, len = 0x00;
    while(buf[len++]);
    len--; 
    for(i = 0; i < HEADLEN; i++)
    {
        if(i == BKM_OFFSET) ch = nBkm << 3; 
        else if(i == LEN_OFFSET) ch = len + 3;
        else ch = head[i];
        xor ^= ch;
        while (busy); busy = 1; S4BUF = ch; 
    }
    for(i = 0; i < len; i++)
    {
        xor ^= buf[i];
        while (busy); busy = 1; S4BUF = buf[i];
    }
    while (busy); busy = 1; S4BUF = xor;         
    
    delayms(10); 
}

void init()  
{
    P0M1 = 0x00;   P0M0 = 0x00;   
    P1M1 = 0x00;   P1M0 = 0x00;   
    P2M1 = 0x00;   P2M0 = 0x00;   
    P3M1 = 0x00;   P3M0 = 0xc0;   // P3.6, P3.7 设为准双向
    P4M1 = 0x00;   P4M0 = 0x00;   
    P5M1 = 0x00;   P5M0 = 0x00;   

    // 定时器0驱动舵机
    TMOD &= 0xF0; TMOD |= 0x01;                 
    TL0 = 0x00; TH0 = 0x00; TR0 = 1; ET0 = 1;                      

    // 串口2和串口4波特率发生器由Timer2提供 (9600bps)
    S2CON = 0x10; S4CON = 0x10;
    AUXR &= 0xFB;                
    T2L = 0xE8; T2H = 0xFF; AUXR |= 0x10;                 
    busy = 0;

    IE2 = 0x11; EA = 1;               
}

void Timer0(uint pwm)           
{
    uint value = 65535-pwm;       
    TR0 = 0; TL0=value; TH0=value>>8; TR0 = 1;                     
}

// 主控板板载数码管显示刷新（常态全部显示000，小车到站时显示00X）
void RefreshDisplay()
{
    if (Mode == 2 && current_station > 0) // 小车到站模式，静态显示 "00X"
    {
        Set1650(0x68, num_table[0]); // 百位 "0"
        Set1650(0x6a, num_table[0]); // 十位 "0"
        Set1650(0x6c, num_table[current_station % 10]); // 个位动态显示站点ID
    }
    else // 日间常态(Mode=0) 或 夜间常态(Mode=1)，主控板数码管均统一保持显示 "000"
    {
        Set1650(0x68, num_table[0]); // 百位 "0"
        Set1650(0x6a, num_table[0]); // 十位 "0"
        Set1650(0x6c, num_table[0]); // 个位 "0"
    }
}

void main()
{     
    uint led_count = 0;
    init();                
    Init1650();            
    
    // 开机初始化：给红绿灯发标准的5字节日间包（绿灯10秒(0x0A)，红灯12秒(0x0C)）
    delayms(200);
    Uart2Send(0xA5); Uart2Send(0x01); Uart2Send(0x0A); Uart2Send(0x0C); Uart2Send(0xDD); 
    
    Speech("欢迎来到互联网交通");

    while(1)
    {    
        Init1650();          

        // --- 核心业务：自动放行倒计时处理 ---
        if(start_countdown)
        {
            if(station_timer >= 250) // 250次 * 20ms = 5000ms = 5秒时间到
            {
                start_countdown = 0; 
                station_timer = 0;
                
                // 1. 无线发送单字节放行码给小车（小车收到 0x01 退出停顿）
                Uart2Send(0xA1); 
							 Uart2Send(0x01); Uart2Send(0xDD);
                
                Speech("小车放行");
                current_station = 0; // 清空站号信息
                Mode = SavedMode;    // 完美恢复到进入到站模式前的日间(0)或夜间(1)状态
            }
        }

        // 【按键K1：切换至日间模式】
        if (k1 == 0)
        {
            delayms(10);     
            if (k1 == 0)
            {
                start_countdown = 0; // 强行切模式时复位小车到站状态
                current_station = 0;
                Mode = 0;    
                SavedMode = 0;
                Servo1PwmDat = 1700; // 抬杆放行
                Speech("系统已切换至日间模式");
                
                // 发送日间指令：绿灯10秒(0x0A)，红灯12秒(0x0C)
                Uart2Send(0xA5); 
                Uart2Send(0x01); 
                Uart2Send(0x0A); 
                Uart2Send(0x0C); 
                Uart2Send(0xDD); 
                
                while(k1 == 0); 
            }
        }

        // 【按键K2：切换至夜间模式】
        if (k2 == 0)
        {
            delayms(10);     
            if (k2 == 0)
            {
                start_countdown = 0; // 强行切模式时复位小车到站状态
                current_station = 0;
                Mode = 1;    
                SavedMode = 1;
                Servo1PwmDat = 810;  // 落杆拦截
                Speech("系统已切换至夜间模式");
                
                // 发送夜间指令给红绿灯板
                Uart2Send(0xA5); 
                Uart2Send(0x02); 
                Uart2Send(0x00); 
                Uart2Send(0x00); 
                Uart2Send(0xDD); 
                
                while(k2 == 0); 
            }
        }

        // 刷新主控板本身的数码管显示 (常态000，到站00X)
        RefreshDisplay();

        led_count++;
        if(led_count==500) { led_count=0; led=~led; }        
    }
}

// 定时器0中断：驱动舵机输出，并提供系统心跳基准
void Timer0_isr() interrupt 1 
{
    static uint i = 1;            
    switch(i)
    {
        case 1: PWM1 = 1; Timer0(Servo1PwmDat); break;      
        case 2: PWM1 = 0; Timer0(20000-Servo1PwmDat); i = 0; break;   
    }
    i++;

    // 状态机每轮询完一次完整的高低电平（即经过20ms）
    if(i == 2)
    {
        if(start_countdown)
        {
            station_timer++; 
        }
    }
}    

// 串口2中断：无缝接收小车无线站点ID
void Uart2Isr() interrupt 8
{
    uchar rx_car;
    if (S2CON & 0x02) { S2CON &= ~0x02; busy = 0; } 
    
    if (S2CON & 0x01) 
    { 
        S2CON &= ~0x01; 
        rx_car = S2BUF;
        
        // 核心修正：取消 Mode != 1 限制，不管是日间还是夜间模式，只要收到 1~5 号站一律响应
        if(rx_car >= 1 && rx_car <= 5)
        {
            // 如果当前不在倒计时状态，记录下当前的模式(日间0或夜间1)，以便5秒后能够原样恢复
            if(Mode != 2)
            {
                SavedMode = Mode; 
            }
            
            current_station = rx_car; // 锁定小车报站ID
            station_timer = 0;        // 倒计时计数器清零
            start_countdown = 1;      // 开启放行倒计时
            Mode = 2;                 // 强制切换为主控板数码管到站显示状态 (00X)
        }
    } 
}

// 串口4中断：语音模块驱动
void Uart4Isr() interrupt 18 
{ 
    if (S4CON & 0x02) { S4CON &= ~0x02; busy = 0; } 
    if (S4CON & 0x01) { S4CON &= ~0x01; } 
}