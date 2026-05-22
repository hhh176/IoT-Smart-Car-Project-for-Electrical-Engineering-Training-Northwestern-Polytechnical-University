/* 红绿灯显示板：STC15W408AS，晶振频率11.0592MHZ */
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

uchar sum = 0;              // 串口接收索引
uchar pc_dat[6];            // 更改为固定6字节缓冲区（适配A5协议）
uchar sec;                  
uchar rsec=12, ysec=3, gsec=10;  

#define STATE_GREEN   0     
#define STATE_YELLOW  1     
#define STATE_RED     2     
uchar state;                

bit flag = 0;               // 1秒标志
bit half_flag = 0;          // 0.5秒标志
bit night_mode = 0;         // 夜间模式标志
bit yel_on = 0;             
bit cmd_ready = 0;          // 新增：一帧蓝牙数据接收完成标志

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

    // 定时器2作为串口波特率发生器 (9600bps)
    SCON = 0x50;                
    AUXR |= 0x01;               
    AUXR &= 0xFB;               
    T2L = 0xE8; T2H = 0xFF;     
    AUXR |= 0x10;               
    
    P_SW1 |= 0x40;              // 关键：串口1切换到 P3.6(RXD), P3.7(TXD) 连接蓝牙
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

void display_night()
{
    // 十位：显示一横 “-”
    D2 = 1; D1 = 0;             // 选中十位数码管
    P1 = 0xBF;                  // 共阳极 “-” 的段码是 0xBF
    DelayMs(3); P1 = 0xFF;      // 消隐
    
    // 个位：彻底不显示（全灭）
    D2 = 0; D1 = 1;             // 选中个位数码管
    P1 = 0xBF;                  // 0xFF 代表所有LED熄灭
    DelayMs(3); P1 = 0xFF;      // 消隐
}

void enter_night_mode()
{
    night_mode = 1;               
    GG = 1; RR = 1;             // 关绿、红
    YY = 0; yel_on = 1;         // 开黄灯开始闪烁
}

void enter_day_mode()
{
    night_mode = 0;               
    state = STATE_GREEN;          
    sec = gsec;                   
    GG = 0; YY = 1; RR = 1;     // 亮绿灯
    yel_on = 0;
}

void change_state()
{
    switch(state)
    {
        case STATE_GREEN:      
            state = STATE_YELLOW;
            sec = ysec;               
            GG = 1; YY = 0; RR = 1;     
            yel_on = 1;              
            break;
            
        case STATE_YELLOW:    
            state = STATE_RED;
            sec = rsec;               
            GG = 1; YY = 1; RR = 0;     
            yel_on = 0;
            break;
            
        case STATE_RED:        
            state = STATE_GREEN;
            sec = gsec;               
            GG = 0; YY = 1; RR = 1;     
            yel_on = 0;
            break;
    }
}

void process_cmd()
{
    uchar i;
    // 验证接收合法性
    if(pc_dat[0] == 0xA5 && pc_dat[4] == 0xDD)
    {
        switch(pc_dat[1])
        {
            case 0x01: // K1按下：日间模式，更新时间参数
                gsec = pc_dat[2];     
                rsec = pc_dat[3];     
                enter_day_mode();
                break;
                
            case 0x02: // K2按下：夜间模式
                enter_night_mode();
                break;
        }
    }
    
    // 释放命令锁，清空接收缓冲区
    for(i=0; i<6; i++) pc_dat[i] = 0;
    cmd_ready = 0; 
}

void main()
{   
    init();                          
    enter_day_mode(); // 默认开机进入日间正常循环
    
    ES = 1;                          
    EA = 1;                          
    
    while(1)
    {
        // 1. 刷新显示
        if(night_mode)
            display_night();          
        else
            display_day();            
        
        // 2. 蓝牙命令到达事件处理
        if(cmd_ready)
        {
            process_cmd();
        }

        // 3. 0.5秒黄灯异步闪烁事件
        if(half_flag)
        {
            half_flag = 0;            
            if(night_mode)
            {
                yel_on = ~yel_on;
                YY = yel_on ? 0 : 1;  
            }
            else if(state == STATE_YELLOW)
            {
                yel_on = ~yel_on;
                YY = yel_on ? 0 : 1;
            }
        }
        
        // 4. 1秒状态机计时事件
        if(flag)
        {
            flag = 0;                 
            if(!night_mode)           
            {
                if(sec == 0) change_state();   
            }
        }
    }
}

void Timer0_isr() interrupt 1 
{
    static uchar t100 = 0;            
    static uchar t50  = 0;            
    
    t100++; t50++;
    
    if(t100 >= 100)          
    {
        t100 = 0; flag = 1;          
        if(!night_mode && sec > 0) sec--;                    
    }
    if(t50 >= 50)            
    {
        t50 = 0; half_flag = 1;        
    }
}

void Uart_1() interrupt 4
{    
    if(RI)                  
    {                    
        RI = 0;             
        
        // 如果前一条命令还没被主循环处理完，则拒绝接收新数据防止被覆盖
        if(!cmd_ready) 
        {
            pc_dat[sum] = SBUF; 
            
            // 收到帧头协议 0xA5 时判定合规
            if(sum == 0 && pc_dat[0] != 0xA5)
            {
                sum = 0; // 过滤非包头杂散信号
            }
            // 完美接收完5个字节且末尾是结束符
            else if(sum == 4 && pc_dat[4] == 0xDD)
            {
                sum = 0;
                cmd_ready = 1; // 锁死接收，挂起命令给主循环处理
            }
            else
            {
                sum++;
                if(sum >= 6) sum = 0; // 强力防爆、防止越界崩溃
            }
        }
    } 
}