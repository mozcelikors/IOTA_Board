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
#include "stm32f1xx_hal.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define DATA_BUFFSIZE  128

/****
 *   Protocol File for IOTA
 *   @author mozcelikors <mozcelikors@gmail.com>
 *
 *   ID -> Idle
 *   WR -> Connect to WiFi network Request
 *   WW -> Waiting for WiFi network
 *   RW -> Request Wifi network connection info  
 *   WI<Data># -> Wifi network connection info <Data>: 0-not connected 1-connected
 *   WF -> Connect to WiFi network Failed
 *   WS -> Connect to WiFi network Successful
 *   HR -> Connect to Host Request
 *   HW -> Waiting to Connect the Host
 *   HF -> Connect to Host Failed
 *   HS -> Connect to Host Successful
 *   HH-> Request Host connection info  
 *   HI<Data># -> Host connection info <Data>: 0-not connected 1-connected
 *   SR<Data># -> Send to server request
 *   SS -> Send to server successful
 *   RR -> Receive from server (Request)
 *   RS<Data># -> Receive from server successful
 *   CR -> Close Connection Request
 *   CS -> Close Connection Successful
 */

typedef enum { _ID_, _WR_, _RW_, _WI_, _WF_, _WW_, _WS_, _HH_, _HI_, _HR_, _HW_, _HF_, _HS_, _SR_, _SS_, _RR_, _RS_, _CR_, _CS_ } Receive_Command_Type;

Receive_Command_Type ParseReceivedCommand(uint8_t * command);
void AssignReceivedData (int word_length);