/* 红绿灯显示板：STC15W408AS，晶振频率11.0592MHZ */
/* 纯日间正常循环版（绿灯、红灯最后5秒均闪烁，黄灯正常闪烁） */
#include "stc15.h"           
#include "intrins.h"          

#define uchar unsigned char  
#define uint  unsigned int   

sbit D1 = P5^4;             // 十位数码管段选
sbit D2 = P5^5;             // 个位数码管段选
sbit RR = P3^2;             // 红灯控制，低电平亮        
sbit YY = P3^3;             // 黄灯控制，低电平亮 
sbit GG = P3^4;             // 绿灯控制，低电平亮 

uchar table[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90}; // 共阳段码

uchar sec;                  
uchar rsec=12, ysec=3, gsec=10;  

#define STATE_GREEN   0     
#define STATE_YELLOW  1     
#define STATE_RED     2     
uchar state;                

bit flag = 0;               // 1秒标志
bit half_flag = 0;          // 0.5秒标志
bit yel_on = 0;             // 闪烁交替辅助标志

void DelayMs(uint ms)
{
    uint i,j;
    for(i=0;i<85;i++)        
        for(j=0;j<ms;j++);
}

void init()  
{
    // STC15W 所有 I/O 口配置为准双向口
    P0M1 = 0x00; P0M0 = 0x00;   
    P1M1 = 0x00; P1M0 = 0x00;   
    P2M1 = 0x00; P2M0 = 0x00;   
    P3M1 = 0x00; P3M0 = 0x00;   
    P4M1 = 0x00; P4M0 = 0x00;   
    P5M1 = 0x00; P5M0 = 0x00;   

    // 定时器0定时10ms
    AUXR &= 0x7F;               
    TMOD &= 0xF0;               
    TL0 = 0x00; TH0 = 0xDC;     
    ET0 = 1; TR0 = 1;           
}

void display_day()
{
    D2 = 0; D1 = 1;             // 亮个位
    P1 = table[sec%10];          
    DelayMs(3); P1 = 0xFF;      // 消隐预防鬼影
    
    D2 = 1; D1 = 0;             // 亮十位
    P1 = table[sec/10];          
    DelayMs(3); P1 = 0xFF;      
}

void enter_day_mode()
{
    state = STATE_GREEN;          
    sec = gsec;                   
    GG = 0; YY = 1; RR = 1;     // 开机默认亮绿灯
    yel_on = 0;
}

void change_state()
{
    switch(state)
    {
        case STATE_GREEN:      
            state = STATE_YELLOW;
            sec = ysec;               
            GG = 1; YY = 0; RR = 1;     // 绿灯完换黄灯
            yel_on = 1;              
            break;
            
        case STATE_YELLOW:    
            state = STATE_RED;
            sec = rsec;               
            GG = 1; YY = 1; RR = 0;     // 黄灯完换红灯
            yel_on = 0;
            break;
            
        case STATE_RED:        
            state = STATE_GREEN;
            sec = gsec;               
            GG = 0; YY = 1; RR = 1;     // 红灯完换绿灯
            yel_on = 0;
            break;
    }
}

void main()
{   
    init();                     
    enter_day_mode();           
    
    EA = 1;                     
    
    while(1)
    {
        // 1. 刷新日间数码管显示
        display_day();            

        // 2. 0.5秒同步闪烁逻辑
        if(half_flag)
        {
            half_flag = 0;            
            yel_on = ~yel_on;

            if(state == STATE_YELLOW)
            {
                YY = yel_on ? 0 : 1;    // 黄灯状态下，0.5秒交替闪烁
            }
            else if(state == STATE_GREEN && sec <= 5)
            {
                GG = yel_on ? 0 : 1;    // 绿灯最后5秒，0.5秒交替闪烁
            }
            else if(state == STATE_RED && sec <= 5)
            {
                RR = yel_on ? 0 : 1;    // 红灯最后5秒，0.5秒交替闪烁
            }
        }
        
        // 3. 1秒状态机计时事件
        if(flag)
        {
            flag = 0;                 
            if(sec == 0) 
            {
                change_state();  
            }
        }
    }
}

// 定时器0中断服务：处理1秒递减与0.5秒闪烁基准
void Timer0_isr() interrupt 1 
{
    static uchar t100 = 0;            
    static uchar t50  = 0;            
    
    t100++; t50++;
    
    // 满1秒
    if(t100 >= 100)          
    {
        t100 = 0; flag = 1;          
        if(sec > 0) sec--;                     
    }

    // 满0.5秒
    if(t50 >= 50)            
    {
        t50 = 0; half_flag = 1;        
    }
}