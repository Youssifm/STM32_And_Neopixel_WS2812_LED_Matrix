#include <stm32f10x.h>
#include <stdbool.h>		

#define GPIO_PWM_PIN				GPIO_Pin_2
#define GPIO_USART_TX_PIN			GPIO_Pin_9
#define GPIO_USART_RX_PIN			GPIO_Pin_10
#define GPIO_ADC_PIN				GPIO_Pin_0

#define PWM_TIM_PERIOD				29								// Number of CPU cycles that will constitute 1 period
#define PWM_HIGH_WIDTH				17								// Duty cycle of pwm signal for a logical 1 to be read by the ws2812 chip. Duty cycle = PWM_HIGH_WIDTH/TIM_PERIOD*100
#define PWM_LOW_WIDTH				9								// Duty cycle of pwm signal for a logical 0 to be read by the ws2812 chip. Duty cycle = PWM_LOW_WIDTH/TIM_PERIOD*100

#define COLUMBS						8
#define	ROWS						1

/* Buffer that holds one complete DMA transmission.
 *  
 * The buffer size can be calculated as followas:
 * number of LEDs * 24 bytes + 42 bytes.
 * 
 * This leaves us with a maximum string length of
 * (2^16 bytes per DMA stream - 42 bytes)/24 bytes per LED = 2728 LEDs.
 */
 
#define LED_COUNT			COLUMBS*ROWS
#define LED_BUFFER_SIZE		24*LED_COUNT+42			// Buffer size needs to be the number of LEDs times 24 bits plus 42 trailing bit to signify the end of the data being transmitted.

extern uint8_t rgb[LED_COUNT][3];					//Array that will store color data
extern uint8_t led_Colors[LED_COUNT];				//Array of integres that will function as indexes for the rgb array
extern uint16_t ledBuff[LED_BUFFER_SIZE];			//Array of data to be sent to leds.
extern uint32_t usart_Baud_Rate;
extern char usartBTBuffer[25];
extern bool config_BT;

extern char usartBuff[LED_BUFFER_SIZE+100];			// Debugging buffer
