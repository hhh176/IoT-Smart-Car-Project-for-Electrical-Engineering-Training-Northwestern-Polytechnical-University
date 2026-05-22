#include<IIC.h>

sbit SCL=P1^6;
sbit SDA=P1^7;


void IIC_delayms()        //用于IIC延时
{
	_nop_();_nop_();_nop_();_nop_();
}

void OpenIIC() 		//IIC启动信号
{
	SDA=1;
	SCL=1;
	IIC_delayms();  
	SDA=0;
	IIC_delayms();  
	SCL=0;
}
void CloseIIC()			//IIC停止信号
{	
	 SCL=0;
	 SDA=0;
	 IIC_delayms();  
	 SCL=1; 
	 SDA=1; 
	 IIC_delayms();  
}

uchar IIC_Wait_Ack(void)	//IIC发送字节后 等待从机发送响应信
{	
	uchar ucErrTime=0;  
	SDA=1;
	IIC_delayms();  	   
	SCL=1;
	IIC_delayms();  	 

	while(SDA==1)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			CloseIIC();
			return 1;
		}
	}
	SCL=0;	   
	return 0;  
} 
void IIC_Ack(void)	  //发送应答信号
{
	SCL=0;
	SDA=0;		  
	IIC_delayms();  		
	SCL=1;
	IIC_delayms();  		
	SCL=0;
}
	    
void IIC_NAck(void)	 //IIC 非应答信号
{
	SCL=0;
	SDA=1;		 
	IIC_delayms();  	
	SCL=1;
	IIC_delayms();  		
	SCL=0;
}					 				     
void IICsendByte(uchar txd)		 //IIC·写一个字节
{                        
    uchar t; 
    SCL=0;
    for(t=0;t<8;t++)
    {  
		if(((txd&0x80)>>7)==1)
			SDA=1;
		else 
			SDA=0;
        txd<<=1; 	  
		IIC_delayms();   
		SCL=1;
		IIC_delayms();  
		SCL=0;	
		IIC_delayms();  
    }	 
} 	
uchar IICReadByte(unsigned char ack)  //IIC读一个字节
{
	unsigned char i,receive=0;
	SDA=1;		  //51单片机讲引脚置高可设为输入引脚
    for(i=0;i<8;i++ )
	{			
		SCL=0; 
		IIC_delayms();  
		SCL=1;
		receive<<=1;
		if(SDA==1)receive++;   
		IIC_delayms();  
    }			
    if (!ack)
        IIC_NAck();
    else
        IIC_Ack();  
    return receive;
}
