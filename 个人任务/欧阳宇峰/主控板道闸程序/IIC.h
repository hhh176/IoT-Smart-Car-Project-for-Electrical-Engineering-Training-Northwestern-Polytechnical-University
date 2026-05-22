#ifndef _IIC_H_
#define _IIC_H_

#include "STC8H.h"
//#include "delay.h"
#include <intrins.h>

#define uchar unsigned char

void OpenIIC();
void CloseIIC();
uchar IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_NAck(void);
void IICsendByte(unsigned char txd);
uchar IICReadByte(unsigned char ack);

#endif

