#include  <msp430x552x.h>
#include <stdbool.h>

#define IIC_SDA_1 P3OUT |= BIT0 //SDA = 1
#define IIC_SDA_0 P3OUT &=~ BIT0 //SDA = 0
#define IIC_SCL_1 P3OUT |= BIT1 //SCL = 1
#define IIC_SCL_0 P3OUT &=~ BIT1 //SCL = 0
#define SDA_IN P3DIR &=~ BIT0 //I/O口为输入
#define SDA_OUT P3DIR |= BIT0 //I/0口为输出
#define READ_SDA (P3IN & 0x01) //Read SDA

#define PCF8563_Address_Control_Status_2 0x01  //控制/状态寄存器2

#define PCF8563_Write			0xa2	//写命令
#define PCF8563_Read			0xa3	//读命令，或者用（PCF8563_Write + 1）
#define Address_year			0x08	//年
#define Address_month			0x07	//月
#define Address_date			0x05	//日
#define Address_week			0x06	//星期
#define Address_hour			0x04	//小时
#define Address_minute			0x03	//分钟
#define Address_second			0x02	//秒

#define Alarm_minute			0x09	//分钟报警
#define Alarm_hour			0x0a	//小时报警
#define Alarm_date			0x0b	//日报警
#define Alarm_week			0x0c	//星期报警


#define u8  unsigned char
#define uchar unsigned char
#define CPU_F ((double)1024*1024)
#define delay_us(x) __delay_cycles((long)(CPU_F*(double)x/1000000.0))
#define delay_ms(x) __delay_cycles((long)(CPU_F*(double)x/1000.0))

int i;
uchar buffer1[] = {"年月\n"};
uchar buffers[6][3] = {{"年"}, {"月"}, {"日"}, {"时"}, {"分"}, {"秒"}};
uchar bufferOfNum[4];
int state = 0;

unsigned char date_time[6];
unsigned char my_date_time[6]={0x20, 0x09, 0x10, 0x18, 0x41, 0x50};

void halUsbInit();
void delay(unsigned int n) ;                                              // 初始化函数 
void PCF8563_init(void);
void IIC_Start(void);
void IIC_Stop(void);
unsigned char IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_Send_Byte(unsigned char txd);
unsigned char IIC_Read_Byte(unsigned char ack);
void PCF8563_Write_Byte(u8 REG_ADD,u8 dat);
u8 PCF8563_Read_Byte(u8 REG_ADD);
void PCF8563_set(void);
void PCF8563_Read_date(void);
int int32tocharbuff(uchar str[], int n);


void uartPrint(uchar code[], int len) // 输出字符串
{
  unsigned int i;
  for(i=0; i<len; i++){
    UCA1TXBUF = code[i];
    while (!(UCA1IFG&UCTXIFG)); 
  }
}

uchar bcd_dec(uchar bat) // bcd_dec
{
uchar temp1,temp2,tol;
temp1=bat&0x0f;
temp2=(bat&0xf0)>>4;
tol=temp2*10+temp1;
return tol; 
}

int x2d(int a) // 16 进制转 10 进制
{
  return a/16*10 + a%16;
}

int d2x(uchar num){
  return num/10*16 + num%10;
}

uchar get_time_after(uchar num) // 获取n分钟以后的绝对分钟
{
  return (num + x2d(PCF8563_Read_Byte(Address_minute)))%60;
}
void  mswrite(char *ptr){
  while(*ptr != '\0')
  {
    while (!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = *ptr++;
  }
  while (!(UCA1IFG&UCTXIFG));
}
void setUart(){
  P4SEL = BIT4 +BIT5;           // 设置 4.4 和 4.5 为输出位
  UCA1CTL1 |= UCSWRST;          // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_2;         // CLK = SMCLK   MCLK = SMCLK = default DCO = 32 x ACLK = 1048576Hz
  UCA1BR0 = 0x09;               // 1048576/115200 = ~9.10 (see User's Guide)
  UCA1BR1 = 0x00;               
  UCA1MCTL = UCBRS_1+UCBRF_0;   // Modulation UCBRSx=3, UCBRFx=0
  UCA1CTL1 &= ~UCSWRST;         // **Initialize USCI state machine**
}
void esp_config(){
  mswrite("AT+RST\r\n");
  delay_ms(15000);
  mswrite("AT+CWMODE=1\r\n");
  delay_ms(5000);
  mswrite("AT+CWJAP=\"dy\",\"a1234567\"\r\n");
  delay_ms(15000);
  mswrite("AT+CIPSTART=\"TCP\",\"192.168.137.1\",19730\r\n");
  delay_ms(2000);
  mswrite("AT+CIPMODE=1\r\n");
  delay_ms(5000);
  mswrite("AT+CIPSEND\r\n");
}

void main( void )
{
    WDTCTL = WDTPW + WDTHOLD; // 关闭看门狗 
    // 对于2.1的设置是为了使用按钮发送温湿度数据
    
    
    P2DIR &=~ BIT1; 
    P2REN |= BIT1; 
    P2OUT |= BIT1;// 设置2.1为输入位， 接上拉电阻， 设置初始值为高电平
    P2IES &=~ BIT1; 
    P2IFG &=~ BIT1; 
    P2IE |= BIT1;// 允许2.1触发中断， 中断标志位复位，设置中断
    
    // 对于1.1的设置是为了按按钮更新温湿度数据
    P1DIR &=~ BIT1; 
    P1REN |= BIT1; 
    P1OUT |= BIT1;// 设置1.1为输入位， 接上拉电阻， 设置初始值为高电平
    P1IES &=~ BIT1; 
    P1IFG &=~ BIT1; 
    P1IE |= BIT1;// 允许1.1触发中断， 中断标志位复位，设置中断
    
    // 对于1.2 进行设置
    P1DIR &=~ BIT2; 
    P1REN |= BIT2; 
    P1OUT |= BIT2;// 设置2.1为输入位， 接上拉电阻， 设置初始值为高电平
    
    
    // 设置时钟中断
    TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled 32768
    TA0CCR0 = 10923;
    TA0CTL |= MC_1; //增计数到CCR0
    TA0CTL|=TASSEL_1;
    
    // 设置小灯
    P1DIR |= BIT0; P1OUT &=~BIT0;
      setUart();
     esp_config();
    // 串口初始化
    halUsbInit();
//    __bis_SR_register(LPM3_bits);                                  // 低功耗3
    P3DIR &=~ BIT0; //I/O口为输入
    P3REN |= BIT0;  //启用P3.0内部上下拉电阻
    P3OUT |= BIT0;  //将电阻设置为上拉
    P3DIR |= BIT1;
    P3OUT |= BIT1;
    PCF8563_init();
    PCF8563_Read_date();
    delay(30);
    // CPU休息，GIE：打开全局中断使能
    __bis_SR_register(LPM0_bits +GIE);
    // 停止操作指令
    __no_operation();
}
/*************************************************************************
** 函数名称：串口初始化函数
** 工作环境: IAR 4.21 
** 作者:     
** 生成日期: 
** 功能:初始化串口
传输方式设置，主从设置，波特率设置
** 相关文件:
** 修改日志：
*************************************************************************/
void halUsbInit()
{
 //   P3SEL |= BIT3 + BIT4;                                        // 选择串口功能
/*
  P4SEL |= BIT4+BIT5; 
  UCA1CTL1 = UCSWRST;                                          // 复位串口功能
  UCA1CTL0 = UCMODE_0;                                         // 选择串口功能  
  UCA1CTL0 &= ~UC7BIT;                                         // 8位数据模式
  UCA1CTL1 |= UCSSEL_1;                                        // 时钟源选择
  UCA1BR0 = 3;                                                 // 32768hz/3=9600
  UCA1BR1 = 0;
  UCA1MCTL = 06;                                               // 波特率调整因子
  UCA1CTL1 &= ~UCSWRST;                                        // 使能串口功能
 
  UCA1IE |= UCRXIE;                                            // 使能接收中断
    __bis_SR_register(GIE); 
*/
  P4SEL = BIT4 +BIT5;           // 设置 4.4 和 4.5 为输出位
  UCA1CTL1 |= UCSWRST;          // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_2;         // CLK = SMCLK   MCLK = SMCLK = default DCO = 32 x ACLK = 1048576Hz
    UCA1CTL0 = UCMODE_0;                                         // 选择串口功能  
    UCA1CTL0 &= ~UC7BIT;                                         // 8位数据模式
  UCA1BR0 = 0x09;               // 1048576/115200 = ~9.10 (see User's Guide)
  UCA1BR1 = 0x00;               
  UCA1MCTL = UCBRS_1+UCBRF_0;   // Modulation UCBRSx=3, UCBRFx=0
  UCA1CTL1 &= ~UCSWRST;         // **Initialize USCI state machine**
  
    UCA1IE |= UCRXIE;                                            // 使能接收中断
    __bis_SR_register(GIE); 
}
/*************************************************************************
** 函数名称：串口接收中断函数
** 工作环境: IAR 4.21 
** 作者:     
** 生成日期: 
** 功能:发送接收到的数据
** 相关文件:
** 修改日志：
*************************************************************************/
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    while (!(UCA1IFG & UCTXIFG));                                // 判断是否发送完毕
    UCA1TXBUF = UCA1RXBUF;                                       // 发送接收到的数据
}

// 2.1进入中断
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
//    PCF8563_set();
    PCF8563_Read_date();
    P2IFG &=~ BIT1;// 清除标志位
}

// 1.1进入中断
#pragma vector = PORT1_VECTOR 
__interrupt void Port_1(void) {
    //  进入温度设置
    uartPrint("请输入分钟\r\n", sizeof("请输入分钟\r\n"));
    int isten = 0;
    uchar a_temp = 0;
    while (1) {
        while (! (UCA1IFG & UCRXIFG));
        uchar c_tt = UCA1RXBUF;
        if (c_tt >= '0' && c_tt <= '9') {
            isten++;
            if (isten == 1) {
                a_temp += (c_tt - '0') * 16;
            } else {
                a_temp += (c_tt - '0');
                break;
            }
        }
    }
    a_temp = get_time_after(a_temp);
    PCF8563_Write_Byte(Alarm_minute, d2x(a_temp));
    uartPrint("设置在", sizeof("设置在"));
    uartPrint(bufferOfNum, int32tocharbuff(bufferOfNum, a_temp));
    uartPrint("分钟报警\r\n", sizeof("分钟报警\r\n"));
    P1IFG &= ~BIT1; // 清除标志位
}


// 定时器进入中断
#pragma vector=TIMER0_A0_VECTOR
__interrupt void  IMER0_A0_ISR(void)
{
  date_time[4]=PCF8563_Read_Byte(Address_minute);
  date_time[5]=PCF8563_Read_Byte(Address_second);
  if(date_time[5]%5==0){
    PCF8563_Read_date();
  }
  if(P1IN & BIT2){
//    uartPrint("hi", 3);
  }else{
//    uartPrint("lo", 3);
    if(state == 0){
      P1OUT |= BIT0;
      state = 1;
    }
  }
  if(state){
    state ++;
    if(state == 30){
      state = 0;
      PCF8563_Write_Byte(PCF8563_Address_Control_Status_2, 
        PCF8563_Read_Byte(PCF8563_Address_Control_Status_2)^BIT3
      );
	  PCF8563_Write_Byte(Alarm_minute, 0x80);
      P1OUT &=~BIT0;
    }
  }
}
int int32tocharbuff(uchar str[], int n)  // 123 ->"123"
{
    int i = 0;
    int j = 0;
    _Bool isfu = false;
    uchar buf[10] = "";
    if (n < 0){  // 判断正负
        n = -n;
        str[0] = '-';
        i++;
        isfu = true;
    }else if(n==0){
        str[0] = '0';
        str[1] = 0;
        return 1;
    }
    j = i;
    while(n){   // 把temp的每一位上的数存入buf
        buf[i++] = (n % 10) + '0';  
        n = n / 10;
    }
    str[i] = 0; // 添加结尾
    i = i - 1;
    while(i>(isfu?0:-1)){
        str[i] = buf[j];
        i--;j++;
    }
    return j;
}
void PCF8563_init(void)
{
	PCF8563_Write_Byte(0,0x0);	
	PCF8563_Write_Byte(1,0x2);
	PCF8563_Write_Byte(0x0d,0x00);
	PCF8563_Write_Byte(0x0e,0x00);
	PCF8563_Write_Byte(Alarm_minute,0x80);
	PCF8563_Write_Byte(Alarm_hour,0x80);
	PCF8563_Write_Byte(Alarm_date,0x80);
	PCF8563_Write_Byte(Alarm_week,0x80);	
}
void PCF8563_set(void)
{
  PCF8563_Write_Byte(Address_year,0x20);
  PCF8563_Write_Byte(Address_month,0x09);
  PCF8563_Write_Byte(Address_date,0x12);
  PCF8563_Write_Byte(Address_hour,0x11);
  PCF8563_Write_Byte(Address_minute,0x20);
  PCF8563_Write_Byte(Address_second,0x00);
}
void PCF8563_Read_date(void)
{
  //volatile unsigned char date_time[6];
  
  date_time[0]=PCF8563_Read_Byte(Address_year);
  date_time[1]=PCF8563_Read_Byte(Address_month);
  date_time[2]=PCF8563_Read_Byte(Address_date);
  date_time[3]=PCF8563_Read_Byte(Address_hour);
  date_time[4]=PCF8563_Read_Byte(Address_minute);
  date_time[5]=PCF8563_Read_Byte(Address_second);
  for(i=0;i<6;i++)
  {
    delay(30);
//    UCA1TXBUF= date_time[i];
    uartPrint(bufferOfNum, int32tocharbuff(bufferOfNum, x2d(date_time[i])));
    uartPrint(buffers[i], 3);
  }
  uartPrint("\r\n", sizeof("\r\n"));
}

void DelayMS(unsigned int dly)//延时1ms
{ 
  unsigned int i;

  for(; dly>0; dly--)
  for(i=0; i<1135; i++);
}

void delay(unsigned int n)
{
  unsigned int i;
  for(; n>0; n--)
  for(i=0; i<100; i++);
}

void IIC_Start(void)
{
  	SDA_OUT;     //sda线输出
	IIC_SDA_1;	  	  
	IIC_SCL_1;
	delay(4);
 	IIC_SDA_0;//START:when CLK is high,DATA change form high to low 
	delay(4);
	IIC_SCL_0;//钳住I2C总线，准备发送或接收数据 
}

void IIC_Stop(void)
{
  SDA_OUT;//sda线输出
  IIC_SCL_0;
  IIC_SDA_0;//STOP:when CLK is high DATA change form low to high
   delay(4);
  IIC_SCL_1; 
  IIC_SDA_1;//发送I2C总线结束信号
  delay(4);	
 
}

//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
unsigned char IIC_Wait_Ack(void)
{
	unsigned char ucErrTime=0;
	SDA_IN;      //SDA设置为输入  
	IIC_SDA_1;delay(1);	   
	IIC_SCL_1;delay(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL_0;//时钟输出0 	   
	return 0;  

}

//产生ACK应答
void IIC_Ack(void)
{
	IIC_SCL_0;
	SDA_OUT;
	IIC_SDA_0;
	delay(2);
	IIC_SCL_1;
	delay(2);
	IIC_SCL_0;
}
//不产生ACK应答		    
void IIC_NAck(void)
{
	IIC_SCL_0;
	SDA_OUT;
	IIC_SDA_1;
	delay(2);
	IIC_SCL_1;
	delay(2);
	IIC_SCL_0;
}	

//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void IIC_Send_Byte(unsigned char txd)
{                        
    unsigned char t;   
	SDA_OUT; 	    
    IIC_SCL_0;//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
        //IIC_SDA=(txd&0x80)>>7;
		if((txd&0x80)>>7)
			IIC_SDA_1;
		else
			IIC_SDA_0;
		txd<<=1; 	  
		delay(2);   //对TEA5767这三个延时都是必须的
		IIC_SCL_1;
		delay(2); 
		IIC_SCL_0;	
		delay(2);
    }	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
unsigned char IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN;//SDA设置为输入
    for(i=0;i<8;i++ )
	{
        IIC_SCL_0; 
        delay(2);
		IIC_SCL_1;
        receive<<=1;
        if(READ_SDA)receive++;   
		delay(1); 
    }					 
    if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK   
    return receive;
}

/******************************************************************************
* Function --> PCF8563某寄存器写入一个字节数据
* Param    --> REG_ADD：要操作寄存器地址
*              dat：    要写入的数据
* Reaturn  --> none 
* Brief    --> none
******************************************************************************/
void PCF8563_Write_Byte(u8 REG_ADD,u8 dat)
{
	IIC_Start();
	IIC_Send_Byte(PCF8563_Write);	//发送写命令并检查应答位
		IIC_Wait_Ack();
		IIC_Send_Byte(REG_ADD);
		IIC_Wait_Ack();
		IIC_Send_Byte(dat);	//发送数据
		IIC_Wait_Ack();
	IIC_Stop();
	delay(10);
} 
/******************************************************************************
* Function --> PCF8563某寄存器读取一个字节数据
* Param    --> REG_ADD：要操作寄存器地址
* Reaturn  --> 读取得到的寄存器的值
* Brief    --> none
******************************************************************************/
u8 PCF8563_Read_Byte(u8 REG_ADD)
{
	u8 ReData;
	IIC_Start();
	IIC_Send_Byte(PCF8563_Write);	//发送写命令并检查应答位
	IIC_Wait_Ack();
		IIC_Send_Byte(REG_ADD);	//确定要操作的寄存器
	IIC_Wait_Ack();
		IIC_Start();	//重启总线
		IIC_Send_Byte(PCF8563_Read);	//发送读取命令
	IIC_Wait_Ack();
		ReData = IIC_Read_Byte(0);	//读取数据
	
		//IIC_Ack(1);	//发送非应答信号结束数据传送
	
	IIC_Stop();
	return ReData;
}


