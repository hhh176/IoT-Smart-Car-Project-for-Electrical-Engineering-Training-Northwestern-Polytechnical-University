#include "stc15.h"
#include "EEPROM.h"
#include "intrins.h"

#define uchar  unsigned  char	  //宏定义
#define uint   unsigned  int	  //宏定义
#define ulong  unsigned  long   //宏定义


///////////////////////////////////////////////////////eeprom用于闹钟存储时间
void Disable_IAP()
 {
   IAP_CONTR = 0;
   IAP_CMD = 0;
   IAP_TRIG = 0;
   IAP_ADDRH = 0xFF;
   IAP_ADDRL = 0xFF;
 }
//--------------------------------------------------
//读取IAP数据并返回
//ADRES = 需要读出数据的地址
//调用：关闭IAP功能函数
//--------------------------------------------------

uchar Read_IAP(uint Adres)
{
 uchar  Value;
 IAP_CONTR = ENABLE_IAP;     //打开IAP功能
 IAP_CMD = ISP_IAP_BYTE_READ;//读IAP
 IAP_ADDRH = Adres >> 8;     //取高位地址
 IAP_ADDRL = Adres &0x00FF;  //取低位地址
 EA = 0;                     //关中断
 IAP_TRIG = 0x5a;            //触发寄存器
 IAP_TRIG = 0xa5;            //IAP触发启动
 _nop_();
 Value = IAP_DATA;       //读取字节数据
 EA = 1;
 Disable_IAP();          //关闭IAP功能
 return Value;
}
//--------------------------------------------------
//字节编程
//Value = 需要写进IAP内部的数据
//ADRES = 需要写入数据的地址0
//调用关闭IAP功能函数
//--------------------------------------------------

void Write_IAP(uchar Value3,uint Adres)
{
 IAP_CONTR = ENABLE_IAP;   //打开IAP功能
 IAP_CMD = ISP_IAP_BYTE_PROGRAM;//字节编程
 IAP_ADDRH = Adres>>8;   //取地址位
 IAP_ADDRL = Adres &0x00FF;
 IAP_DATA = Value3;    //写入数据
 EA = 0;
 IAP_TRIG = 0x5a;     //触发IAP功能
 IAP_TRIG = 0xa5;
 _nop_();
 EA = 1;
 Disable_IAP();     //关闭IAP功能
}

//--------------------------------------------------
//擦除扇区功能
//Sector = 需要擦除的扇区地址
//调用函数：关闭IAP功能函数
//--------------------------------------------------

void Sector_Erase_IAP(uint Sector)
  {
	 IAP_CONTR = ENABLE_IAP;
	 IAP_CMD = 0x03;
	 IAP_ADDRH = Sector >>8;
	 IAP_ADDRL = Sector & 0x00FF;
	 EA = 0;
	 IAP_TRIG = 0x5a;
	 IAP_TRIG = 0xa5;
	 _nop_();
	 EA = 1;
	 Disable_IAP();
  }