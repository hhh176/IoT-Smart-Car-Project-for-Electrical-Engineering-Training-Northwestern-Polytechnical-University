/*--------------------------------------------------------------------------
TM1650.H
2λ�����TM1650 ��д����
--------------------------------------------------------------------------*/
#ifndef __TM1650_H__
#define __TM1650_H__

#include "intrins.h"
#define uchar unsigned char
#define uint  unsigned int
sbit CLK = P1^6;
sbit DIO = P1^7;
void Set1650(uchar add,uchar dat);

uchar ld = 1;          //显示亮度
uchar code tab[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};//0-F
                                // 0    1    2    3    4    5    6    7    8    9

void Init1650(){
     Set1650(0x48,(ld*16+0x01));//�趨���ȣ�����ʾ�˶���ʾ��ʽ����һ���ֽ��и�λ�������ȣ���λ�����߶λ�˶���ʾ��ʽ�Ϳ�����
                                //�磺0x71Ϊ�߼����ȣ��˶���ʾ��ʽ������ʾ��0x79ΪΪ�߼����ȣ��߶���ʾ��ʽ������ʾ
}

void Delay_us(uint i){ //us��ʱ
        for(;i>0;i--){
                _nop_();
                _nop_();
                _nop_();
                _nop_();
                _nop_();
        }
}

void Start1650(void){//��ʼ�ź�
        CLK = 1;
        DIO = 1;
        Delay_us(5);
        DIO = 0;
}

void Ask1650(void){ //ACK�ź�
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

void Stop1650(void){ //ֹͣ�ź�
        CLK = 1;
        DIO = 0;
        Delay_us(5);
        DIO = 1;
}

void WrByte1650(uchar oneByte){//дһ���ֽڸ�λ��ǰ����λ�ں�
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


void Set1650(uchar add,uchar dat){ //�������ʾ
                                   //д�Դ����Ӹߵ�ַ��ʼд
        Start1650();
        WrByte1650(add);          //��һ���Դ��ַ
        Ask1650();
        WrByte1650(dat);
        Ask1650();
        Stop1650();
}

#endif
