#include "vars.h"// Global Variables

uint8_t rgb[LED_COUNT][3];              //Array that will store color data
uint8_t led_Colors[LED_COUNT];          //Array of integres that will function as indexes for the rgb array
uint16_t ledBuff[LED_BUFFER_SIZE];      //Array of data to be sent to leds.
uint32_t usart_Baud_Rate;
char usartBTBuffer[25];
bool config_BT;

char usartBuff[LED_BUFFER_SIZE+100];
