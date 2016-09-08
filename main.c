#include "compiler_defs.h"
#include "C8051F560_defs.h"
//#include "font_X.h"

#include <stdio.h>

sbit CS_1 = P1^1;
sbit CS_2 = P1^2;
sbit CS_3 = P1^3;

unsigned char RX_BYTE;		//змінна в яку читаємо з SBUF0 в прериванні
unsigned char number; 		//лічильник байт
unsigned char cnt_tim1; 	//лічильник переповнень таймера 1
bit  start_fl,finish_fl; 	//флаги готовності прийому даних
int mass_in[13];			//буфер отриманих по УСАПП даних
int delay_cnt; 				//лічильник переповнень таймера затримки
unsigned int i;
int Horizont_ygol,Vertical_ygol,Dalnost; //
char str_1[6];  //строки для передачі на дисплей
char str_2[6];
char str_3[5];
//*************************************************************************///Знакогенератор для семисегментного индикатора
const unsigned char	code_digit [] =						
{
	0x3f,	//0
	0x06,	//1
	0x5b,	//2
	0x4f,	//3
	0x66,	//4
	0x6d,	//5
	0x7d,	//6
	0x07,	//7
	0x7f,	//8
	0x6f	//9
};
 //.......................................................................//
 //Прототипи функцій
 //.......................................................................//


void Init_Device(void);
void SPI_Byte_Write (U8 SPI_WRITE);
void STLED_on(void);
void Transmit_str1(void);
void Transmit_str2(void);
void Transmit_str3(void);
void Delay_ms(int delay_ms);
void read_13_byte(void);
int convert(int a);
void sort_data(void);
void display(void);


INTERRUPT_PROTO (UART0_ISR, INTERRUPT_UART0);
INTERRUPT_PROTO (TIMER0_ISR, INTERRUPT_TIMER0);
INTERRUPT_PROTO (TIMER1_ISR, INTERRUPT_TIMER1);

//*************************************************************************//Основна програма
void main(void)
{
Init_Device();
STLED_on();						// Початкова настройка мікроконтроллера
SFRPAGE = ACTIVE_PAGE;              // Change for PCA0MD and SBUF0
PCA0MD &= ~0x40;                    // Disable the watchdog timer
		
		while(1)
		{
			
read_13_byte(); 		//прийом 13 байт і запис в масив
sort_data();			//підготовка даних для виводу на дисплей
display();				//передача по SPI в STLED316
Delay_ms(100); 			//затримка для відображення														
for(number=0;number<13;number++){mass_in[number]=0;}	//очищення прийомного буферу
		}
}
//*************************************************************************
void read_13_byte(void)						// Функція читання 13 байт по УАПП
{
TR1=1; //timer 1 start   			 		//запуск таймера 1 з періодом 5мс
while((!finish_fl)&(cnt_tim1<10)){}  		//очікування прийому даних, інакше вихід через 10 переповнень таймера (50мс)
cnt_tim1=0; 								//зкидання лічильника переповнень таймера 1
finish_fl=0;								//зкидання флагу успішного завершення прийому.
TR1=0; 										//зупинка таймера 1	
}
void sort_data(void)
{
//складання старшого і молодшого байтів
Horizont_ygol=(mass_in[6]<<8)+mass_in[7]; 			
Vertical_ygol=(mass_in[8]<<8)+mass_in[9];
Dalnost=(mass_in[10]<<8)+mass_in[11];
//перетворення в знакові величини
Horizont_ygol=convert(Horizont_ygol); 				
Vertical_ygol=convert(Vertical_ygol);
//перетворення в строку
sprintf(str_1, "%d", Horizont_ygol); 
sprintf(str_2, "%d", Vertical_ygol);
sprintf(str_3, "%d", Dalnost);
//перекодування в формат сегментного індикатора
	for(i=0;i<6;i++) 
	{
	str_1[i]=code_digit[str_1[i]];
	str_2[i]=code_digit[str_2[i]];
	str_3[i]=code_digit[str_3[i]];
	}
//однобітні команди (на світлодіоди), перезаписуємо 4 розряд
if((mass_in[12]&3)==1)  {str_3[4]=str_3[4]+1;  } 		//a 	ціль 1  
if((mass_in[12]&3)==2)  {str_3[4]=str_3[4]+2;  }		//b 	ціль 2  
if((mass_in[12]&3)==3)  {str_3[4]=str_3[4]+4;  }		//c 	ціль 3  
if((mass_in[12]&4)==4)  {str_3[4]=str_3[4]+8;  } 		//d 	Вивірка 
if((mass_in[12]&8)==8)  {str_3[4]=str_3[4]+16; } 		//e 	Ніч
if((mass_in[12]&16)==16){str_3[4]=str_3[4]+32; } 		//f 	День
if((mass_in[12]&32)==32){str_3[4]=str_3[4]+64; }   	//g 	Готов ЛД
if((mass_in[12]&64)==64){str_3[4]=str_3[4]+128;}  	//h 	АВАРІЯ
}	
//*************************************************************************// 
void display(void)
{
Transmit_str1();
Transmit_str2();
Transmit_str3();									
}
//*************************************************************************//перетворення в знакові величини
int convert(int a) 
{
  if (a < 0)
    a = ( ~-a|32768 ) + 1;
  return a;
}
//*************************************************************************// Преривання УАПП
INTERRUPT(UART0_ISR, INTERRUPT_UART0) 
{
   if (RI0 == 1 ) 				//якщо преривання по прийому
   { 
    TL1 = 0x1E; 				//перезагрузка таймера 1
	TH1 = 0xFB;	
	RX_BYTE=SBUF0; 				// читаємо з регістру даних УАПП
		if(start_fl==1) 		//якщо дозволено початок запису даних
		{
		mass_in[number]=SBUF0;  //запис даних в масив
		number++;
			if(number==13)		//якщо прийняли 13 байт 
			{
			start_fl=0;  		// дозволяємо прийом наступної посилки
			finish_fl=1; 		//установка фрагу завершення прийому
			} 
		}

	RI0 = 0; 					//зкидання флагу преривання по прийому УАПП
   }

   if (TI0 == 1)          // Check if transmit flag is set
   {     
    TI0 = 0;                // Clear interrupt flag
   }
}

INTERRUPT(TIMER0_ISR, INTERRUPT_TIMER0) //реалізована функція затримки (період переповнення 1мс)
{
TF0=0; //зкидання флагу преривання таймера 0
delay_cnt++;
}
INTERRUPT(TIMER1_ISR, INTERRUPT_TIMER1) //реалізована функція пошуку початку посилки даних (період переповнення 5мс)
{										//якщо переповнився то можна записувати наступні дані
TF1=0; //зкидання флагу преривання таймера 1
TL1       = 0x1E; //перезавантаження значення таймера
TH1       = 0xFB;
number=0;		//зкидання номера початкового байта в масиві отриманих даних на нульовий
start_fl=1;		//дозвіл записувати дані в масив
cnt_tim1++;		//рахуємо кількість переповнень поспіль. (потрібно якщо немає прийому)
}
//*************************************************************************//Затримка
void Delay_ms (int delay_ms)
{	
TR0=1; 								//запуск таймера 0 
delay_cnt=0; 						//зкидання лічильника преривань таймера 0	
while(delay_cnt<=delay_ms){} 		//очікування необхідної кількості переповнень
TR0=0; 								//зупинка таймера 0
}
//*************************************************************************//
void SPI_Byte_Write (U8 SPI_WRITE)
{
	SPIF=0;
 	while (!TXBMT);  
    SPI0DAT = SPI_WRITE;
	while (!SPIF); 
}

//*************************************************************************// печать десятичного числа
void STLED_on(void) 
{
CS_1=0;
SPI_Byte_Write(0x0D);
CS_1=1;

CS_2=0;
SPI_Byte_Write(0x0D);
CS_2=1;

CS_3=0;
SPI_Byte_Write(0x0D);
CS_3=1;
}
//*************************************************************************//
void Transmit_str1(void) 
{
CS_1=0;
SPI_Byte_Write(0x00); //вкл авто нарощування адреси
//SPI_Byte_Write(0x00); //перший  розряд пустий (не задіяний)
for(i=0;i<6;i++)
	{
	SPI_Byte_Write(str_1[i]);
	}
CS_1=1;
}
//*************************************************************************//
void Transmit_str2(void) 
{
CS_2=0;
SPI_Byte_Write(0x00); //вкл авто нарощування адреси
//SPI_Byte_Write(0x00); //перший  розряд пустий (не задіяний)
for(i=0;i<6;i++)
	{
	SPI_Byte_Write(str_2[i]);
	}
CS_2=1;
}
//*************************************************************************//
void Transmit_str3(void) 
{
CS_2=0;
SPI_Byte_Write(0x00); //вкл авто нарощування адреси
//SPI_Byte_Write(0x00); //перший  розряд пустий (не задіяний)
for(i=0;i<5;i++)
	{
	SPI_Byte_Write(str_3[i]);
	}
CS_2=1;
}


//*************************************************************************//Ініціалізація периферії
void Reset_Sources_Init()
{
    RSTSRC    = 0x06;
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

	Reset_Sources_Init();
    Timer_Init();
    UART_Init();
    SPI_Init();
    Port_IO_Init();
    Oscillator_Init();
    Interrupts_Init();
}
