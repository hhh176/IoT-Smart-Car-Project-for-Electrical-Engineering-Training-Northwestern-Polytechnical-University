/*单片机STC15W408AS，晶振频率选择11.0592MHZ*/
#include   "stc15.h"            //包含stc15w单片机头文件
#include   "intrins.h"          //
#define     uchar  unsigned char  //
#define     uint   unsigned int   //
//////////////////////////////////IO口控制定义	
sbit     D1 = P5^4;             //控制十位数码管段选
sbit     D2 = P5^5;             //控制个位数码管段选
sbit     RR = P3^2;             //南通道红灯控制     低电平点亮        
sbit     YY = P3^3;             //南通道黄灯控制     低电平点亮 
sbit     GG = P3^4;             //南通道绿灯控制     低电平点亮 
///////////////////////////////////共阳极数码管显示段定义   P1口段选显示
uchar  table[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};//显示0-9段码(共阳)
//////////////////////////////////变量定义
uchar  sum;                         //串口接收计数
uchar  pc_dat[10];				    //串口接收数据存储数组
uchar  sec;                         //当前倒计时秒数
uchar  rsec=12,ysec=3,gsec=10;      //红灯12s、黄灯3s、绿灯10s

// 交通灯状态定义
#define  STATE_GREEN   0            //绿灯状态
#define  STATE_YELLOW  1            //黄灯状态
#define  STATE_RED     2            //红灯状态
uchar  state;                       //当前交通灯状态

bit    flag = 0;                    //1秒到标志位
bit    half_flag = 0;               //0.5秒到标志位（用于黄灯闪烁）
bit    night_mode = 0;              //夜间模式标志 0=日间正常循环 1=夜间黄灯常闪
bit    yel_on = 0;                  //黄灯当前亮灭状态（用于闪烁控制）

//////////////////////////////////延时函数               
void DelayMs(uint ms)
{
	uint i,j;
	for(i=0;i<85;i++)		 
		for(j=0;j<ms;j++);
}
//////////////////////////////////串口发送单字节（单片机向PC十六进制通信）
void uart_fa(uchar dat)
{
   SBUF = dat ;
	 while(!TI);
	 TI = 0;
}
/////////////////////////////////IO口、定时器、串口通信波特率初始化
void init()  
{
	 //////////////////////////////////////////IO口初始化
	 P0M1 = 0x00;   P0M0 = 0x00;   //设置为准双向口
   P1M1 = 0x00;   P1M0 = 0x00;   //设置为准双向口
   P2M1 = 0x00;   P2M0 = 0x00;   //设置为准双向口
   P3M1 = 0x00;   P3M0 = 0x00;   //设置为准双向口
   P4M1 = 0x00;   P4M0 = 0x00;   //设置为准双向口
   P5M1 = 0x00;   P5M0 = 0x00;   //设置为准双向口
	 /////////////////////////////////////////定时器T0   10ms@11.0592MHz
	 AUXR &= 0x7F;		               //定时器时钟12T模式
	 TMOD &= 0xF0;		               //设置定时器模式
	 TL0 = 0x00;		               //设置定时初值
	 TH0 = 0xDC;	  	               //设置定时初值
	 ET0 = 1;		                   //使能定时器T0中断
	 TR0 = 1;		                   //定时器0开始计时
	 ///////////////////////////////定时器T2 串口通信波特率9600bps@11.0592MHz
	 SCON = 0x50;		               //8位数据,可变波特率
	 AUXR |= 0x01;		               //串口1选择定时器2为波特率发生器
	 AUXR &= 0xFB;		               //定时器时钟12T模式
	 T2L = 0xE8;		               //设置定时初值
	 T2H = 0xFF;	 	               //设置定时初值
	 AUXR |= 0x10;		               //定时器2开始计时	 
   P_SW1 |= 0x40;                    //串口1切换到P3.6( RXD ) P3.7( TXD )
}
//////////////////////////////////数码管显示子程序（日间模式：倒计时数字）
void display_day()
{
	D2 = 0;D1 = 1;                //打开个位数码管关闭十位数码管
	P1=table[sec%10];             //显示个位数字
	DelayMs(3);                  
	
	D2 = 1;D1 = 0;                //关闭个位数码管打开十位数码管
	P1=table[sec/10];             //显示十位数字
	DelayMs(3);                  
}

//////////////////////////////////数码管显示子程序（夜间模式：显示"-."）
void display_night()
{
	// 个位显示小数点"."（共阳极：仅DP段亮，0x7F）
	D2 = 0;D1 = 1;
	P1 = 0x7F;                    //仅DP段点亮，显示小数点"."
	DelayMs(3);
	
	// 十位显示横杠"-"（共阳极：仅g段亮，0xBF）
	D2 = 1;D1 = 0;
	P1 = 0xBF;                    //仅g段点亮，显示横杠"-"
	DelayMs(3);
}

//////////////////////////////////进入夜间模式
void enter_night_mode()
{
	night_mode = 1;               //置夜间模式标志
	GG = 1;                       //关绿灯
	YY = 0;                       //开黄灯（初始亮）
	RR = 1;                       //关红灯
	yel_on = 1;                   //黄灯当前为亮
}

//////////////////////////////////进入日间模式
void enter_day_mode()
{
	night_mode = 0;               //清夜间模式标志
	state = STATE_GREEN;          //从绿灯状态开始
	sec = gsec;                   //加载绿灯倒计时
	GG = 0;                       //开绿灯（低电平点亮）
	YY = 1;                       //关黄灯
	RR = 1;                       //关红灯
	yel_on = 0;
}

//////////////////////////////////交通灯状态切换（日间模式用）
void change_state()
{
	switch(state)
	{
		case STATE_GREEN:             //绿灯结束→切换到黄灯
			state = STATE_YELLOW;
			sec = ysec;               //加载黄灯倒计时（3s）
			GG = 1;                   //关绿灯（高电平熄灭）
			YY = 0;                   //开黄灯（低电平点亮）
			RR = 1;                   //关红灯
			yel_on = 1;               //黄灯初始为亮
			break;
			
		case STATE_YELLOW:            //黄灯结束→切换到红灯
			state = STATE_RED;
			sec = rsec;               //加载红灯倒计时（12s）
			GG = 1;
			YY = 1;                   //关黄灯
			RR = 0;                   //开红灯
			yel_on = 0;
			break;
			
		case STATE_RED:               //红灯结束→切换到绿灯
			state = STATE_GREEN;
			sec = gsec;               //加载绿灯倒计时（10s）
			GG = 0;                   //开绿灯
			YY = 1;
			RR = 1;                   //关红灯
			yel_on = 0;
			break;
	}
}

//////////////////////////////////处理串口接收到的命令
// 命令协议（共两种）：
//   PC串口命令：0xA1(地址) 绿灯值 黄灯值 红灯值 0xDD(结束符)
//   无线调度台命令：0xA5 命令码 数据1 数据2 0xDD
//     命令码 0x01 = K1按下 → 日间模式（数据1=绿灯时长，数据2=红灯时长）
//     命令码 0x02 = K2按下 → 夜间模式
void process_cmd()
{
	uchar i;
	
	if(pc_dat[4] != 0xDD) return;    //未收到完整命令则退出
	
	// PC串口命令（设置各灯时长并强制日间模式）
	if(pc_dat[0] == 0xA1)
	{
		gsec = pc_dat[1];             //更新绿灯秒数
		ysec = pc_dat[2];             //更新黄灯秒数
		rsec = pc_dat[3];             //更新红灯秒数
		enter_day_mode();             //恢复日间正常循环
	}
	// 无线调度台命令
	else if(pc_dat[0] == 0xA5)
	{
		switch(pc_dat[1])
		{
			case 0x01:                //K1：日间模式，携带红绿灯时长
				gsec = pc_dat[2];     //绿灯时长
				rsec = pc_dat[3];     //红灯时长
				enter_day_mode();
				break;
				
			case 0x02:                //K2：夜间模式
				enter_night_mode();
				break;
		}
	}
	
	// 清除命令缓冲，准备接收下一条
	for(i=0;i<5;i++) pc_dat[i] = 0;
}

//////////////////////////////////////////////主函数
void main()
{ 	
	init();                          //调用初始化函数
	
	// 默认日间模式：绿灯亮起
	enter_day_mode();
	
	ES = 1;                          //开串口中断
	EA = 1;                          //开总中断
	
	while(1)
	{
		//========== 数码管显示 ==========
		if(night_mode)
			display_night();          //夜间模式显示"-."
		else
			display_day();            //日间模式显示倒计时数字
		
		//========== 0.5秒事件处理（黄灯闪烁）==========
		if(half_flag)
		{
			half_flag = 0;            //清除0.5秒标志
			
			if(night_mode)
			{
				// 夜间模式：黄灯每0.5s翻转一次（亮0.5s灭0.5s循环）
				yel_on = ~yel_on;
				YY = yel_on ? 0 : 1;  //低电平点亮
			}
			else if(state == STATE_YELLOW)
			{
				// 日间模式黄灯阶段：闪烁3次（3秒内每0.5s翻转，共3个亮灭周期）
				yel_on = ~yel_on;
				YY = yel_on ? 0 : 1;
			}
		}
		
		//========== 1秒事件处理 ==========
		if(flag)
		{
			flag = 0;                 //清除1秒标志
			
			if(!night_mode)           //日间模式才走状态机
			{
				if(sec == 0)          //当前状态倒计时结束
				{
					change_state();   //切换到下一个交通灯状态
				}
			}
			
			// 检测是否有完整串口命令待处理
			if(pc_dat[4] == 0xDD)
			{
				process_cmd();        //处理串口命令
			}
		}
	}
}

//////////////////////////////////////////////定时器T0中断处理（10ms一次）
void Timer0_isr() interrupt 1 
{
	static uchar t100 = 0;            //1秒计数器（100×10ms=1s）
	static uchar t50  = 0;            //0.5秒计数器（50×10ms=0.5s）
	
	t100++;
	t50++;
	
	if(t100 == 100)                   //每10ms中断一次，10ms*100=1s
	{
		t100 = 0;                     //清零1秒计数
		flag = 1;                     //置位1秒标志
		
		if(!night_mode && sec > 0)    //日间模式且未到0
			sec--;                    //倒计时减1
	}
	
	if(t50 == 50)                     //50×10ms=0.5s
	{
		t50 = 0;                      //清零0.5秒计数
		half_flag = 1;                //置位0.5秒标志
	}
}

///////////////////////////////////////////串口接收中断（接收PC/无线模块发来的十六进制指令）
void Uart_1() interrupt 4
{	 
	if(RI)			                 //等待接收指令是否接收完，接收完RI就置1
	{			      
		RI = 0;		                 //软件清零RI标志
		pc_dat[sum] = SBUF;          //将串口通信模块发来的指令存到pc_dat[]数组
		
		// 收到结束符0xDD时重置sum，为下次接收准备
		if(pc_dat[sum] == 0xDD)
		{
			sum = 0;                 //归零，等待主循环处理
		}
		else
		{
			sum++;
			if(sum >= 10) sum = 0;   //防止数组越界
		}
	} 
}