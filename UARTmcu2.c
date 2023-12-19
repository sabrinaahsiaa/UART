#include "PLL.h"
#include "UART1.h"
#include "tm4c123gh6pm.h"
#include "stdint.h"
#include "Nokia5110.h"

#define LED (*((volatile unsigned long *)0x40025038))  // use onboard three LEDs: PF321

#define DARK		0    	
#define RED   	0x02  	
#define BLUE  	0x04  	
#define GREEN 	0x08  	      
#define WHITE   0x0E	
#define PURPLE  0x06	
#define NUM_COLORS 	6

extern void DisableInterrupts(void);
extern void EnableInterrupts(void);  
extern void WaitForInterrupt(void);
	
unsigned char mode, i;
char message[255];  // global to assist in debugging
char ackmessage[255]="I received: ";
char color;
char choice;
const uint32_t colors[] = {RED, BLUE, GREEN, WHITE, PURPLE, DARK};

//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART1_OutChar(CR);
  UART1_OutChar(LF);
}
//debug code
int main(void){
	DisableInterrupts();
  PLL_Init();
	UART1_Init();
	GPIOPortF_Init();
	Nokia5110_Init();
	Nokia5110_Clear();
	EnableInterrupts(); 
	LED = DARK;
  while(1){
		mode = UART1_InChar();
		Nokia5110_SetCursor(1, 1);
		switch(mode){
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

// Port F button handler
unsigned char deactivate;
void GPIOPortF_Handler(void){
	for (uint32_t time=0;time<72724;time++)
	deactivate = 0;
	if (mode == '2'){
		if(GPIO_PORTF_RIS_R&0x01){ // switch 2
			GPIO_PORTF_ICR_R = 0x01;
			UART1_OutChar(color);
			deactivate = 1;
			mode = '0';
		}
		while(GPIO_PORTF_RIS_R&0x10 && deactivate == 0){ // switch 1
			GPIO_PORTF_ICR_R = 0x10;
			LED = DARK;
			LED = colors[i];
			reset_color();
			color = LED;
		}
	}
	if (mode == '3'){
		if(GPIO_PORTF_RIS_R&0x10){ // switch 1
			GPIO_PORTF_ICR_R = 0x10;
			UART1_OutChar('-');
		}
	}
}

void reset_color(void){
	i++;
	i=i%NUM_COLORS;
}

// Receive color info from MCU 1 and change LED on MCU 2
void Mode_2(void){
	mode = '2';
	color = UART1_InChar();
	LED = color;
}

// Turn off LED, receive message from MCU 1, output message to LCD
void Mode_3(void){
	mode = '3';
	LED = DARK;
	UART1_InString(message, 255);
	Nokia5110_Clear();
	Nokia5110_OutString(message);
	WaitForInterrupt();
}
