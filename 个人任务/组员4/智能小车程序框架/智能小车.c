/*???????????????11.0592MHZ*/
#include "STC8H.h"
#include "stdio.h"
#include "math.h"
#include "intrins.h"
#define  uchar unsigned char
#define  uint unsigned int
#define FOSC        11059200UL
#define BRT         (65536 - FOSC / 9600 / 4)
#define HEADLEN       5
#define BKM_OFFSET    4
#define LEN_OFFSET    2
sbit  BUSY = P2^6;
uchar nBkm = 0x00;
uchar code head[HEADLEN] = {0xfd,0x00,0x00,0x01,0x00};
bit    busy;
uchar  wptr;
uchar  rptr;
uchar  buffer[23];
uchar  table[4];
uchar  BT;
uchar  dat;
sbit  DJL1 = P1^2;
sbit  DJL2 = P1^6;
sbit  DJR1 = P2^2;
sbit  DJR2 = P5^5;
sbit  HWL =  P2^3;
sbit  HWZ =  P3^5;
sbit  HWR =  P2^1;
sbit  GM  =  P3^3;
sbit  k1  =  P3^4;
sbit  led =  P5^4;
sbit  Trig = P2^0;
sbit  Echo = P3^2;
uint  S;
bit  flag1,flag2,flag3,flag4,flag5,flag6;

void delayms(uint xms)
{
  uchar j;uint i;
  for(i=xms;i>0;i--)
    for(j=110;j>0;j--);
}
void qianjin()
 {
    DJL1 = 0;
    DJL2 = 1;
    DJR1 = 1;
    DJR2 = 0;
 }
void houtui()
 {
    DJL1 = 1;
    DJL2 = 0;
    DJR1 = 0;
    DJR2 = 1;
 }
void zuozhuan()
 {
    DJL1 = 0;
    DJL2 = 0;
    DJR1 = 1;
    DJR2 = 0;
 }
void youzhuan()
 {
    DJL1 = 0;
    DJL2 = 1;
    DJR1 = 0;
    DJR2 = 0;
 }
void stop()
 {
    DJL1 = 0;
    DJL2 = 0;
    DJR1 = 0;
    DJR2 = 0;
 }
void TM0_Init( )
{
  TMOD = 0x01;
  TL0 = 0x00;
  TH0 = 0x00;
}
void Timer1_Init( )
{
  AUXR &= 0xBF;
  TMOD &= 0x0F;
  TL1 = 0x00;
  TH1 = 0xDC;
  TF1 = 0;
  TR1 = 0;
}
void  Start()
  {
    Trig = 1;
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
void Uart3_Init()
{
    S3CON = 0x50;
    T3L = BRT;
    T3H = BRT >> 8;
    T4T3M = 0x0a;
    busy = 0;
 }
void UartInit()
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
void UartSend(uchar dat)
{
    while (busy);
    busy = 1;
    SBUF = dat;
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
void Uart4SendStr(char *p)
{
    while (*p)
    {
      Uart4Send(*p++);
    }
}
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
      Uart3Send(ch);
    }
  for(i = 0; i < len; i++)
  {
    xor ^= buf[i];
    Uart3Send(buf[i]);
  }
    Uart3Send(xor);
    delayms(50);
      while(BUSY);
      delayms(50);
}
void init()
{
  TM0_Init( );
  Timer1_Init( );
  UartInit( );
  Uart3_Init( );
  IT0 = 0;
  EX0 = 1;
  TR1 = 1;
  ET1 = 1;
  IE2 = 0x19;
  ES  = 1;
  EA  = 1;
  P_SW1 = 0x40;
  P0M0 = 0xfc; P0M1 = 0x00;
  P1M0 = 0xfc; P1M1 = 0x00;
  P2M0 = 0x04; P2M1 = 0x00;
  P3M0 = 0x10; P3M1 = 0x00;
  P5M0 = 0xff; P5M1 = 0x00;
}
void main()
{
  uint a;
  init();
  while(1)
  {
      a++;
      if(a==50000){a=0;led=~led;}
      Start();
      TR0 = 1;
      if(buffer[5]==1)      {buffer[5]=0;Speech("第一站");}
      else if(buffer[5]==2) {buffer[5]=0;Speech("第二站");}
      else if(buffer[5]==3) {buffer[5]=0;Speech("第3站");}
      else if(buffer[5]==4) {buffer[5]=0;Speech("第四站");}
      else if(buffer[5]==5) {buffer[5]=0;Speech("第五站");}
      if(BT==7){BT=0; qianjin();}
   }
}
void INT0_Isr() interrupt 0
{
   EX0 = 0;
   TR0 = 0;
   S = TH0*256+TL0;
   TH0 = 0;
   TL0 = 0;
    S = S*1.7/100;
    EX0 = 1;
}
void TM1_Isr() interrupt 3
{
    if(GM==0)
    {
        stop();
    }
    else
    {
        if(HWL==1 && HWZ==0 && HWR==1)       { qianjin(); }
        else if(HWL==0 && HWZ==1 && HWR==1)  { zuozhuan(); }
        else if(HWL==1 && HWZ==1 && HWR==0)  { youzhuan(); }
        else if(HWL==0 && HWZ==0 && HWR==0)  { qianjin(); }
        else if(HWL==0 && HWZ==0 && HWR==1)  { zuozhuan(); }
        else if(HWL==1 && HWZ==0 && HWR==0)  { youzhuan(); }
        else                                 { qianjin(); }
    }
}
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
       buffer[wptr++] = SBUF;
       if(wptr>=22){wptr=0;}
    }
}
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
        if(S2BUF==0xDD){dat=0;}
    }
}
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
        BT = S4BUF;
    }
}
