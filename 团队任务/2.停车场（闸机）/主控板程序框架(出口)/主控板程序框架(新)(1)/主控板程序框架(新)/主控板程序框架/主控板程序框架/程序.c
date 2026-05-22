/* 单片机时钟频率选择11.0592MHZ */
#include   "STC8H.h"               // 单片机型号stc8h3k64s4
#include   "intrins.h"
#include   "tm1650.h"
#define     uchar  unsigned char
#define     uint   unsigned int

/* ======================================================== */
/*             系统工作模式: EXIT=出口                        */
/* ======================================================== */
#define  MODE_EXIT     1        // 出口模式

/* ======================================================== */
/*             语音文本定义                                   */
/* ======================================================== */
#if  defined(MODE_EXIT)
   uchar code voice_bye[] = "欢迎再次来到好老师最好停车场";
#endif

/* ======================================================== */
/*             语音模块 SYN6288 相关定义                       */
/* ======================================================== */
#define HEADLEN       5  // 数据包头的长度
#define BKM_OFFSET    4  // 背景音乐命令偏移
#define LEN_OFFSET    2  // 长度字节的偏移量
sbit    BUSY = P1^2;     // 语音芯片SYN6288的BUSY信号固定接
uchar   nBkm = 0x00;
uchar   code head[HEADLEN] = {0xfd,0x00,0x00,0x01,0x00};

/* ======================================================== */
/*             IO口定义                                       */
/* ======================================================== */
bit    busy;
sbit   PWM1 = P2^5;             // 舵机1PWM输出
sbit   PWM2 = P2^6;             // 舵机2PWM输出
sbit  IR_IN = P3^2;             // 红外传感器输入定义  检测到车辆输出低电平
sbit    IN2 = P3^3;             // 电机2控制
sbit    IN3 = P3^4;             // 电机3控制
sbit    IN4 = P3^5;             // 电机4控制
sbit    led = P5^5;             // 指示灯IO定义,用于在调试
sbit    k1  = P3^7;             // 按键k1IO定义(抬杆)
sbit    k2  = P3^6;             // 按键k2IO定义(备用)

/* ======================================================== */
/*             舵机参数                                       */
/* ======================================================== */
uint   Servo1PwmDat = 810;      // 舵机关闭=(1600~1750) 开=(700~850)
uint   Servo1PwmDat_Open = 750; // 舵机抬杆角度PWM值(对应约30度)
uint   Servo1PwmDat_Close = 1700;// 舵机落杆角度PWM值(对应约90度)
uint   ServoTarget = 810;       // 舵机目标位置

/* ======================================================== */
/*             道闸状态机定义                                 */
/* ======================================================== */
#define  STATE_IDLE        0   // 空闲, 等待红外检测或无线指令
#define  STATE_OPENING     1   // 正在抬杆
#define  STATE_OPEN        2   // 道闸已抬起, 等待车辆通过
#define  STATE_CLOSING     3   // 正在落杆
uchar  gate_state = STATE_IDLE;

/* ======================================================== */
/*             全局变量                                       */
/* ======================================================== */
uchar  PC_dat;                  // 接收电脑发送的存储变量
uchar  table[4];                // 串口2接收数据缓冲
uchar  dat;                     // 串口2计数
uint   sum;                     // 停车计时(秒)
bit    flag1,flag2,flag3,flag4; // 各功能标志位 (flag1: 计时使能)
bit    IR_Car_Detect = 0;       // 车辆检测标志位
uint   IR_Delay_Count = 0;      // 红外延时去抖计数
uint   Gate_Timer = 0;          // 门杆保持计时(单位:10ms)
bit    flag_need_speech;        // 需要播放语音

#define  GATE_HOLD_TIME   300   // 抬杆保持时间 300*10ms = 3秒
#define  CLOSE_DELAY_MS    30   // 车辆通过后落杆延时 30*10ms = 300ms

/* ======================================================== */
/*             延时函数                                       */
/* ======================================================== */
void delayms(uint ms)
{
    uint i,j;
    for(i=0;i<85;i++)
    for(j=0;j<ms;j++);
}

/* ======================================================== */
/*             串口发送函数                                    */
/* ======================================================== */
void uart_fa(uchar dat)
{
   SBUF = dat ;
   while(!TI);
   TI = 0;
}

void Uart2Send(uchar dat)
{
    while (busy);
    busy = 1;
    S2BUF = dat;
}

void Uart3Send(uchar dat)
{
    while (busy);
    busy = 1;
    S3BUF = dat;
}

void Uart4Send(uchar dat)
{
    while (busy);
    busy = 1;
    S4BUF = dat;
}

/* ======================================================== */
/*             语音播放函数                                    */
/* ======================================================== */
void Speech(uchar *buf)
{
     uchar i = 0;
     uchar xor = 0x00;
     uchar ch = 0x00;
   uchar len = 0x00;
   while(buf[len++]);
  for(i = 0; i < HEADLEN; i++)
      {
          if(i == BKM_OFFSET)
          ch = nBkm << 3;
          else if(i == LEN_OFFSET)
          ch = len + 3;
          else
          ch = head[i];
          xor ^= ch;
          Uart4Send(ch);
    }
    for(i = 0; i < len; i++)
    {
        xor ^= buf[i];
        Uart4Send(buf[i]);
    }
    Uart4Send(xor);
	 delayms(20);
	 while(BUSY);
	 delayms(20);
}

// 异步语音发送支持
#define SPEECH_BUF_SIZE 64
uchar xdata speechBuf[SPEECH_BUF_SIZE];
uint  speechLen = 0;
uint  speechPos = 0;
bit   speechSending = 0;

void Speech_Start(uchar *buf)
{
    uchar i = 0;
    uchar xor = 0x00;
    uchar ch = 0x00;
    uchar len = 0x00;
    while(buf[len++]);
    if(len + HEADLEN + 1 > SPEECH_BUF_SIZE) return;
    for(i = 0; i < HEADLEN; i++)
    {
        if(i == BKM_OFFSET)
            ch = nBkm << 3;
        else if(i == LEN_OFFSET)
            ch = len + 3;
        else
            ch = head[i];
        speechBuf[i] = ch;
        xor ^= ch;
    }
    for(i = 0; i < len; i++)
    {
        speechBuf[HEADLEN + i] = buf[i];
        xor ^= buf[i];
    }
    speechBuf[HEADLEN + len] = xor;
    speechLen = HEADLEN + len + 1;
    speechPos = 0;
    speechSending = 1;
    if(!busy && speechPos < speechLen)
    {
        busy = 1;
        S4BUF = speechBuf[speechPos++];
    }
}

/* ======================================================== */
/*             道闸控制函数                                    */
/* ======================================================== */
void GateOpen(void)
{
    ServoTarget = Servo1PwmDat_Close;  // 抬杆
    gate_state = STATE_OPENING;
}

void GateClose(void)
{
    ServoTarget = Servo1PwmDat_Open;   // 落杆
    gate_state = STATE_CLOSING;
}

/* ======================================================== */
/*             定时器0 PWM值更新                               */
/* ======================================================== */
void Timer0(uint pwm)
{
   uint value;
   value=65535-pwm;
   TR0 = 0;
   TL0=value;
   TH0=value>>8;
   TR0 = 1;
}

/* ======================================================== */
/*             红外传感器检测函数                              */
/* ======================================================== */
void IR_Detect_Process(void)
{
    if(IR_IN == 0)
    {
        IR_Delay_Count++;
        if(IR_Delay_Count >= 10) // 10*10ms=100ms
        {
            IR_Car_Detect = 1;
            IR_Delay_Count = 0;
        }
    }
    else
    {
        IR_Car_Detect = 0;
        IR_Delay_Count = 0;
    }
}

/* ======================================================== */
/*             数码管显示函数 (显示停车计时)                    */
/* ======================================================== */
void display(void)
{
     Set1650(0x68,tab[sum/100]);        // 百位
     Set1650(0x6a,tab[sum%100/10]);     // 十位
     Set1650(0x6c,tab[sum%10]);         // 个位
}

/* ======================================================== */
/*             IO口、定时器、串口通讯、全局变量初始化            */
/* ======================================================== */
void init()
{
   P0M1 = 0x00;   P0M0 = 0x00;
   P1M1 = 0x00;   P1M0 = 0x00;
   P2M1 = 0x00;   P2M0 = 0x00;
   P3M1 = 0x00;   P3M0 = 0x00;  // 全部准双向口
   P4M1 = 0x00;   P4M0 = 0x00;
   P5M1 = 0x00;   P5M0 = 0x00;

   k1 = 1;   // 准双向口使能上拉
   k2 = 1;

   /* ---- Timer0: 舵机PWM ---- */
   TMOD &= 0xF0;
   TMOD |= 0x01;
   TL0 = 0x00;
   TH0 = 0x00;
   TF0 = 0;
   TR0 = 1;
   ET0 = 1;

   /* ---- Timer1: 10毫秒@11.0592MHz ---- */
   AUXR &= 0x7F;
   TMOD &= 0xF0;
   TL1 = 0x00;
   TH1 = 0xDC;
   ET1 = 1;
   TR1 = 1;

   /* ---- 串口通讯波特率9600bps@11.0592MHz ---- */
   SCON = 0x50;
   S2CON = 0x10;
   S3CON = 0x10;
   S4CON = 0x10;
   AUXR |= 0x01;
   AUXR &= 0xFB;
   T2L = 0xE8;
   T2H = 0xFF;
   AUXR |= 0x10;
   busy = 0;

   IE2 = 0x11;
   ES = 1;
   EA = 1;

   /* ---- 变量初始化 ---- */
   sum = 0;
   gate_state = STATE_IDLE;
   flag1 = 0;
   ServoTarget = Servo1PwmDat_Open;
}

/* ======================================================== */
/*             主函数                                         */
/* ======================================================== */
void main()
{
   uint a;

    init();
    Speech_Start("停车计时系统已启动");

    while(1)
    {
        Init1650();
        display();

        a++;
        if(a==500){a=0;led=~led;}

        /* ---- 红外检测函数 ---- */
        IR_Detect_Process();

        /* ---- 道闸状态机(全自动) ---- */
        switch(gate_state)
        {
            case STATE_IDLE:
                // 红外检测到车辆 → 自动抬杆
                if(IR_Car_Detect)
                {
                    GateOpen();
                }
                break;

            case STATE_OPENING:
                // 等待舵机到达抬杆位置
                if(Servo1PwmDat >= Servo1PwmDat_Close - 5)
                {
                    Servo1PwmDat = Servo1PwmDat_Close;
                    gate_state = STATE_OPEN;
                    Gate_Timer = GATE_HOLD_TIME;
                }
                break;

            case STATE_OPEN:
                // 车辆通过后自动落杆
                if(!IR_Car_Detect)
                {
                    // 车辆已离开红外, 缩短落杆等待时间
                    if(Gate_Timer > CLOSE_DELAY_MS)
                    {
                        Gate_Timer = CLOSE_DELAY_MS;
                    }
                }
                if(Gate_Timer > 0)
                {
                    Gate_Timer--;
                }
                if(Gate_Timer == 0)
                {
                    GateClose();
                }
                break;

            case STATE_CLOSING:
                // 舵机回到落杆位置
                if(Servo1PwmDat <= Servo1PwmDat_Open + 5)
                {
                    Servo1PwmDat = Servo1PwmDat_Open;
                    gate_state = STATE_IDLE;
                    // 停止计时
                    flag1 = 0;
                    // 播放语音
                    flag_need_speech = 1;
                }
                break;
        }

        /* ---- 语音播放函数(主循环执行) ---- */
        if(flag_need_speech)
        {
            flag_need_speech = 0;
            Speech_Start(voice_bye);
        }
    }
}

/* ======================================================== */
/*  定时器T0中断: 舵机PWM生成 (20ms周期)                       */
/* ======================================================== */
void Timer0_isr() interrupt 1
{
   static uint i = 1;
   switch(i)
     {
        case 1: PWM1 = 1;
        Timer0(Servo1PwmDat); break;
          case 2: PWM1 = 0;
        Timer0(20000-Servo1PwmDat); i = 0; break;
     }
   i++;
}

/* ======================================================== */
/*  定时器T1中断: 10ms时钟                                   */
/*  功能: 停车计时(秒累计)、舵机缓动                          */
/* ======================================================== */
void Timer1_isr() interrupt 3
{
     static char t1,t2;
     t1++;t2++;

     /* ---- 停车计时(flag1由无线指令控制) ---- */
     if(flag1==1)
      {
          if(t1==100)                           // 10ms*100=1s
           {
               t1 = 0;
               if(sum < 999) sum++;              // 停车时间累计(最大999秒)
           }
      }
        else{ t1 = 0; }

     /* ---- 舵机缓动 ---- */
     if(Servo1PwmDat < ServoTarget)
         Servo1PwmDat += 5;
     else if(Servo1PwmDat > ServoTarget)
         Servo1PwmDat -= 5;
}

/* ======================================================== */
/*  串口2中断: 无线通信接收                                   */
/*  指令格式: (0xA5, 0x01, 0xDD)                             */
/*  收到该指令后开始计时, 数码管显示停车累计时间(秒)           */
/* ======================================================== */
void Uart2Isr() interrupt 8
{
    if (S2CON & 0x02)
    {
        S2CON &= ~0x02;
        busy = 0;
    }
    if (S2CON & 0x01)
    {
        S2CON &= ~0x01;
        table[dat++] = S2BUF;
        if(S2BUF == 0xDD)
        {
            // 检查是否为开始计时指令: 0xA5, 0x01, 0xDD
            if(table[0] == 0xA5 && table[1] == 0x01)
            {
                flag1 = 1;           // 开始计时
                sum = 0;             // 计时清零
            }
            dat = 0;
        }
        else
        {
            if(dat >= 4) dat = 0;
        }
    }
}

/* ======================================================== */
/*  串口3中断: 备用                                           */
/* ======================================================== */
void Uart3Isr() interrupt 17
{
    if (S3CON & 0x02)
    {
        S3CON &= ~0x02;
        busy = 0;
    }
    if (S3CON & 0x01)
    {
        S3CON &= ~0x01;
    }
}

/* ======================================================== */
/*  串口4中断: 语音模块异步发送                                */
/* ======================================================== */
void Uart4Isr() interrupt 18
{
    if (S4CON & 0x02)
    {
        S4CON &= ~0x02;
        if(speechSending && speechPos < speechLen)
        {
            S4BUF = speechBuf[speechPos++];
            busy = 1;
        }
        else
        {
            busy = 0;
            speechSending = 0;
        }
    }
    if (S4CON & 0x01)
    {
        S4CON &= ~0x01;
    }
}