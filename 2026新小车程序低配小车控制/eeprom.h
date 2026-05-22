#ifndef _EEPROM_H_
#define _EEPROM_H_

//#include

#define uint unsigned int
#define uchar  unsigned char

//----------------------------------------------------

//STC单片机IAP定义
#define ISP_IAP_BYTE_READ  1 //字节读
#define ISP_IAP_BYTE_PROGRAM 2 //字节编程
#define ISP_IAP_SECTOR_ERASE 3 //扇区擦除
#define WAIT_TIME    0 //等侍时间
#define ENABLE_IAP   0x81           //if SYSCLK<20MHz
//----------------------------------------------------

uchar Read_IAP(uint Adres);
void Write_IAP(uchar Value,uint Adres);
void Sector_Erase_IAP(uint Sector);
void DISAble_IAP(void);

#endif