#include "compiler_defs.h"
#include "C8051F560_defs.h"
#include "font_X.h"

#include <stdio.h>

sbit RST = P1^5;
sbit DC = P1^4;
sbit CS = P1^3;
//sbit LED0 = P2^2;
//sbit LED1 = P2^3;

unsigned char RX_BYTE;		//змінна в яку читаємо з SBUF0 в прериванні
unsigned char number; 		//лічильник байт
unsigned char cnt_tim1; 	//лічильник переповнень таймера 1
bit  start_fl,finish_fl; 	//флаги готовності прийому даних
int mass_in[13];			//буфер отриманих по УСАПП даних
int delay_cnt; 				//лічильник переповнень таймера затримки
xdata unsigned char lcd8544_buff[84*6]; // буфер дисплея
unsigned int i;
int Horizont_ygol,Vertical_ygol,Dalnost; //
char str_Horizont_ygol[6];  //строки для передачі на дисплей
char str_Vertical_ygol[6];
char str_Dalnost[5];
 //.......................................................................//
 //Прототипи функцій
 //.......................................................................//
void lcd8544_init(void); // инициализация дисплея
void lcd8544_sendCmd(unsigned char cmd);	// отправка команды на дисплей		
void lcd8544_refresh(void);// очистка дисплея		
void lcd8544_clear(void);// очистка буфера	
void lcd8544_putpix(unsigned char x, unsigned char y, unsigned char mode);	// вывод пиксела		
void lcd8544_line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode);// вывод линии		
void lcd8544_rect(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode);// рисование прямоугольника (не заполненного)		
void lcd8544_putchar(unsigned char px, unsigned char py, unsigned char ch, unsigned char mode);// вывод символа в цвете по координатам	
void lcd8544_leftchline(unsigned char x, unsigned char y);	// линия левее символа для печати в инверсе
void lcd8544_putstr(unsigned char x, unsigned char y, const unsigned char str[], unsigned char mode);// вывод строки в цвете по координатам		
void lcd8544_dec(unsigned int numb, unsigned char dcount, unsigned char x, unsigned char y, unsigned char mode);// печать десятичного числа

void Init_Device(void);
void Delay_ms(int delay_ms);
void read_13_byte(void);
void display_13_byte(void);
int convert(int a);

INTERRUPT_PROTO (UART0_ISR, INTERRUPT_UART0);
INTERRUPT_PROTO (TIMER0_ISR, INTERRUPT_TIMER0);
INTERRUPT_PROTO (TIMER1_ISR, INTERRUPT_TIMER1);

//*************************************************************************//Основна програма
void main(void)
{
Init_Device();						// Початкова настройка мікроконтроллера
SFRPAGE = ACTIVE_PAGE;              // Change for PCA0MD and SBUF0
PCA0MD &= ~0x40;                    // Disable the watchdog timer

lcd8544_init();
lcd8544_clear();
		
		while(1)
		{
			
read_13_byte(); 								 //прийом 13 байт і запис в масив

//***********************************************//складання старшого і молодшого байтів
Horizont_ygol=(mass_in[6]<<8)+mass_in[7]; 			
Vertical_ygol=(mass_in[8]<<8)+mass_in[9];
Dalnost=(mass_in[10]<<8)+mass_in[11];
//***********************************************//перетворення в знакові величини
Horizont_ygol=convert(Horizont_ygol); 				
Vertical_ygol=convert(Vertical_ygol);
//***********************************************//перетворення в строку
sprintf(str_Horizont_ygol, "%d", Horizont_ygol); 
sprintf(str_Vertical_ygol, "%d", Vertical_ygol);
sprintf(str_Dalnost, 	   "%d", Dalnost);

lcd8544_clear();				          	     //очистка екрану



//***********************************************//однобітні команди (на світлодіоди)
if((mass_in[12]&3)==1){lcd8544_putstr(0,30,"Ціль- 1",0);} 	//індикація цілей
if((mass_in[12]&3)==2){lcd8544_putstr(0,30,"Ціль- 2",0);}
if((mass_in[12]&3)==3){lcd8544_putstr(0,30,"Ціль- 3",0);}

//************************************************//

if((mass_in[12]&4)==4)  {lcd8544_putstr(0,40 ,"В",0);} 	//Вивірка
if((mass_in[12]&8)==8)  {lcd8544_putstr(10,40,"Н",0);} 	//Ніч
if((mass_in[12]&16)==16){lcd8544_putstr(20,40,"Д",0);} 	//День
if((mass_in[12]&32)==32){lcd8544_putstr(30,40,"Г",0);} //Готов ЛД
if((mass_in[12]&64)==64){lcd8544_putstr(40,40,"А",0);} //АВАРІЯ

lcd8544_putstr(0,0,str_Horizont_ygol,0);  				//вивід горизонтального угла
lcd8544_putstr(35,0,"Горизонт",0);

lcd8544_putstr(0,10,str_Vertical_ygol,0); 				//вивід вертикального угла
lcd8544_putstr(35,10,"Вертикал",0);

lcd8544_putstr(0,20,str_Dalnost,0); 					//вивід дальності
lcd8544_putstr(35,20,"Віддаль",0);
//lcd8544_dec(Horizont_ygol, 4, 8, 8, 0);				//передача горизонтального угла
//lcd8544_dec(Vertical_ygol, 4, 8, 16, 0); 				//передача вертикального угла
//lcd8544_dec(Dalnost, 4, 8, 24, 0); 					//передача дальності

lcd8544_refresh();										//оновлення зображення на дисплеї	
						
									
for(number=0;number<13;number++){mass_in[number]=0;	}	//очищення прийомного буферу
//display_13_byte(); //індикація 
Delay_ms(100); 
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
	
//*************************************************************************// 
void display_13_byte(void)
{
lcd8544_clear();				             		//очистка екрану	
for(number=0;number<7;number++)		
{
lcd8544_dec(mass_in[number+6], 3, 8, 7*number, 0); 	//передача значень на дисплей
mass_in[number]=0;								 	//очищення прийомного буферу
}	
lcd8544_refresh();									//оновлення зображення на дисплеї										
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
//*************************************************************************//Відправка по SPI
void lcd8544_senddata(char LCDdata)
{ 
	SPIF=0; 				//зкидання флагу преривання
 	while (!TXBMT);  		//очікування спустошення буферу передачі
    SPI0DAT = LCDdata; 	//
	while (!SPIF); 
}
//*************************************************************************// оновлення екрану
void lcd8544_refresh(void) {
	unsigned char y,x;
	CS=0;            			// СS=0 - начали сеанс работы с дисплеем
	DC=0;            			// передача комманд
	lcd8544_senddata(0x40);		// установка курсора в позицию Y=0; X=0
	lcd8544_senddata(0x80);
	DC=1;            			// передача данных
	for (y=0;y<6;y++) for (x=0;x<84;x++) lcd8544_senddata(lcd8544_buff[y*84+x]);
	 Delay_ms(10);
	 CS=1;						// СS=1 - закончили сеанс работы с дисплеем
}
//*************************************************************************//очистка дисплея
void lcd8544_clear(void) {				 
   unsigned char x,y;
   for (y=0;y<6;y++) for(x=0;x<84;x++) lcd8544_buff[y*84+x]=0;
}
//*************************************************************************//Инициализация
void lcd8544_init(void) {			
	CS=0;            					// CS=0  - начали сеанс работы с дисплеем
	DC=0;								// A0=0  - переход в командный режим
										// сброс дисплея
	RST=0;                   			// RST=0
    Delay_ms(1);						// пауза
	RST=1;                   			// RST=1
	Delay_ms(50);						// пауза
//**************************************// инициализация дисплея
	lcd8544_senddata(0x21);      		// переход в расширенный режим
	lcd8544_senddata(0xC1);
	lcd8544_senddata(0x06);				// температурный коэффициент, от 4 до 7
	lcd8544_senddata(0x13);				// Bias 0b0001 0xxx - работает как контрастность
	lcd8544_senddata(0x20); 			// переход в обычный режим
	lcd8544_senddata(0xC);				// 0b1100 - normal mode // 0b1101 - invert mode // 0b1001 - полностью засвеченный экран// 0b1000 - чистый экран                     	                        
	CS=1;
}
//*************************************************************************//вывод пиксела
void lcd8544_putpix(unsigned char x, unsigned char y, unsigned char mode) {
	unsigned int adr=(y>>3)*84+x;
unsigned char dataLCD=(1<<(y&0x07));
	if ((x>84) || (y>47)) return;
	if (mode) lcd8544_buff[adr]|=dataLCD;
   else lcd8544_buff[adr]&=~dataLCD;
}
//*************************************************************************// процедура рисования линии
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
//*************************************************************************// рисование прямоугольника (не заполненного)
void lcd8544_rect(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char mode) {
	lcd8544_line(x1,y1, x2,y1, mode);
	lcd8544_line(x1,y2, x2,y2, mode);
	lcd8544_line(x1,y1, x1,y2, mode);
	lcd8544_line(x2,y1, x2,y2, mode);
}
//*************************************************************************// печать десятичного числа
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
//*************************************************************************// вывод символа на экран по координатам
void lcd8544_putchar(unsigned char px, unsigned char py, unsigned char ch, unsigned char mode) {
	data unsigned char *fontpointer;
 unsigned char lcd_YP=7- (py & 0x07);    // битовая позиция символа в байте
 unsigned char lcd_YC=(py & 0xF8)>>3; 	// байтовая позиция символа на экране
 unsigned char x;

	for (x=0; x<5; x++) {
unsigned char temp=(FontTable[ch][x]);
		if (mode!=0) {
			temp=~temp;
			if (py>0) lcd8544_putpix(px, py-1, 1);	// если печать в режиме инверсии - сверху отчертим линию
		}
		temp&=0x7F;
		lcd8544_buff[lcd_YC*84+px]=lcd8544_buff[lcd_YC*84+px] | (temp<<(7-lcd_YP)); 	// печать верхней части символа
	    if (lcd_YP<7) lcd8544_buff[(lcd_YC+1)*84+px]=lcd8544_buff[(lcd_YC+1)*84+px] | (temp>>(lcd_YP+1)); 	// печать нижней части символа
		px++;
		if (px>83) return;
	}
}
//*************************************************************************// линия левее символа для печати в инверсе
void lcd8544_leftchline(unsigned char x, unsigned char y) {
	if (x>0) lcd8544_line(x-1, y-1, x-1, y+6, 1);
}
//*************************************************************************// вывод строки
void lcd8544_putstr(unsigned char x, unsigned char y,  unsigned char str[], unsigned char mode) {
	if (mode) lcd8544_leftchline(x, y);
	while (*str!=0) {
		lcd8544_putchar(x, y, *str, mode);
		x=x+6;
		str++;
	}
}
//*************************************************************************//Ініціалізація периферії
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
