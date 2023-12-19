#include "PLL.h"
#include "UART.h"
#include "UART1.h"
#include "tm4c123gh6pm.h"
#include "stdint.h"

#define LED (*((volatile unsigned long *)0x40025038))  // use onboard three LEDs: PF321

#define DARK		0    	
#define RED   	0x02  	
#define BLUE  	0x04  	
#define GREEN 	0x08  	
//#define YELLOW  	
//#define CRAN      
#define WHITE   0x0E	
#define PURPLE  0x06	
#define NUM_COLORS 	6

extern void DisableInterrupts(void);
extern void EnableInterrupts(void);  
extern void WaitForInterrupt(void);

unsigned char i;
char mode[1];
char message[255];  // global to assist in debugging
char ackmessage[255] = "I Received: ";
char color[1];
const uint32_t colors[] = {RED, BLUE, GREEN, WHITE, PURPLE, DARK};
char test, c;
//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART0_OutChar(CR);
  UART0_OutChar(LF);
}
//debug code
int main(void){
	DisableInterrupts();
  PLL_Init();
  UART0_Init();              // initialize UART0
	UART1_Init();
	GPIOPortF_Init();
	EnableInterrupts(); 
  while(1){
		Display_Menu();
		UART0_InString(mode, 1);
		OutCRLF();
		switch(mode[0]){
			case '1':
				Mode_1();
				break;
			case '2':
				Mode_2();
				break;
			case '3':
				Mode_3();
				break;
			default:
				break;
		}
  }
}

void Mode_1(void){
	UART0_OutString("Please enter r(red), g(green), b(blue), p(purple), w(white), d(dark)");
	OutCRLF();
	UART0_InString(color, 1);
	OutCRLF();
	switch(color[0]){
		case 'r':
			LED = RED;
			UART0_OutString("Red LED is on");
			OutCRLF();
			break;
		case 'g':
			LED = GREEN;
			UART0_OutString("Green LED is on");
			OutCRLF();
			break;
		case 'b':
			LED = BLUE;
			UART0_OutString("Blue LED is on");
			OutCRLF();
			break;
		case 'p':
			LED = PURPLE;
			UART0_OutString("Purple LED is on");
			OutCRLF();
			break;
		case 'w':
			LED = WHITE;
			UART0_OutString("White LED is on");
			OutCRLF();
			break;
		case 'd':
			LED = DARK;
			UART0_OutString("Dark LED is on");
			OutCRLF();
			break;
		default:
			break;
	}
	return;
	LED = color[0];
}

void Display_Menu(void){
	UART0_OutString("Welcome to CECS 447 Project 2 - UART");
	OutCRLF();
	UART0_OutString("Please choose a communication mode (type 1 or 2 or 3 followed by return):");
	OutCRLF();
	UART0_OutString("		1. PC <-> MCU_1 only");
	OutCRLF();
	UART0_OutString("		2. MCU_1 <-> MCU_2 LED Control");
	OutCRLF();
	UART0_OutString("		3. PC <-> MCU_1 <-> MCU_2 Messenger");
	OutCRLF();
}

void GPIOPortF_Init(void){
	SYSCTL_RCGC2_R |= 0x00000020;     	// activate F clock
	while ((SYSCTL_RCGC2_R&0x00000020)!=0x00000020){} // wait for the clock to be ready
		
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   	// unlock PortF PF0  
	GPIO_PORTF_CR_R |= 0x1F;         		// allow changes to PF4-0 :11111->0x1F     
  GPIO_PORTF_AMSEL_R &= ~0x1F;        // disable analog function
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF; 	// GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R &= ~0x11;          // PF4,PF0 input   
  GPIO_PORTF_DIR_R |= 0x0E;          	// PF3,PF2,PF1 output   
	GPIO_PORTF_AFSEL_R &= ~0x1F;        // no alternate function
  GPIO_PORTF_PUR_R |= 0x11;          	// enable pullup resistors on PF4,PF0       
  GPIO_PORTF_DEN_R |= 0x1F;          	// enable digital pins PF4-PF0  
	
	// Initialize edge-triggered interrupts.
	GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4, PF0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF4, PF0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    //     PF4, PF0 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag
  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4, PF0
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF1FFFFF)|0x00400000; // (g) priority 1
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
}

unsigned char deactivate;
void GPIOPortF_Handler(void){
	for (uint32_t time=0;time<72724;time++)
	deactivate = 0;
	if (mode[0] == '2'){
		if(GPIO_PORTF_RIS_R&0x01){ // switch 2
			GPIO_PORTF_ICR_R = 0x01;
			UART1_OutString(mode);
			UART1_OutChar(c);
			deactivate = 1;
		}
		while(GPIO_PORTF_RIS_R&0x10 && deactivate == 0){ // switch 1
			GPIO_PORTF_ICR_R = 0x10;
			LED = DARK;
			LED = colors[i];
			reset_color();
			c = LED;
		}
	}
}

void reset_color(void){
	i++;
	i=i%NUM_COLORS;
}

void Mode_2(void){
	mode[0] = '2';
	c = UART1_InChar();
	LED = c;
	mode[0] = '0';
}

void Mode_3(void){
	mode[0] = '3';
	LED = GREEN;
	UART0_OutString("Enter a message: ");
	UART0_InString(message, 255);
	UART1_OutString(mode);
	UART1_OutString(message);
	UART1_OutChar(CR);
	OutCRLF();
	while(test != '-'){
		test = UART1_InChar();
		if(test == '-'){
			LED = DARK;
			mode[0] = '0';
			UART0_OutString(ackmessage);
			UART0_OutString(message);
			OutCRLF();
		}
	}
}
