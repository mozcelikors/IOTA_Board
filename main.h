/*
License:
	IOTA_Client is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	IOTA_Client is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with IOTA_Client.  If not, see <http://www.gnu.org/licenses/>.

Description:
	This software is used to create serial protocol between a STM32F103
	processor and an ESP8266-12 wifi device, interface with an OLED and
	Rotary encoder for HMI.

Authors: 
	Mustafa Ozcelikors 
		thewebblog.net
		github.com/mozcelikors
		<mozcelikors@gmail.com>

*/
/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "MO_Commands.h"

#define LEFT 1
#define RIGHT 2
#define NO_MOVEMENT 0

#define UART_RX_BUFFER_SIZE 1
#define UART_TX_BUFFER_SIZE 3
#define UART_LINE_BUFFSIZE 1024

//Pins
#define               WEMOS_HW_RST_PORT 						GPIOB
#define               WEMOS_HW_RST_PIN  						GPIO_PIN_15
#define		            USER_LED_PORT 								GPIOA //GPIOC
#define               USER_LED_PIN 									GPIO_PIN_1//GPIO_PIN_13
#define		            RELAY_PORT 										GPIOB 
#define               RELAY_PIN 										GPIO_PIN_12
#define               ROTARY_PORT 									GPIOB
#define               ROTARY_A_PIN									GPIO_PIN_5
#define               ROTARY_B_PIN									GPIO_PIN_6
#define               ROTARY_SW_PIN									GPIO_PIN_7

extern int OLED_STATE;
extern int LED_STATE;
extern uint16_t ticks;
extern uint8_t interrupts_exti9_5_flag;
extern uint16_t ctr10ms;
extern uint8_t Rotary_Direction;
extern uint16_t Rotary_Counter;

extern uint8_t uart_receive_buffer[UART_RX_BUFFER_SIZE];
extern uint8_t uart_transmit_buffer[UART_TX_BUFFER_SIZE];
extern uint8_t uart_line_buffer[UART_LINE_BUFFSIZE];
extern uint8_t UART_RECEIVED_COMMAND[UART_LINE_BUFFSIZE];
extern uint8_t uart_line_ready;
extern int k;

extern 		uint8_t								ROTARY_EVENT;

extern		uint8_t              	SSID [DATA_BUFFSIZE];
extern		uint8_t              	HOST [DATA_BUFFSIZE];

extern UART_HandleTypeDef huart1;
extern Receive_Command_Type 	RECEIVED_COMMAND;
extern uint8_t              	RECEIVED_DATA [DATA_BUFFSIZE];