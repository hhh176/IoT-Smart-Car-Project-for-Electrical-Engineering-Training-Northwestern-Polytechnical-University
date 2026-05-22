/* ?????????11.0592MHZ */
#include "stc15.h"
#include "intrins.h"
#include "stdio.h"
#define  uchar unsigned char
#define  uint  unsigned int

/////////////////////////////////////////////////??????
uchar xdata BT = 0;             //??????
//////////////////////////////PWM?????????
uchar  SPEED_L = 10;   // ?????????
uchar  SPEED_R = 10;   // ?????????
uchar  t_count = 0;    // PWM????? 0~99
/////////////////////////// (??) ????
// ???????
sbit   PWM1_L = P1^2;
sbit   PWM2_L = P1^3;
// ???????
sbit   PWM1_R = P1^4;
sbit   PWM2_R = P1^5;
//?????????
uchar  DIR_L,DIR_R;
/////////////////////////////////////??????
sbit   HWL =  P1^6;	   //????IO??
sbit   HWZ =  P3^4;	   //????IO??
sbit   HWR =  P1^7;	   //????IO??
////////////////////////////////////
sbit   led =  P5^4;     //???IO??,??????
////////////////////////////////////????
void delayms(uint xms)
{
  uchar j;uint i;
	for(i=xms;i>0;i--)
	for(j=110;j>0;j--);
}
//////////////////////////////////???????(?????????????????)
void uart_fa(uchar dat)
{
   SBUF = dat ;
	 while(!TI);
	 TI = 0;
}
// ???putchar
char putchar(char c) {
    SBUF = c;
    while(!TI);
    TI = 0;
    return c;
}
//////////////////////////////////???T0???
void Timer0_Init( )		//100??@11.0592MHz
{
	AUXR &= 0x7F;			//?????12T??
	TMOD &= 0xF0;			//???????
	TL0   = 0xA4;				//???????
	TH0   = 0xFF;				//???????
	ET0   = 1;				  //??T0????
	TR0   = 0;				  //?????0
}
//////////////////////////////////////???T2???9600
void Uart1_Init(void)	//9600bps@11.0592MHz
{
	SCON = 0x50;		//8???,?????
	AUXR |= 0x01;		//??1?????2???????
	AUXR &= 0xFB;		//?????12T??
	T2L = 0xE8;			//???????
	T2H = 0xFF;			//???????
	AUXR |= 0x10;		//???2????
}
///////////////////////////////////////////////??
void Stop( )
{
	TR0    = 0;                     //?????T0
	PWM1_L = 0;
  PWM2_L = 0;
  PWM1_R = 0;
  PWM2_R = 0;
}
///////////////////////////////////////////////??
void Forward (uchar speed_L, uchar speed_R)
{
	TR0 =  1;   //????T0
	DIR_L= 1; SPEED_L = speed_L;
	DIR_R= 0; SPEED_R = speed_R;
}
///////////////////////////////////////////////??
void Backward (uchar speed_L, uchar speed_R)
{
	TR0 =  1;   //????T0
	DIR_L= 0; SPEED_L = speed_L;
	DIR_R= 1; SPEED_R = speed_R;
}
///////////////////////////////////////////////??
void Left (uchar speed_L, uchar speed_R)
{
	TR0 =  1;   //????T0
	DIR_L= 1; SPEED_L = speed_L;
	DIR_R= 1; SPEED_R = speed_R;
}
///////////////////////////////////////////////??
void Right (uchar speed_L, uchar speed_R)
{
	TR0 =  1;   //????T0
	DIR_L= 0; SPEED_L = speed_L;
	DIR_R= 0; SPEED_R = speed_R;
}
//////////////////////////////////////////////????
void init( )
{
	P1M0 = 0xFF; P1M1 = 0x00;
	P2M0 = 0x00; P2M1 = 0x00;
	P3M0 = 0x00; P3M1 = 0x00;
	P5M0 = 0x00; P5M1 = 0x00;
	Timer0_Init( );             //???T0???
	Uart1_Init( );              //???????
  ES    = 1;                  //????1??
  EA    = 1;					        //?????
	P_SW1 = 0x40;			   	      //????1???P3.6?P3.7????
}
void main()
{
  uint a;
  init();                   	//?????

	delayms(1000);
	printf("AT+BMHK\r\n");
	delayms(1000);
	printf("AT+BDHK\r\n");
	delayms(1000);

  while(1)
  {
		///////////////////////////////////????????
    a++;
    if(a==50000){a=0;led=~led;}

    //////////////////////////////////????
		if(BT==0x01)
		{
		  BT = 0;
			Forward (80,80);
		}
		if(BT==0x02)
		{
		  BT = 0;
			Stop( );
		}

		//////////////////////////////////??(???????)
		if(BT == 0x0C)
		{
			uchar left  = HWL;
			uchar mid   = HWZ;
			uchar right = HWR;

			if(left == 0 && mid == 0 && right == 0)
			{
				Stop();
			}
			else if(left == 1 && mid == 1 && right == 1)
			{
				Forward(75,75);
			}
			else if(left == 0 && mid == 1 && right == 0)
			{
				Forward(60,60);
			}
			else if(left == 1 && mid == 1 && right == 0)
			{
				Forward(55,70);
			}
			else if(left == 0 && mid == 1 && right == 1)
			{
				Forward(70,55);
			}
			else if(left == 1 && mid == 0 && right == 0)
			{
				Forward(45,80);
			}
			else if(left == 0 && mid == 0 && right == 1)
			{
				Forward(80,45);
			}
			else if(left == 1 && mid == 0 && right == 1)
			{
				Forward(60,60);
			}
		}
	}
}
//////////////////////////////////////////////// ???0,100us?????? - ??PWM??
void Timer0_ISR( ) interrupt 1
{
	t_count++;
  if(t_count >= 100){t_count = 0;}

////////////////////////////////////////// ??? PWM
	if(DIR_L==0)
	{
    PWM2_L = 1;
		if(t_count < SPEED_L)
    PWM1_L = 1;
    else
    PWM1_L = 0;
	}
	if(DIR_L==1)
	{
		PWM1_L = 1;
		if(t_count < SPEED_L)
    PWM2_L = 1;
    else
    PWM2_L = 0;
	}

////////////////////////////////////////////??? PWM
	if(DIR_R==0)
	{
    PWM2_R = 1;
		if(t_count < SPEED_R)
    PWM1_R = 1;
    else
    PWM1_R = 0;
	}
	if(DIR_R==1)
	{
		PWM1_R = 1;
		if(t_count < SPEED_R)
    PWM2_R = 1;
    else
    PWM2_R = 0;
	}
}
///////////////////////////////////////////??????
void Uart_1( ) interrupt 4
{
	if(RI)
	{
		RI = 0;
		BT = SBUF;
	}
}