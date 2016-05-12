#include <stm32f10x.h>

#include "vars.h"						// Global Variables

//Prototypes
void Peripheral_Config(); 
void RCC_Config(void);
void GPIO_Config(void);
void TIM_Config(void);
void USART_Config(uint32_t);
void DMA_Config(void);

void Peripheral_Config() {
	
		RCC_Config();
		GPIO_Config();
		USART_Config(usart_Baud_Rate);
	if(!config_BT) {
		TIM_Config();
		DMA_Config();
	}
}

void RCC_Config(void) {
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 , ENABLE);		// Enable clock for GPIOA and USART1
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);								// Enable clock for TIM2
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);									// Enable clock for DMA1
}

void GPIO_Config(void) {

	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Pin = GPIO_PWM_PIN | GPIO_USART_TX_PIN | GPIO_USART_RX_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void USART_Config(uint32_t usart_Baud_Rate) {
	
	USART_InitTypeDef USART_InitStruct;
	
	USART_InitStruct.USART_BaudRate = usart_Baud_Rate;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
	USART_Init(USART1, &USART_InitStruct);
	USART_Cmd(USART1, ENABLE);
}

void TIM_Config(void) {
	
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStruct;
	TIM_OCInitTypeDef  TIM_OCInitStruct;

	uint16_t PrescalerValue = (uint16_t)(72000000 / 24000000) - 1;			// PrescalerValue = 2, Clock is scaled down to 72MHz /(PrescalerValue + 1) = 762MHz/3 = 24MHz
		
	/* PWM Time base configuration */
	TIM_TimeBaseStruct.TIM_Period = PWM_TIM_PERIOD;						 	// Species the period value
	TIM_TimeBaseStruct.TIM_Prescaler = PrescalerValue;						// Specifies the prescaler value used to divide the TIM clock.
	TIM_TimeBaseStruct.TIM_ClockDivision = 0;								// Specifies the clock division.
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;				//
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);

	/* PWM1 Mode configuration: Channel1 */
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;							// Specifies the TIM mode.
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;				//
	TIM_OCInitStruct.TIM_Pulse = 0;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM2, &TIM_OCInitStruct);
	TIM_Cmd(TIM2, ENABLE);
	
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
}

void DMA_Config(void) {
	
	DMA_InitTypeDef DMA_InitStruct;
	
	DMA_DeInit(DMA1_Channel1);															// Deinitialize DAM1 Channel 1 to their default reset values.
	
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&TIM2->CCR3; 						// Specifies Physical address of the peripheral in this case Timer 2 CCR1
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)&ledBuff;								// Specifies the buffer memory address
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;										// Data transfered from memory to peripheral
	DMA_InitStruct.DMA_BufferSize = LED_BUFFER_SIZE;									// Specifies the buffer size
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;						// Do not incrament the peripheral address
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;								// Incrament the buffer index
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;			// Specifies the peripheral data width
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;					// Specifies the memory data width
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;											// Specifies the operation mode. Normal or Circular
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;									// Specifies the software priority
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;											//
	
	DMA_Init(DMA1_Channel1, &DMA_InitStruct);											// Initialize DAM1 Channel 1 to values specified in the DMA_InitStruct structure.
	TIM_DMACmd(TIM2, TIM_DMA_CC3, ENABLE);												// Enables TIM2's DMA request. TIM_DMA_CC1 : TIM Capture Compare 1 DMA source 
}

