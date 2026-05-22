/*单片机晶振频率选用11.0592MHZ*/
#include "STC8H.h"		      //单片机型号stc8h3k64s4
#include "stdio.h"
#include "math.h"
#include "intrins.h"
#define  uchar unsigned char
#define  uint unsigned int
//////////////////////////////////////	
#define FOSC        11059200UL
#define BRT         (65536 - FOSC / 9600 / 4)
/////////////////////////////////////////////////////语音播报相关定义
#define HEADLEN       5  //数据包头的长度
#define BKM_OFFSET    4  //背景音乐命令偏移
#define LEN_OFFSET    2  //长度字节的偏移量（一般不会超过255字节，因此只使用1字节长度）
sbit  BUSY = P2^6;       //开发板上SYN6288的BUSY引脚固定连接到
uchar nBkm = 0x00;
uchar code head[HEADLEN] = {0xfd,0x00,0x00,0x01,0x00};//
/////////////////////////////////////////////////串口通讯定义
bit    busy;
uchar  wptr;
uchar  rptr;
uchar  buffer[23];		//IC接收存储数组
uchar  table[4];		//无线通讯接收存储数组
uchar  BT;  			//蓝牙
uchar  dat;             //无线通讯变量
/////////////////////////////////////左右电机接口
sbit  DJL1 = P1^2;	   //左电机IO定义
sbit  DJL2 = P1^6;	   //左电机IO定义
sbit  DJR1 = P2^2;	   //右电机IO定义
sbit  DJR2 = P5^5;	   //右电机IO定义
/////////////////////////////////////寻迹红外接口
sbit  HWL =  P2^3;	   //红外左边IO定义
sbit  HWZ =  P3^5;	   //红外中间IO定义
sbit  HWR =  P2^1;	   //红外右边IO定义
////////////////////////////////////
sbit  GM  =  P3^3;	   //光敏电阻IO定义 用来检测红绿灯的红灯
////////////////////////////////////
sbit  k1  =  P3^4;      //按键IO定义
////////////////////////////////////
sbit  led =  P5^4;     //指示灯IO定义，可以用于调试
///////////////////////////////////超声波发射接收IO定义
sbit  Trig = P2^0;     //超声波发射
sbit  Echo = P3^2;	   //超声波接收
uint  S;               //S存放超声波测距的值
///////////////////////////////////定义标志位
bit  flag1,flag2,flag3,flag4,flag5,flag6;
uchar lastRFID = 0;       //上一次播报过的RFID编号，防止同一标签连续重复播报
uchar traceDir = 0;       //循迹丢线时使用的上一次修正方向：0直行 1左 2右
#define RED_LIGHT_LEVEL 0 //红灯检测电平，若现场红灯检测相反，把0改成1即可
////////////////////////////////////延时程序
void delayms(uint xms)
{
  uchar j;uint i;
	for(i=xms;i>0;i--)
	for(j=110;j>0;j--);
}
///////////////////////////////////
void qianjin()     //小车前进
 {
    DJL1 = 0;
    DJL2 = 1;
    DJR1 = 1;
    DJR2 = 0;
 }
///////////////////////////////////
void houtui()     //小车后退
 {
    DJL1 = 1;
    DJL2 = 0;
    DJR1 = 0;
    DJR2 = 1;
 }
///////////////////////////////////
void zuozhuan()   //小车左转
 {
   	 DJL1 = 0;
     DJL2 = 0;
     DJR1 = 1;
     DJR2 = 0;
 }

///////////////////////////////////
void youzhuan()   //小车右转
 {
	   DJL1 = 0;
     DJL2 = 1;
     DJR1 = 0;
     DJR2 = 0;
 }
/////////////////////////////////////
void stop()       //小车停止
 {
    DJL1 = 0;
    DJL2 = 0;
    DJR1 = 0;
    DJR2 = 0;
 }
/////////////////////////////////////
bit IsRedLight()
{
    if(GM == RED_LIGHT_LEVEL)
    {
        delayms(20);
        if(GM == RED_LIGHT_LEVEL)
        {
            return 1;
        }
    }
    return 0;
}
/////////////////////////////////////
void FollowLine()  //三路红外循迹，黑线检测为0
{
    if(HWZ == 0 && HWL == 1 && HWR == 1)
    {
        traceDir = 0;
        qianjin();
    }
    else if(HWL == 0 && HWR == 1)
    {
        traceDir = 1;
        zuozhuan();
    }
    else if(HWR == 0 && HWL == 1)
    {
        traceDir = 2;
        youzhuan();
    }
    else if(HWL == 0 && HWZ == 0 && HWR == 0)
    {
        traceDir = 0;
        qianjin();
    }
    else
    {
        if(traceDir == 1)
        {
            zuozhuan();
        }
        else if(traceDir == 2)
        {
            youzhuan();
        }
        else
        {
            qianjin();
        }
    }
}
////////////////////////////////////定时器T0初始化 
void TM0_Init( )
{
	TMOD = 0x01;		        //设置定时器T0模式1
	TL0 = 0x00;		          //设置定时初值
	TH0 = 0x00;		          //设置定时初值
//	TR0 = 1;		          //定时器0开始计时
//	ET0 = 1;		   	      //允许定时器T0中断	
}	
////////////////////////////////////定时器T1定时10ms初始化
void Timer1_Init( )		//10毫秒@11.0592MHz
{
	AUXR &= 0xBF;			     //定时器时钟12T模式
	TMOD &= 0x0F;			     //设置定时器模式
	TL1 = 0x00;				     //设置定时初始值
	TH1 = 0xDC;				     //设置定时初始值
	TF1 = 0;				       //清除TF1标志
	TR1 = 0;				       //关闭定时器T1，需要的定时器T1，TR1 = 1; 
}

///////////////////////////////////////////////
void  Start() 		           //启动发射超声波脉冲
  {
	  Trig = 1;		             //启动一次模块
		_nop_();_nop_();
		_nop_();_nop_();
		_nop_();_nop_();
		_nop_();_nop_();
		_nop_();_nop_();
		_nop_();_nop_();
		_nop_();_nop_();
		_nop_();_nop_();
	  Trig = 0;
  }
////////////////////////////////////定时器3做串口3
void Uart3_Init()	               //波特率9600bps@11.0592MHz
{  
    S3CON = 0x50;
    T3L = BRT;
    T3H = BRT >> 8;
    T4T3M = 0x0a;
    busy = 0;	
 }
//////////////////////////////////定时器2做串口1、串口2、串口4
void UartInit()                   //波特率9600bps@11.0592MHz
{								   
    SCON  = 0x50;
	S2CON = 0x10;
	S4CON = 0x10;
    T2L = BRT;
    T2H = BRT >> 8;
    AUXR = 0x15;			            
    wptr = 0x00;
    busy = 0;
}
////////////////////////////////串口1 IC卡发送程序
void UartSend(uchar dat)
{
    while (busy);
    busy = 1;
    SBUF = dat;
}
////////////////////////////////串口2无线通讯模块发送程序
void Uart2Send(uchar dat)
{
    while (busy);
    busy = 1;
    S2BUF = dat;
}
////////////////////////////////串口3语音播报发送程序
void Uart3Send(uchar dat)
{
    while (busy);
    busy = 1;
    S3BUF = dat;
}
////////////////////////////////串口4蓝牙通讯发送程序    
void Uart4Send(uchar dat)
{
    while (busy);
    busy = 1;
    S4BUF = dat;
}
void Uart4SendStr(char *p)
{
    while (*p)
    {
      Uart4Send(*p++);
    }
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
		  Uart3Send(ch);
    }
	//发送文字内容
	for(i = 0; i < len; i++)
	{
		xor ^= buf[i];
		Uart3Send(buf[i]);
	}
	  Uart3Send(xor);         //发送校验位
	  delayms(50);
 	  while(BUSY);            //等待语音播报完成
      delayms(50);
}
//////////////////////////////////////////////RFID站点播报，按需要修改站名
void BroadcastStation(uchar id)
{
    switch(id)
    {
        case 1: Speech("一号站"); break;
        case 2: Speech("二号站"); break;
        case 3: Speech("三号站"); break;
        case 4: Speech("四号站"); break;
        case 5: Speech("五号站"); break;
        default: break;
    }
}
//////////////////////////////////////////////RFID标签检测
void CheckRFID()
{
    uchar id;
    id = buffer[5];
    if(id >= 1 && id <= 5)
    {
        if(id != lastRFID)
        {
            lastRFID = id;
            BroadcastStation(id);
        }
        buffer[5] = 0;
    }
}
//////////////////////////////////////////////红绿灯处理：红灯停车，绿灯继续循迹
void CheckTrafficLight()
{
    if(IsRedLight())
    {
        stop();
        while(IsRedLight())
        {
            stop();
            delayms(20);
        }
        delayms(100);
    }
}
//////////////////////////////////////////////初始程序
void init()
{
  TM0_Init( );                //定时器T0初始化
  Timer1_Init( );             //定时器T1初始化
  UartInit( );                //定时器2波特初始化
  Uart3_Init( );              //定时器3波特初始化
  IT0 = 0;                    //使能INT0上升沿和下降沿中断
  EX0 = 1;                    //使能INT0中断
//TR1 = 1;                    //启动定时器
//ET1 = 1;                    //使能定时器中断
  IE2 = 0x19;			      //开串口2、串口3、串口4		 
  ES  = 1;					  //开串口1
  EA  = 1;					  //打开总中断
  P_SW1 = 0x40;			   	  //设置串口1切换到P3.6和P3.7读IC卡信息
  P0M0 = 0xfc; P0M1 = 0x00;	  //1111 1100    	                 
  P1M0 = 0xfc; P1M1 = 0x00;   //1111 1100   
  P2M0 = 0x04; P2M1 = 0x00;   //0000 0100  
  P3M0 = 0x10; P3M1 = 0x00; 
  P5M0 = 0xff; P5M1 = 0x00; 
    
}
void main()
{
  uint a;
  a = 0;
  init();                    	//调用初始化
  //while(k1==1);               //按下k1键开始执行
  Speech("欢迎使用智能车");
  while(1)
  {
      ///////////////////////////////////单片机运行指示灯,请不要删除
      a++;
      if(a==50000){a=0;led=~led;}
      ///////////////////////////////////
      Start();                          //发送超声波信号
      TR0 = 1;                          //定时器T0开启计数

      CheckTrafficLight();              //红灯停，绿灯继续
      CheckRFID();                      //读到1-5号RFID标签后播报站名
      FollowLine();                     //三路红外循迹

      /////////////////////////////////超声波
      //if(S<5&&S>3)//检测前方有大于3CM小于5CM障碍停车并语音播报“停车”，无线发送0xA1,0x01,0xDD指令。
      //{
      //    stop();Speech("停车");Uart2Send(0xA1);Uart2Send(0x01);Uart2Send(0xDD);
      //}
      ////////////////////////////////蓝牙
      if(BT==7){BT=0; qianjin();}
   }
}
/////////////////////////////////////////////////外部中断T0超声波接收计算距离
void INT0_Isr() interrupt 0
{
   EX0 = 0;                                     //关闭外部中断T0
   TR0 = 0;	                                    //停止定时T0工作
   S = TH0*256+TL0;                             //把超声返回时间放到S里
   TH0 = 0;                                     //清除TH0
   TL0 = 0;                                     //清除TL0
   S = S*1.7/100;                               //算出来是CM存到S变量里
   EX0 = 1;                                     //打开外部中断
}
////////////////////////////////////////////////定时器T1中断每10ms进入一次
void TM1_Isr() interrupt 3
{
                                   
}
/////////////////////////////////////////////////IC读卡器串口1中断 
void UartIsr() interrupt 4    
{
    if (TI)
    {
      TI = 0;
      busy = 0;
    }
    if (RI)
    {
       RI = 0;
       buffer[wptr++] = SBUF;   //读到IC卡数据存到数组buffer[]里，我们只要判断buffer[5]是否等于1-5,
	     if(wptr>=22){wptr=0;}    //例如：if(buffer[5]==1){buffer[5]=0;Speech("西工大站");}记得buffer[5]=0;要清零。
    }
}
//////////////////////////////////////////////串口2无线通讯模块接收中断
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
        table[dat++] = S2BUF;        //主控板发过来的多个数据存到数组里
		if(S2BUF==0xDD){dat=0;}	 	 //接收到结束码0xDD表示数据已接收完成，dat清零为下次接收做准备。
				                     //如接收到：0xA1  0x01  0xDD。0xA1表示地址码区分试验台，0x01表示数据码，0xDD表示结束码  
        
    }
}

///////////////////////////////////////////////串口3中断语音播报用
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
///////////////////////////////////////////////串口4蓝牙接收中断
void Uart4Isr() interrupt 18
{
    if (S4CON & 0x02)
    {
        S4CON &= ~0x02;
        busy = 0;
    }
    if (S4CON & 0x01)
    {
        S4CON &= ~0x01;
		BT = S4BUF;               //把手机发送过来的数据存储到BT变量里
    }
}
