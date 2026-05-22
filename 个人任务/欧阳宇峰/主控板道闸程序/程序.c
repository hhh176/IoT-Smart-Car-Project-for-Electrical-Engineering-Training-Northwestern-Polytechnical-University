/*单片机晶振频率选用11.0592MHZ*/
#include   "STC8H.h"               //单片机型号stc8h3k64s4
#include   "intrins.h"          //
#include   "tm1650.h"
#define     uchar  unsigned char  //                       
#define     uint   unsigned int   //
/////////////////////////////////////////////////////语音播报相关定义
#define HEADLEN       5  //数据包头的长度
#define BKM_OFFSET    4  //背景音乐命令偏移
#define LEN_OFFSET    2  //长度字节的偏移量（一般不会超过255字节，因此只使用1字节长度）
sbit    BUSY = P1^2;     //开发板上SYN6288的BUSY引脚固定连接
uchar   nBkm = 0x00;
uchar   code head[HEADLEN] = {0xfd,0x00,0x00,0x01,0x00};//
uchar   code shu[10] = {0,1,2,3,4,5,6,7,8,9};
uchar   code yingliang[11]={0xFD, 0x00, 0x08, 0x01, 0x01, 0x5B, 0x76, 0x31, 0x32, 0x5D, 0x86 };
//////////////////////////////////////////////////////////
bit    busy;    
//////////////////////////////////IO名称定义    
sbit   PWM1 = P2^5;             //舵机1输出定义
sbit   PWM2 = P2^6;             //舵机2输出定义
sbit  IR_IN = P3^2;             //红外传感器输入定义  检测到物体输出低电平
sbit    IN2 = P3^3;             //输入2定义
sbit    IN3 = P3^4;             //输入3定义
sbit    IN4 = P3^5;             //输入4定义
sbit     RR = P2^0;             //交通灯红灯定义      低电平点亮        
sbit     YY = P2^1;             //交通灯黄灯定义      低电平点亮 
sbit     GG = P2^2;             //交通灯绿灯定义      低电平点亮 
sbit    led = P5^5;             //指示灯IO定义，可以用于调试
sbit    k1  = P3^7;             //按键k1IO定义
sbit    k2  = P3^6;             //按键k2IO定义
//////////////////////////////////变量定义
uint   Servo1PwmDat = 810;      //舵机关闭=(1600~1750) 打开=(700~850) 单片机时钟对应角度，500~2500对应0-180度
uint   Servo1PwmDat_Open = 750; //舵机抬杆角度PWM值（对应约30度，可根据实际调整）
uint   Servo1PwmDat_Close = 1700;//舵机落杆角度PWM值（对应约90度，可根据实际调整）

#define GATE_TIMER_COUNT 300   //门杆保持计时（单位：10ms），100*10ms=1s，可改小以加速落杆

uchar  PC_dat;                  //接收到电脑发送的存储数据 
uchar  table[4];               //串口2接收数据缓存数组
uchar  dat;                    //串口2计数
uint   sum;                     //计数总和
bit    flag1,flag2,flag3,flag4; //功能标志位
bit    IR_Car_Detect = 0;       //车辆检测标志位
bit    Servo_Status = 0;        //舵机状态：0-落杆 1-抬杆
uint   IR_Delay_Count = 0;      //红外检测消抖计数
uint   Gate_Timer = 0;          //门杆开关计时（单位：10ms）
uchar  Gate_State = 0;          //门杆状态机：0=落杆待触发，1=抬杆计时，2=落杆后复检
//////////////////////////////////延时子程               
void delayms(uint ms)
{
    uint i,j;
    for(i=0;i<85;i++)        
    for(j=0;j<ms;j++);
}
//////////////////////////////////串口发送子程序（单片机发送十六进制数给电脑）
void uart_fa(uchar dat)
{
   SBUF = dat ;
   while(!TI);
   TI = 0;
}
////////////////////////////////无线通讯串口2发送程序
void Uart2Send(uchar dat)
{
    while (busy);
    busy = 1;
    S2BUF = dat;
}
////////////////////////////////备用串口3发送程序    
void Uart3Send(uchar dat)
{
    while (busy);
    busy = 1;
    S3BUF = dat;
}
////////////////////////////////语音播报串口4发送程序    
void Uart4Send(uchar dat)
{
    while (busy);
    busy = 1;
    S4BUF = dat;
}
//////////////////////////////////语音播报程序 
void Speech(uchar *buf)
{
     uchar i = 0;          //循环计数变量
     uchar xor = 0x00;     //校验码初始化
     uchar ch = 0x00;
   uchar len = 0x00;
   while(buf[len++]);
    //发送数据包头(0xFD + 2字节长度 + 1字节命令字 + 1字节命令参数)
  for(i = 0; i < HEADLEN; i++)
      {
          if(i == BKM_OFFSET)
          ch = nBkm << 3; //写入背景音乐
          else if(i == LEN_OFFSET)
          ch = len + 3;
          else
          ch = head[i];
          xor ^= ch;
          Uart4Send(ch);
    }
    //发送文字内容
    for(i = 0; i < len; i++)
    {
        xor ^= buf[i];
        Uart4Send(buf[i]);
    }
    Uart4Send(xor);         //发送校验位
	 delayms(20);
	 while(BUSY);            //等待语音播报完成（保留兼容阻塞接口）
	 delayms(20);
}

// 异步语音播放支持
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
    // 计算字符串长度（与同步函数一致的方式）
    while(buf[len++]);
    // 构造包头和内容到 speechBuf
    if(len + HEADLEN + 1 > SPEECH_BUF_SIZE) return; //超长则放弃
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
    // 复制文字内容
    for(i = 0; i < len; i++)
    {
        speechBuf[HEADLEN + i] = buf[i];
        xor ^= buf[i];
    }
    speechBuf[HEADLEN + len] = xor; //校验位
    speechLen = HEADLEN + len + 1;
    speechPos = 0;
    speechSending = 1;
    // 如果当前总线空闲，先发第一个字节，否则ISR会继续发送
    if(!busy && speechPos < speechLen)
    {
        busy = 1;
        S4BUF = speechBuf[speechPos++];
    }
}
///////////////////////////////////////////////舵机使用定时器0计数器赋值产生定时中断
void Timer0(uint pwm)           //11.0592Mhz晶振，12分频，所以计数器每递增一个数就是接近1微秒，
{
   uint value;
   value=65535-pwm;       
   TR0 = 0;                     //关闭定时器T0
   TL0=value;                   //16位数据给8位数据赋值默认将16位数据的低八位直接赋给八位数据
   TH0=value>>8;                //将16位数据右移8位，也就是将高8位移到低八位，再赋值给8位数据       
   TR0 = 1;                     //启动定时器T0
}
/////////////////////////////////舵机控制函数
void Servo_Control(uint pwm_val)
{
    Servo1PwmDat = pwm_val; //更新舵机PWM值
    Timer0(Servo1PwmDat);   //设置定时器0参数
}

/////////////////////////////////红外检测消抖处理
void IR_Detect_Process()
{
    //红外传感器低电平表示检测到车辆（消抖：连续检测100ms）
    if(IR_IN == 0)
    {
        IR_Delay_Count++;
        if(IR_Delay_Count >= 10) //10*10ms=100ms（Timer1中断10ms一次）
        {
            IR_Car_Detect = 1;    //标记检测到车辆
            IR_Delay_Count = 0;
        }
    }
    else
    {
        IR_Car_Detect = 0;
        IR_Delay_Count = 0;
    }
}
/////////////////////////////////IO口、定时器、串口通讯波特率初始化
void init()  
{
     //////////////////////////////////////////IO初始化
   P0M1 = 0x00;   P0M0 = 0x00;   //设置为准双向口
   P1M1 = 0x00;   P1M0 = 0x00;   //设置为准双向口
   P2M1 = 0x00;   P2M0 = 0x00;   //设置为准双向口
   P3M1 = 0x00;   P3M0 = 0xc0;   //设置为准双向口
   P4M1 = 0x00;   P4M0 = 0x00;   //设置为准双向口
   P5M1 = 0x00;   P5M0 = 0x00;   //设置为准双向口
     /////////////////////////////////////////舵机使用定时器T0初始
   TMOD &= 0xF0;                 //设置定时器模式
   TMOD |= 0x01;                 //设置定时器模式
   TL0 = 0x00;                   //设置定时初值
   TH0 = 0x00;                   //设置定时初值
   TF0 = 0;                      //清除TF0标志
   TR0 = 1;                      //定时器0开始计时
   ET0 = 1;                      //开定时器0中断
     /////////////////////////////////////////定时器T1   10毫秒@11.0592MHz
     AUXR &= 0x7F;                     //定时器时钟12T模式
     TMOD &= 0xF0;                     //设置定时器模式
     TL1 = 0x00;                       //设置定时初值
     TH1 = 0xDC;                      //设置定时初值
     ET1 = 1;                          //允许定时器T1产生中断
     TR1 = 1;                          //定时器1开始计时
     ///////////////////////////////定时器T2 串口通讯设置波特率9600bps@11.0592MHz
     SCON = 0x50;                       //8位数据,可变波特率
     S2CON = 0x10;
     S4CON = 0x10;
     AUXR |= 0x01;                     //串口1选择定时器2为波特率发生器
     AUXR &= 0xFB;                     //定时器时钟12T模式
     T2L = 0xE8;                       //设置定时初始值
     T2H = 0xFF;                       //设置定时初始值
     AUXR |= 0x10;                     //定时器2开始计时
     busy = 0;
   /////////////////////////////////////////     
     IE2 = 0x11;          //开串口2,串口4中断
     ES = 1;              //开串口1中断
     EA = 1;              //打开总中断
}

void display()                                   //数码管显示子程序
 { 
     Set1650(0x68,tab[sum/100]);          //数码管显示百位 
     Set1650(0x6a,tab[sum%100/10]);          //数码管显示十位
     Set1650(0x6c,tab[sum%10]);           //数码管显示个位 
 }
//////////////////////////////////////////////主程序
void main()
{    
   uint a;
  
    init();                //调用初始化程序
        
     Speech_Start("欢迎来到互联网交通");
    while(1)
    {    
      //***************************************************//请不要删除
        Init1650();                           //tm1650调用亮度调节
        display();                            //数码管显示子程序
      ///////////////////////////////////单片机运行指示灯
        a++;
        if(a==500){a=0;led=~led;}        
      //***************************************************//     
    //////////////////////////////////完成你们的任务
        IR_Detect_Process(); //红外消抖处理
    
        if(Gate_State == 0 && IR_Car_Detect == 1)
        {
            Servo_Control(Servo1PwmDat_Close); //舵机落杆
            Servo_Status = 1;                //标记为抬杆状态
            Gate_State = 1;                  //进入抬杆计时状态
            Gate_Timer = GATE_TIMER_COUNT;  //倒计时（GATE_TIMER_COUNT*10ms），可调以改变抬/落保持时间
            Speech_Start("欢迎再次来到停车场");      //语音播报（异步）
        }
        else if(Gate_State == 1 && Gate_Timer == 0)
        {
            Servo_Control(Servo1PwmDat_Open); //舵机抬杆
            Servo_Status = 0;                 //标记为落杆状态
            Gate_State = 0;                   //落杆后直接回到待触发状态
        }
    }
}

//////////////////////////////////////////////定时器T0中断处理舵机
void Timer0_isr() interrupt 1 
{
   static uint i = 1;            //静态变量：每次调用函数时保持上一次所赋的值    
   switch(i)
     {
        case 1: PWM1 = 1;           //PWM1控制舵机脚高电平输出                
        Timer0(Servo1PwmDat); break;//给定时器0赋值，计数Pwm1Dat个脉冲后产生中断，下次中断会进入下一个case语句      
          case 2: PWM1 = 0;           //PWM控制舵机脚低电平               
        Timer0(20000-Servo1PwmDat); i = 0; break;   
//高脉冲结束后剩下的时间(20000-Pwm1Dat)全是低电平了，Pwm1Dat + (20000-Pwm1Dat) = 20000个脉冲正好为一个周期20毫秒                                            
     }
   i++;
}   
//////////////////////////////////////////////定时器T1中断处理，每10ms进入一次
void Timer1_isr() interrupt 3
{
     static char t1,t2;
     t1++;t2++;
     if(flag1==1)                             //flag1=1时数码管开始1秒计数
      {
          if(t1==100)                           //10ms*100=1s
           {
               t1 = 0;                            //清零
               sum++;
               if(sum==999){sum=0;} 
           }
      }
        else{ t1 = 0; }
     ////////////////////////////////////////门杆计时器
     if(Gate_Timer > 0)
     {
         Gate_Timer--;
     }
}
/////////////////////////////////////无线通讯接收串口2中断
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
        table[dat++] = S2BUF;         //小车发过来的多个数据存到数组里
        if(SBUF==0xDD){dat = 0;}     //接收到结束码0xDD表示数据已接收完成，dat清零为下次接收做准备。
                                             //如接收到：0xA1  0x01  0xDD。0xA1表示地址码区分多台设备，0x01表示数据码，0xDD表示结束码       
    }
}
/////////////////////////////////////备用串口3中断
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
/////////////////////////////////
void Uart4Isr() interrupt 18     //串口4中断语音播报用
{
    if (S4CON & 0x02)
    {
        S4CON &= ~0x02;
        // 传输完成中断：如果有异步语音数据则继续发送下一字节
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
