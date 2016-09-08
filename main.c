#include "compiler_defs.h"
#include "C8051F560_defs.h"
#include "font_X.h"

#include <stdio.h>

sbit RST = P1^5;
sbit DC = P1^4;
sbit CS = P1^3;
//sbit LED0 = P2^2;
//sbit LED1 = P2^3;

unsigned char RX_BYTE;		//����� � ��� ������ � SBUF0 � ���������
unsigned char number; 		//�������� ����
unsigned char cnt_tim1; 	//�������� ����������� ������� 1
bit  start_fl,finish_fl; 	//����� ��������� ������� �����
int mass_in[13];			//����� ��������� �� ����� �����
int delay_cnt; 				//�������� ����������� ������� ��������
xdata unsigned char lcd8544_buff[84*6]; // ����� �������
unsigned int i;
int Horizont_ygol,Vertical_ygol,Dalnost; //
char str_Horizont_ygol[6];  //������ ��� �������� �� �������
char str_Vertical_ygol[6];
char str_Dalnost[5];
 //.......................................................................//
 //��������� �������
 //.......................................................................//
void lcd8544_init(void); // ������������� �������
void lcd8544_sendCmd(unsigned char cmd);	// �������� ������� �� �������		
void lcd8544_refresh(void);// ������� �������		
void lcd8544_clear(void);// ������� ������	
void lcd8544_putpix(unsigned char x, unsigned char y, unsigned char mode);	// ����� �������		
void lcd8544_line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode);// ����� �����		
void lcd8544_rect(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode);// ��������� �������������� (�� ������������)		
void lcd8544_putchar(unsigned char px, unsigned char py, unsigned char ch, unsigned char mode);// ����� ������� � ����� �� �����������	
void lcd8544_leftchline(unsigned char x, unsigned char y);	// ����� ����� ������� ��� ������ � �������
void lcd8544_putstr(unsigned char x, unsigned char y, const unsigned char str[], unsigned char mode);// ����� ������ � ����� �� �����������		
void lcd8544_dec(unsigned int numb, unsigned char dcount, unsigned char x, unsigned char y, unsigned char mode);// ������ ����������� �����

void Init_Device(void);
void Delay_ms(int delay_ms);
void read_13_byte(void);
void display_13_byte(void);
int convert(int a);

INTERRUPT_PROTO (UART0_ISR, INTERRUPT_UART0);
INTERRUPT_PROTO (TIMER0_ISR, INTERRUPT_TIMER0);
INTERRUPT_PROTO (TIMER1_ISR, INTERRUPT_TIMER1);

//*************************************************************************//������� ��������
void main(void)
{
Init_Device();						// ��������� ��������� ���������������
SFRPAGE = ACTIVE_PAGE;              // Change for PCA0MD and SBUF0
PCA0MD &= ~0x40;                    // Disable the watchdog timer

lcd8544_init();
lcd8544_clear();
		
		while(1)
		{
			
read_13_byte(); 								 //������ 13 ���� � ����� � �����

//***********************************************//��������� �������� � ��������� �����
Horizont_ygol=(mass_in[6]<<8)+mass_in[7]; 			
Vertical_ygol=(mass_in[8]<<8)+mass_in[9];
Dalnost=(mass_in[10]<<8)+mass_in[11];
//***********************************************//������������ � ������ ��������
Horizont_ygol=convert(Horizont_ygol); 				
Vertical_ygol=convert(Vertical_ygol);
//***********************************************//������������ � ������
sprintf(str_Horizont_ygol, "%d", Horizont_ygol); 
sprintf(str_Vertical_ygol, "%d", Vertical_ygol);
sprintf(str_Dalnost, 	   "%d", Dalnost);

lcd8544_clear();				          	     //������� ������



//***********************************************//������� ������� (�� ���������)
if((mass_in[12]&3)==1){lcd8544_putstr(0,30,"ֳ��- 1",0);} 	//��������� �����
if((mass_in[12]&3)==2){lcd8544_putstr(0,30,"ֳ��- 2",0);}
if((mass_in[12]&3)==3){lcd8544_putstr(0,30,"ֳ��- 3",0);}

//************************************************//

if((mass_in[12]&4)==4)  {lcd8544_putstr(0,40 ,"�",0);} 	//������
if((mass_in[12]&8)==8)  {lcd8544_putstr(10,40,"�",0);} 	//ͳ�
if((mass_in[12]&16)==16){lcd8544_putstr(20,40,"�",0);} 	//����
if((mass_in[12]&32)==32){lcd8544_putstr(30,40,"�",0);} //����� ��
if((mass_in[12]&64)==64){lcd8544_putstr(40,40,"�",0);} //���в�

lcd8544_putstr(0,0,str_Horizont_ygol,0);  				//���� ��������������� ����
lcd8544_putstr(35,0,"��������",0);

lcd8544_putstr(0,10,str_Vertical_ygol,0); 				//���� ������������� ����
lcd8544_putstr(35,10,"��������",0);

lcd8544_putstr(0,20,str_Dalnost,0); 					//���� ��������
lcd8544_putstr(35,20,"³�����",0);
//lcd8544_dec(Horizont_ygol, 4, 8, 8, 0);				//�������� ��������������� ����
//lcd8544_dec(Vertical_ygol, 4, 8, 16, 0); 				//�������� ������������� ����
//lcd8544_dec(Dalnost, 4, 8, 24, 0); 					//�������� ��������

lcd8544_refresh();										//��������� ���������� �� ������	
						
									
for(number=0;number<13;number++){mass_in[number]=0;	}	//�������� ���������� ������
//display_13_byte(); //��������� 
Delay_ms(100); 
		}
}
//*************************************************************************
void read_13_byte(void)						// ������� ������� 13 ���� �� ����
{
TR1=1; //timer 1 start   			 		//������ ������� 1 � ������� 5��
while((!finish_fl)&(cnt_tim1<10)){}  		//���������� ������� �����, ������ ����� ����� 10 ����������� ������� (50��)
cnt_tim1=0; 								//�������� ��������� ����������� ������� 1
finish_fl=0;								//�������� ����� �������� ���������� �������.
TR1=0; 										//������� ������� 1	
}
	
//*************************************************************************// 
void display_13_byte(void)
{
lcd8544_clear();				             		//������� ������	
for(number=0;number<7;number++)		
{
lcd8544_dec(mass_in[number+6], 3, 8, 7*number, 0); 	//�������� ������� �� �������
mass_in[number]=0;								 	//�������� ���������� ������
}	
lcd8544_refresh();									//��������� ���������� �� ������										
}
//*************************************************************************//������������ � ������ ��������
int convert(int a) 
{
  if (a < 0)
    a = ( ~-a|32768 ) + 1;
  return a;
}
//*************************************************************************// ���������� ����
INTERRUPT(UART0_ISR, INTERRUPT_UART0) 
{
   if (RI0 == 1 ) 				//���� ���������� �� �������
   { 
    TL1 = 0x1E; 				//������������ ������� 1
	TH1 = 0xFB;	
	RX_BYTE=SBUF0; 				// ������ � ������� ����� ����
		if(start_fl==1) 		//���� ��������� ������� ������ �����
		{
		mass_in[number]=SBUF0;  //����� ����� � �����
		number++;
			if(number==13)		//���� �������� 13 ���� 
			{
			start_fl=0;  		// ���������� ������ �������� �������
			finish_fl=1; 		//��������� ����� ���������� �������
			} 
		}

	RI0 = 0; 					//�������� ����� ���������� �� ������� ����
   }

   if (TI0 == 1)          // Check if transmit flag is set
   {     
    TI0 = 0;                // Clear interrupt flag
   }
}

INTERRUPT(TIMER0_ISR, INTERRUPT_TIMER0) //���������� ������� �������� (����� ������������ 1��)
{
TF0=0; //�������� ����� ���������� ������� 0
delay_cnt++;
}
INTERRUPT(TIMER1_ISR, INTERRUPT_TIMER1) //���������� ������� ������ ������� ������� ����� (����� ������������ 5��)
{										//���� ������������ �� ����� ���������� ������� ���
TF1=0; //�������� ����� ���������� ������� 1
TL1       = 0x1E; //���������������� �������� �������
TH1       = 0xFB;
number=0;		//�������� ������ ����������� ����� � ����� ��������� ����� �� ��������
start_fl=1;		//����� ���������� ��� � �����
cnt_tim1++;		//������ ������� ����������� ������. (������� ���� ���� �������)
}
//*************************************************************************//��������
void Delay_ms (int delay_ms)
{	
TR0=1; 								//������ ������� 0 
delay_cnt=0; 						//�������� ��������� ��������� ������� 0	
while(delay_cnt<=delay_ms){} 		//���������� ��������� ������� �����������
TR0=0; 								//������� ������� 0
}
//*************************************************************************//³������� �� SPI
void lcd8544_senddata(char LCDdata)
{ 
	SPIF=0; 				//�������� ����� ����������
 	while (!TXBMT);  		//���������� ����������� ������ ��������
    SPI0DAT = LCDdata; 	//
	while (!SPIF); 
}
//*************************************************************************// ��������� ������
void lcd8544_refresh(void) {
	unsigned char y,x;
	CS=0;            			// �S=0 - ������ ����� ������ � ��������
	DC=0;            			// �������� �������
	lcd8544_senddata(0x40);		// ��������� ������� � ������� Y=0; X=0
	lcd8544_senddata(0x80);
	DC=1;            			// �������� ������
	for (y=0;y<6;y++) for (x=0;x<84;x++) lcd8544_senddata(lcd8544_buff[y*84+x]);
	 Delay_ms(10);
	 CS=1;						// �S=1 - ��������� ����� ������ � ��������
}
//*************************************************************************//������� �������
void lcd8544_clear(void) {				 
   unsigned char x,y;
   for (y=0;y<6;y++) for(x=0;x<84;x++) lcd8544_buff[y*84+x]=0;
}
//*************************************************************************//�������������
void lcd8544_init(void) {			
	CS=0;            					// CS=0  - ������ ����� ������ � ��������
	DC=0;								// A0=0  - ������� � ��������� �����
										// ����� �������
	RST=0;                   			// RST=0
    Delay_ms(1);						// �����
	RST=1;                   			// RST=1
	Delay_ms(50);						// �����
//**************************************// ������������� �������
	lcd8544_senddata(0x21);      		// ������� � ����������� �����
	lcd8544_senddata(0xC1);
	lcd8544_senddata(0x06);				// ������������� �����������, �� 4 �� 7
	lcd8544_senddata(0x13);				// Bias 0b0001 0xxx - �������� ��� �������������
	lcd8544_senddata(0x20); 			// ������� � ������� �����
	lcd8544_senddata(0xC);				// 0b1100 - normal mode // 0b1101 - invert mode // 0b1001 - ��������� ����������� �����// 0b1000 - ������ �����                     	                        
	CS=1;
}
//*************************************************************************//����� �������
void lcd8544_putpix(unsigned char x, unsigned char y, unsigned char mode) {
	unsigned int adr=(y>>3)*84+x;
unsigned char dataLCD=(1<<(y&0x07));
	if ((x>84) || (y>47)) return;
	if (mode) lcd8544_buff[adr]|=dataLCD;
   else lcd8544_buff[adr]&=~dataLCD;
}
//*************************************************************************// ��������� ��������� �����
void lcd8544_line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode) {
signed char   dx, dy, sx, sy;
unsigned char  x,  y, mdx, mdy, l;
  dx=x2-x1; dy=y2-y1;
  if (dx>=0) { mdx=dx; sx=1; } else { mdx=x1-x2; sx=-1; }
  if (dy>=0) { mdy=dy; sy=1; } else { mdy=y1-y2; sy=-1; }
  x=x1; y=y1;
  if (mdx>=mdy) {
     l=mdx;
     while (l>0) {
         if (dy>0) { y=y1+mdy*(x-x1)/mdx; }
            else { y=y1-mdy*(x-x1)/mdx; }
         lcd8544_putpix(x,y,mode);
         x=x+sx;
         l--;
     }
  } else {
     l=mdy;
     while (l>0) {
        if (dy>0) { x=x1+((mdx*(y-y1))/mdy); }
          else { x=x1+((mdx*(y1-y))/mdy); }
        lcd8544_putpix(x,y,mode);
        y=y+sy;
        l--;
     }
  }
  lcd8544_putpix(x2, y2, mode);
}
//*************************************************************************// ��������� �������������� (�� ������������)
void lcd8544_rect(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode) {
	lcd8544_line(x1,y1, x2,y1, mode);
	lcd8544_line(x1,y2, x2,y2, mode);
	lcd8544_line(x1,y1, x1,y2, mode);
	lcd8544_line(x2,y1, x2,y2, mode);
}
//*************************************************************************// ������ ����������� �����
void lcd8544_dec(unsigned int numb, unsigned char dcount, unsigned char x, unsigned char y, unsigned char mode) {
	unsigned int divid=10000;
	unsigned char i;
	for (i=5; i!=0; i--) {
		unsigned char res=numb/divid;
		if (i<=dcount) {
			lcd8544_putchar(x, y, res+'0', mode);
			x=x+6;
		}
		numb%=divid;
		divid/=10;
	}
}
//*************************************************************************// ����� ������� �� ����� �� �����������
void lcd8544_putchar(unsigned char px, unsigned char py, unsigned char ch, unsigned char mode) {
	data unsigned char *fontpointer;
 unsigned char lcd_YP=7- (py & 0x07);    // ������� ������� ������� � �����
 unsigned char lcd_YC=(py & 0xF8)>>3; 	// �������� ������� ������� �� ������
 unsigned char x;

	for (x=0; x<5; x++) {
unsigned char temp=(FontTable[ch][x]);
		if (mode!=0) {
			temp=~temp;
			if (py>0) lcd8544_putpix(px, py-1, 1);	// ���� ������ � ������ �������� - ������ �������� �����
		}
		temp&=0x7F;
		lcd8544_buff[lcd_YC*84+px]=lcd8544_buff[lcd_YC*84+px] | (temp<<(7-lcd_YP)); 	// ������ ������� ����� �������
	    if (lcd_YP<7) lcd8544_buff[(lcd_YC+1)*84+px]=lcd8544_buff[(lcd_YC+1)*84+px] | (temp>>(lcd_YP+1)); 	// ������ ������ ����� �������
		px++;
		if (px>83) return;
	}
}
//*************************************************************************// ����� ����� ������� ��� ������ � �������
void lcd8544_leftchline(unsigned char x, unsigned char y) {
	if (x>0) lcd8544_line(x-1, y-1, x-1, y+6, 1);
}
//*************************************************************************// ����� ������
void lcd8544_putstr(unsigned char x, unsigned char y,  unsigned char str[], unsigned char mode) {
	if (mode) lcd8544_leftchline(x, y);
	while (*str!=0) {
		lcd8544_putchar(x, y, *str, mode);
		x=x+6;
		str++;
	}
}
//*************************************************************************//����������� �������
void Reset_Sources_Init()
{
    RSTSRC    = 0x06;
}
void PCA_Init()
{
    PCA0MD    &= ~0x40;
    PCA0MD    = 0x00;
    PCA0CPL5  = 0xC2;
    PCA0MD    |= 0x40;
}
void Timer_Init()
{
    TCON      = 0x50;
    TMOD      = 0x12;
    CKCON     = 0x02;
    TL1       = 0x1E;
    TH0       = 0x05;
    TH1       = 0xFB;
}
void UART_Init()
{  //62500
    SCON0     = 0x10;
    SMOD0     = 0x3C;
    SFRPAGE   = CONFIG_PAGE;
    SBRLL0    = 0xA0;
    SBRLH0    = 0xFF;
    SBCON0    = 0x43;
    SFRPAGE   = ACTIVE_PAGE;

/* //9600
	SCON0     = 0x10;
    SFRPAGE   = CONFIG_PAGE;
    SBRLL0    = 0x8F;
    SBRLH0    = 0xFD;
    SBCON0    = 0x43;
    SFRPAGE   = ACTIVE_PAGE;*/
}
void SPI_Init()
{
    SPI0CFG   = 0x40;
    SPI0CN    = 0x01;
    SPI0CKR   = 0x0B;
}
void Port_IO_Init()
{
    // P0.0  -  Skipped,     Open-Drain, Digital
    // P0.1  -  Skipped,     Open-Drain, Digital
    // P0.2  -  Skipped,     Open-Drain, Digital
    // P0.3  -  Skipped,     Open-Drain, Digital
    // P0.4  -  UART_TX (UART0), Open-Drain, Digital
    // P0.5  -  UART_RX (UART0), Open-Drain, Digital
    // P0.6  -  Skipped,     Open-Drain, Digital
    // P0.7  -  Skipped,     Open-Drain, Digital

    // P1.0  -  SCK  (SPI0), Push-Pull,  Digital
    // P1.1  -  MISO (SPI0), Open-Drain, Digital
    // P1.2  -  MOSI (SPI0), Push-Pull,  Digital
    // P1.3  -  Unassigned,  Push-Pull,  Digital
    // P1.4  -  Unassigned,  Push-Pull,  Digital
    // P1.5  -  Unassigned,  Push-Pull,  Digital
    // P1.6  -  Unassigned,  Open-Drain, Digital
    // P1.7  -  Unassigned,  Open-Drain, Digital

    // P2.0  -  Unassigned,  Open-Drain, Digital
    // P2.1  -  Unassigned,  Open-Drain, Digital
    // P2.2  -  Unassigned,  Open-Drain, Digital
    // P2.3  -  Unassigned,  Open-Drain, Digital
    // P2.4  -  Unassigned,  Open-Drain, Digital
    // P2.5  -  Unassigned,  Open-Drain, Digital
    // P2.6  -  Unassigned,  Open-Drain, Digital
    // P2.7  -  Unassigned,  Open-Drain, Digital

    // P3.0  -  Unassigned,  Open-Drain, Digital

    SFRPAGE   = CONFIG_PAGE;
    P1MDOUT   = 0x3D;
    P0SKIP    = 0xCF;
    XBR0      = 0x05;
    XBR2      = 0x40;
    SFRPAGE   = ACTIVE_PAGE;
}
void Oscillator_Init()
{
    SFRPAGE   = CONFIG_PAGE;
    OSCICN    = 0xC6;
    SFRPAGE   = ACTIVE_PAGE;
}
void Interrupts_Init()
{
    IE        = 0x9A;
}

void Init_Device(void)
{
//PCA_Init();
Reset_Sources_Init();
    Timer_Init();
    UART_Init();
    SPI_Init();
    Port_IO_Init();
    Oscillator_Init();
    Interrupts_Init();
}
