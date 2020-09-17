#include  <msp430x552x.h>
#include <stdbool.h>

#define IIC_SDA_1 P3OUT |= BIT0 //SDA = 1
#define IIC_SDA_0 P3OUT &=~ BIT0 //SDA = 0
#define IIC_SCL_1 P3OUT |= BIT1 //SCL = 1
#define IIC_SCL_0 P3OUT &=~ BIT1 //SCL = 0
#define SDA_IN P3DIR &=~ BIT0 //I/O��Ϊ����
#define SDA_OUT P3DIR |= BIT0 //I/0��Ϊ���
#define READ_SDA (P3IN & 0x01) //Read SDA

#define PCF8563_Address_Control_Status_2 0x01  //����/״̬�Ĵ���2

#define PCF8563_Write			0xa2	//д����
#define PCF8563_Read			0xa3	//����������ã�PCF8563_Write + 1��
#define Address_year			0x08	//��
#define Address_month			0x07	//��
#define Address_date			0x05	//��
#define Address_week			0x06	//����
#define Address_hour			0x04	//Сʱ
#define Address_minute			0x03	//����
#define Address_second			0x02	//��

#define Alarm_minute			0x09	//���ӱ���
#define Alarm_hour			0x0a	//Сʱ����
#define Alarm_date			0x0b	//�ձ���
#define Alarm_week			0x0c	//���ڱ���


#define u8  unsigned char
#define uchar unsigned char
#define CPU_F ((double)1024*1024)
#define delay_us(x) __delay_cycles((long)(CPU_F*(double)x/1000000.0))
#define delay_ms(x) __delay_cycles((long)(CPU_F*(double)x/1000.0))

int i;
uchar buffer1[] = {"����\n"};
uchar buffers[6][3] = {{"��"}, {"��"}, {"��"}, {"ʱ"}, {"��"}, {"��"}};
uchar bufferOfNum[4];
int state = 0;

unsigned char date_time[6];
unsigned char my_date_time[6]={0x20, 0x09, 0x10, 0x18, 0x41, 0x50};

void halUsbInit();
void delay(unsigned int n) ;                                              // ��ʼ������ 
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


void uartPrint(uchar code[], int len) // ����ַ���
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

int x2d(int a) // 16 ����ת 10 ����
{
  return a/16*10 + a%16;
}

int d2x(uchar num){
  return num/10*16 + num%10;
}

uchar get_time_after(uchar num) // ��ȡn�����Ժ�ľ��Է���
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
  P4SEL = BIT4 +BIT5;           // ���� 4.4 �� 4.5 Ϊ���λ
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
    WDTCTL = WDTPW + WDTHOLD; // �رտ��Ź� 
    // ����2.1��������Ϊ��ʹ�ð�ť������ʪ������
    
    
    P2DIR &=~ BIT1; 
    P2REN |= BIT1; 
    P2OUT |= BIT1;// ����2.1Ϊ����λ�� ���������裬 ���ó�ʼֵΪ�ߵ�ƽ
    P2IES &=~ BIT1; 
    P2IFG &=~ BIT1; 
    P2IE |= BIT1;// ����2.1�����жϣ� �жϱ�־λ��λ�������ж�
    
    // ����1.1��������Ϊ�˰���ť������ʪ������
    P1DIR &=~ BIT1; 
    P1REN |= BIT1; 
    P1OUT |= BIT1;// ����1.1Ϊ����λ�� ���������裬 ���ó�ʼֵΪ�ߵ�ƽ
    P1IES &=~ BIT1; 
    P1IFG &=~ BIT1; 
    P1IE |= BIT1;// ����1.1�����жϣ� �жϱ�־λ��λ�������ж�
    
    // ����1.2 ��������
    P1DIR &=~ BIT2; 
    P1REN |= BIT2; 
    P1OUT |= BIT2;// ����2.1Ϊ����λ�� ���������裬 ���ó�ʼֵΪ�ߵ�ƽ
    
    
    // ����ʱ���ж�
    TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled 32768
    TA0CCR0 = 10923;
    TA0CTL |= MC_1; //��������CCR0
    TA0CTL|=TASSEL_1;
    
    // ����С��
    P1DIR |= BIT0; P1OUT &=~BIT0;
      setUart();
     esp_config();
    // ���ڳ�ʼ��
    halUsbInit();
//    __bis_SR_register(LPM3_bits);                                  // �͹���3
    P3DIR &=~ BIT0; //I/O��Ϊ����
    P3REN |= BIT0;  //����P3.0�ڲ�����������
    P3OUT |= BIT0;  //����������Ϊ����
    P3DIR |= BIT1;
    P3OUT |= BIT1;
    PCF8563_init();
    PCF8563_Read_date();
    delay(30);
    // CPU��Ϣ��GIE����ȫ���ж�ʹ��
    __bis_SR_register(LPM0_bits +GIE);
    // ֹͣ����ָ��
    __no_operation();
}
/*************************************************************************
** �������ƣ����ڳ�ʼ������
** ��������: IAR 4.21 
** ����:     
** ��������: 
** ����:��ʼ������
���䷽ʽ���ã��������ã�����������
** ����ļ�:
** �޸���־��
*************************************************************************/
void halUsbInit()
{
 //   P3SEL |= BIT3 + BIT4;                                        // ѡ�񴮿ڹ���
/*
  P4SEL |= BIT4+BIT5; 
  UCA1CTL1 = UCSWRST;                                          // ��λ���ڹ���
  UCA1CTL0 = UCMODE_0;                                         // ѡ�񴮿ڹ���  
  UCA1CTL0 &= ~UC7BIT;                                         // 8λ����ģʽ
  UCA1CTL1 |= UCSSEL_1;                                        // ʱ��Դѡ��
  UCA1BR0 = 3;                                                 // 32768hz/3=9600
  UCA1BR1 = 0;
  UCA1MCTL = 06;                                               // �����ʵ�������
  UCA1CTL1 &= ~UCSWRST;                                        // ʹ�ܴ��ڹ���
 
  UCA1IE |= UCRXIE;                                            // ʹ�ܽ����ж�
    __bis_SR_register(GIE); 
*/
  P4SEL = BIT4 +BIT5;           // ���� 4.4 �� 4.5 Ϊ���λ
  UCA1CTL1 |= UCSWRST;          // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_2;         // CLK = SMCLK   MCLK = SMCLK = default DCO = 32 x ACLK = 1048576Hz
    UCA1CTL0 = UCMODE_0;                                         // ѡ�񴮿ڹ���  
    UCA1CTL0 &= ~UC7BIT;                                         // 8λ����ģʽ
  UCA1BR0 = 0x09;               // 1048576/115200 = ~9.10 (see User's Guide)
  UCA1BR1 = 0x00;               
  UCA1MCTL = UCBRS_1+UCBRF_0;   // Modulation UCBRSx=3, UCBRFx=0
  UCA1CTL1 &= ~UCSWRST;         // **Initialize USCI state machine**
  
    UCA1IE |= UCRXIE;                                            // ʹ�ܽ����ж�
    __bis_SR_register(GIE); 
}
/*************************************************************************
** �������ƣ����ڽ����жϺ���
** ��������: IAR 4.21 
** ����:     
** ��������: 
** ����:���ͽ��յ�������
** ����ļ�:
** �޸���־��
*************************************************************************/
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    while (!(UCA1IFG & UCTXIFG));                                // �ж��Ƿ������
    UCA1TXBUF = UCA1RXBUF;                                       // ���ͽ��յ�������
}

// 2.1�����ж�
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
//    PCF8563_set();
    PCF8563_Read_date();
    P2IFG &=~ BIT1;// �����־λ
}

// 1.1�����ж�
#pragma vector = PORT1_VECTOR 
__interrupt void Port_1(void) {
    //  �����¶�����
    uartPrint("���������\r\n", sizeof("���������\r\n"));
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
    uartPrint("������", sizeof("������"));
    uartPrint(bufferOfNum, int32tocharbuff(bufferOfNum, a_temp));
    uartPrint("���ӱ���\r\n", sizeof("���ӱ���\r\n"));
    P1IFG &= ~BIT1; // �����־λ
}


// ��ʱ�������ж�
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
    if (n < 0){  // �ж�����
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
    while(n){   // ��temp��ÿһλ�ϵ�������buf
        buf[i++] = (n % 10) + '0';  
        n = n / 10;
    }
    str[i] = 0; // ��ӽ�β
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

void DelayMS(unsigned int dly)//��ʱ1ms
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
  	SDA_OUT;     //sda�����
	IIC_SDA_1;	  	  
	IIC_SCL_1;
	delay(4);
 	IIC_SDA_0;//START:when CLK is high,DATA change form high to low 
	delay(4);
	IIC_SCL_0;//ǯסI2C���ߣ�׼�����ͻ�������� 
}

void IIC_Stop(void)
{
  SDA_OUT;//sda�����
  IIC_SCL_0;
  IIC_SDA_0;//STOP:when CLK is high DATA change form low to high
   delay(4);
  IIC_SCL_1; 
  IIC_SDA_1;//����I2C���߽����ź�
  delay(4);	
 
}

//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
unsigned char IIC_Wait_Ack(void)
{
	unsigned char ucErrTime=0;
	SDA_IN;      //SDA����Ϊ����  
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
	IIC_SCL_0;//ʱ�����0 	   
	return 0;  

}

//����ACKӦ��
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
//������ACKӦ��		    
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

//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��			  
void IIC_Send_Byte(unsigned char txd)
{                        
    unsigned char t;   
	SDA_OUT; 	    
    IIC_SCL_0;//����ʱ�ӿ�ʼ���ݴ���
    for(t=0;t<8;t++)
    {              
        //IIC_SDA=(txd&0x80)>>7;
		if((txd&0x80)>>7)
			IIC_SDA_1;
		else
			IIC_SDA_0;
		txd<<=1; 	  
		delay(2);   //��TEA5767��������ʱ���Ǳ����
		IIC_SCL_1;
		delay(2); 
		IIC_SCL_0;	
		delay(2);
    }	 
} 	    
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
unsigned char IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN;//SDA����Ϊ����
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
        IIC_NAck();//����nACK
    else
        IIC_Ack(); //����ACK   
    return receive;
}

/******************************************************************************
* Function --> PCF8563ĳ�Ĵ���д��һ���ֽ�����
* Param    --> REG_ADD��Ҫ�����Ĵ�����ַ
*              dat��    Ҫд�������
* Reaturn  --> none 
* Brief    --> none
******************************************************************************/
void PCF8563_Write_Byte(u8 REG_ADD,u8 dat)
{
	IIC_Start();
	IIC_Send_Byte(PCF8563_Write);	//����д������Ӧ��λ
		IIC_Wait_Ack();
		IIC_Send_Byte(REG_ADD);
		IIC_Wait_Ack();
		IIC_Send_Byte(dat);	//��������
		IIC_Wait_Ack();
	IIC_Stop();
	delay(10);
} 
/******************************************************************************
* Function --> PCF8563ĳ�Ĵ�����ȡһ���ֽ�����
* Param    --> REG_ADD��Ҫ�����Ĵ�����ַ
* Reaturn  --> ��ȡ�õ��ļĴ�����ֵ
* Brief    --> none
******************************************************************************/
u8 PCF8563_Read_Byte(u8 REG_ADD)
{
	u8 ReData;
	IIC_Start();
	IIC_Send_Byte(PCF8563_Write);	//����д������Ӧ��λ
	IIC_Wait_Ack();
		IIC_Send_Byte(REG_ADD);	//ȷ��Ҫ�����ļĴ���
	IIC_Wait_Ack();
		IIC_Start();	//��������
		IIC_Send_Byte(PCF8563_Read);	//���Ͷ�ȡ����
	IIC_Wait_Ack();
		ReData = IIC_Read_Byte(0);	//��ȡ����
	
		//IIC_Ack(1);	//���ͷ�Ӧ���źŽ������ݴ���
	
	IIC_Stop();
	return ReData;
}


