#include <stm32f10x.h>
#include <stdio.h>								// For sprintf()  function
#include <stdlib.h>								// For rand() 		function
#include <math.h>								// For floor() 		function

#define GPIO_PWM_PIN		GPIO_Pin_2
#define GPIO_USART_PIN		GPIO_Pin_9

#define TIM_PERIOD			29					// Number of CPU cycles that will constitute 1 period
#define PWM_HIGH_WIDTH		17					// Duty cycle of pwm signal for a logical 1 to be read by the ws2812 chip. Duty cycle = PWM_HIGH_WIDTH/TIM_PERIOD*100
#define PWM_LOW_WIDTH		9					// Duty cycle of pwm signal for a logical 0 to be read by the ws2812 chip. Duty cycle = PWM_LOW_WIDTH/TIM_PERIOD*100

#define COLUMBS				30
#define	ROWS				8

#define LED_COUNT			COLUMBS*ROWS

#define LED_BUFFER_SIZE		24*LED_COUNT+42		// Buffer size needs to be the number of LEDs times 24 bits plus 42 trailing bit to signify the end of the data being transmitted.

/* Buffer that holds one complete DMA transmission.
 *  
 * The buffer size can be calculated as followas:
 * number of LEDs * 24 bytes + 42 bytes.
 * 
 * This leaves us with a maximum string length of
 * (2^16 bytes per DMA stream - 42 bytes)/24 bytes per LED = 2728 LEDs.
 */
uint8_t rgb[LED_COUNT][3];						//Array that will store color data

uint8_t led_Colors[LED_COUNT];					//Array of integers that will function as indexes for the rgb array
uint16_t ledBuff[LED_BUFFER_SIZE];				//Array of data to be sent to leds.

// Function Prototypes
void Peripheral_Config(void); 
void RCC_Config(void);
void GPIO_Config(void);
void TIM_Config(void);
void DMA_Config(void);
void ADC_Config(void);
void send_data(uint8_t (*led_Colors), uint16_t len);
void random_Noise(float r, float g, float b);
void converge_Center(float r, float g, float b);
void rainbow_Loop(void);

void Delay(__IO uint32_t nCount) { while(nCount--) { } }

int main() {

	uint8_t animation_Select;
	uint16_t i;
	
	// Configure peripherals
	Peripheral_Config();
	
	// Reset LEDs
	send_data(led_Colors, LED_COUNT);

	animation_Select = 1;
	
	while (1){  
		switch(animation_Select) {
			case(1) : { random_Noise(0.1, 0.2, 0); break;}
			case(2) : { converge_Center(0, 0.5, 0); break; }
			case(3) : { rainbow_Loop(); break;}
		}
	}
}

void Peripheral_Config(void) {
	RCC_Config();
	GPIO_Config();
	USART_Config();
	TIM_Config();
	DMA_Config();
}

void RCC_Config(void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 , ENABLE);		// Enable clock for GPIOA and USART1
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);								// Enable clock for TIM2
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);									// Enable clock for DMA1
}

void GPIO_Config(void) {

	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Pin = GPIO_PWM_PIN | GPIO_USART_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void USART_Config(void) {
	
	USART_InitTypeDef USART_InitStruct;
	
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx;
	
	USART_Init(USART1, &USART_InitStruct);
	
	USART_Cmd(USART1, ENABLE);
}



void TIM_Config(void) {
	
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStruct;
	TIM_OCInitTypeDef  TIM_OCInitStruct;

	uint16_t PrescalerValue = (uint16_t)(72000000 / 24000000) - 1;					// PrescalerValue = 2, Clock is scaled down to 72MHz /(PrescalerValue + 1) = 762MHz/3 = 24MHz
		
	/* Time base configuration */
	TIM_TimeBaseStruct.TIM_Period = TIM_PERIOD; // 800kHz 							// Species the period value
	TIM_TimeBaseStruct.TIM_Prescaler = PrescalerValue;								// Specifies the prescaler value used to divide the TIM clock.
	TIM_TimeBaseStruct.TIM_ClockDivision = 0;										// Specifies the clock division.
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;						//
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);

	/* PWM1 Mode configuration: Channel1 */
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;									// Specifies the TIM mode.
  TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;						//
	TIM_OCInitStruct.TIM_Pulse = 0;
  TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM2, &TIM_OCInitStruct);
	TIM_Cmd(TIM2, ENABLE);
	
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
	
}

void DMA_Config(void) {
	
	DMA_InitTypeDef DMA_InitStruct;
	
	DMA_DeInit(DMA1_Channel1);														// Deinitialize DAM1 Channel 1 to their default reset values.
	
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&TIM2->CCR3; 					// Specifies Physical address of the peripheral in this case Timer 2 CCR1
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)&ledBuff;							// Specifies the buffer memory address
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;									// Data transfered from memory to peripheral
	DMA_InitStruct.DMA_BufferSize = LED_BUFFER_SIZE;								// Specifies the buffer size
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					// Do not incrament the peripheral address
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;							// Incrament the buffer index
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;		// Specifies the peripheral data width
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;				// Specifies the memory data width
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;										// Specifies the operation mode. Normal or Circular
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;								// Specifies the software priority
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;										//
	
	DMA_Init(DMA1_Channel1, &DMA_InitStruct);										// Initialize DAM1 Channel 1 to values specified in the DMA_InitStruct structure.
	
	TIM_DMACmd(TIM2, TIM_DMA_CC3, ENABLE);											// Enables TIM2's DMA request. TIM_DMA_CC1 : TIM Capture Compare 1 DMA source 

}

void send_data(uint8_t *led_Colors, uint16_t len) {
	
	
	uint8_t i, j, k = 0;
	uint16_t buffersize = (24*len)+42, memaddr = 0;

	
	uint8_t temp, led = 0;
	
	while(len) {		
		for (i = 0; i < 3; i++) {											// Set RGB LED color R -> i=0, G -> i=1, B -> i=2
			temp = rgb[led_Colors[led]][i];
			for (j = 0; j < 8; j++) {										// Set 8 bits of color
				if ((temp) & 0x80) {										// Data sent MSB first, j = 0 is MSB j = 7 is LSB	
					ledBuff[memaddr++] = PWM_HIGH_WIDTH; 					// Compare value for logical 1
				} else {
					ledBuff[memaddr++] = PWM_LOW_WIDTH;						// Compare value for logical 0
				}	
					temp = temp << 1;
			}
		}
			
		led++;
		len--;
	}
	
	// Add needed delay at end of byte cycle, pulsewidth = 0
	while(memaddr < buffersize) {
		ledBuff[memaddr++] = 0;
	}
		
	DMA_SetCurrDataCounter(DMA1_Channel1, LED_BUFFER_SIZE); 	// load number of bytes to be transferred
	DMA_Cmd(DMA1_Channel1, ENABLE); 							// enable DMA channel 1
	TIM_Cmd(TIM2, ENABLE); 										// enable Timer 2
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC1)); 					// wait until transfer complete
	TIM_Cmd(TIM2, DISABLE); 									// disable Timer 2
	DMA_Cmd(DMA1_Channel1, DISABLE); 							// disable DMA channel 1
	DMA_ClearFlag(DMA1_FLAG_TC1); 								// clear DMA1 Channel 1 transfer complete flag
}


/********random_Noise********random_Noise********random_Noise********random_Noise********random_Noise********
* Purpose:			Fluctuate LED brightness to simulate noise.
* Parameters:		Three floats that range from [0,1]. These values represent 
*								how bright a specific color should be. This allows us to 
*								chose whatever color of noise we want.
*	Note:					This function can easily turn all the LED a specific color
*								with a constant brightness, such as full brightness white
*								which requires the mose current, by simply setting the 
*								"noise" value which ranges from [0,255].
*/
void random_Noise(float r, float g, float b) {
	
	uint8_t i, noise;
	
	for(i = 0; i < LED_COUNT; i++) {
		noise = rand() % 255;
		rgb[i][0] = (uint8_t)floor(r*noise);
		rgb[i][1] = (uint8_t)floor(g*noise);
		rgb[i][2] = (uint8_t)floor(b*noise);
		led_Colors[i] = rand() % LED_COUNT+1;
	}
	
	// Send data to LEDs
	send_data(led_Colors, LED_COUNT);	
	
	for(i = 0; i < 0.5*10; i++) {
		Delay(50000L);
	}
}
/********END random_Noise********END random_Noise********END random_Noise********END random_Noise********/


// Not working on matrix shown on youtube video
void converge_Center(float r, float g, float b) {
	uint8_t i, j, temp = 0;
	
	rgb[0][0] = 0;
	rgb[0][1] = 0;
	rgb[0][2] = 0;
	rgb[1][0] = (uint8_t)floor(r*255);
	rgb[1][1] = (uint8_t)floor(g*255);
	rgb[1][2] = (uint8_t)floor(b*255);
	
	
	for(i = 0; i < ROWS/2; i++) {
		for(j = 0; j < COLUMBS; j++) {
			led_Colors[/*temp+*/j-1] = 0;	
			led_Colors[(COLUMBS-1)*8+j] = 0;
		}
	
		if(i > 0 && ((ROWS-1)-i) < (ROWS-1)){
				led_Colors[i-1] = 1;
				led_Colors[(ROWS-1)-i+1] = 1;
		}
	
		// Send data to LEDs
		send_data(led_Colors, LED_COUNT);	

		for(j = 0; j < 1.5*10; j++) {
			Delay(50000L);
		}
	}
}

void rainbow_Loop(){
	
	uint8_t j, intStageNum = 0, r = 255, g = 0, b = 0;
	uint16_t i;
	
	for(i = 0; i < 255*5; i++) {
		switch (intStageNum) {
			case 0 : { g++; if(g == 255){ intStageNum++;}  break;}     //'To change from Red to Yellow, just Green needs to go from 0 to 255 so we do this
			case 1 : { r--; if(r == 0){ intStageNum++;} break;}				//'we are now at (255, 255, 0) yellow, to go to green we need to reduce red to 0
			case 2 : { b++; if(b == 255){ intStageNum++;} break; }
			case 3 : { g--; if(g == 0){ intStageNum++;} break;}		// 'now at (0, 255, 255)... for blue reduce green to 0
			case 4 : {r++; if(r == 255){ intStageNum++; break;} }		// 'now at (0, 0, 255)... for magenta increase red to 255
			case 5 : { b--; if(b == 0){ intStageNum++;} break;}		//'now at (255, 0, 255)... and back to red reduce B to 0
			default : intStageNum = 0;
			}
		rgb[0][0] = (uint8_t)floor(r/20);
		rgb[0][1] = (uint8_t)floor(g/20);
		rgb[0][2] = (uint8_t)floor(b/20);	
		led_Colors[0] = 0;
		// Send data to LEDs
		send_data(led_Colors, LED_COUNT);	
		for(j = 0; j < 0.1*10; j++) {
			Delay(50000L);
		}
	}
}
