/*--------------------------------------------------------------------------
TM1650.H
2位数码管TM1650 读写程序
--------------------------------------------------------------------------*/
#ifndef __TM1650_H__
#define __TM1650_H__

#include "intrins.h"
#define uchar unsigned char
#define uint  unsigned int
sbit CLK = P1^6;
sbit DIO = P1^7;
void Set1650(uchar add,uchar dat);

uchar ld = 1;          //亮度等级
uchar  tab[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};//0-F
                                // 0    1    2    3    4    5    6    7    8    9

void Init1650(){
     Set1650(0x48,(ld*16+0x01));//设定亮度，开显示八段显示方式，后一个字节中高位用于亮度，低位用于七段或八段显示方式和开关显
                                //如：0x71为七级亮度，八段显示方式，开显示；0x79为为七级亮度，七段显示方式，开显示
}

void Delay_us(uint i){ //us延时
        for(;i>0;i--){
                _nop_();
                _nop_();
                _nop_();
                _nop_();
                _nop_();
        }
}

void Start1650(void){//开始信号
        CLK = 1;
        DIO = 1;
        Delay_us(5);
        DIO = 0;
}

void Ask1650(void){ //ACK信号
        uchar timeout = 1;
        CLK = 1;
        Delay_us(5);
        CLK = 0;
        while((DIO)&&(timeout<=100)){
        timeout++;
        }
        Delay_us(5);
        CLK = 0;
}

void Stop1650(void){ //停止信号
        CLK = 1;
        DIO = 0;
        Delay_us(5);
        DIO = 1;
}

void WrByte1650(uchar oneByte){//写一个字节高位在前，低位在后
        uchar i;
        CLK = 0;
        Delay_us(1);
        for(i=0;i<8;i++){
            oneByte = oneByte<<1;
            DIO = CY;
            CLK = 0;
            Delay_us(5);
            CLK = 1;
            Delay_us(5);
            CLK = 0;
        }
}


void Set1650(uchar add,uchar dat){ //数码管显示
                                   //写显存必须从高地址开始写
        Start1650();
        WrByte1650(add);          //第一个显存地址
        Ask1650();
        WrByte1650(dat);
        Ask1650();
        Stop1650();
}

#endif
