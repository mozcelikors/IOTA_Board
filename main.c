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

#include "main.h"

/* USER CODE BEGIN Includes */
#include "fonts.h"
#include "ssd1306.h"
#include "usart.h"

//TO INCLUDE FREERTOS .. Comment SysTick Handler in stm32f1xx_it.c 
//Include only one heap from MemMang i.e. heap_1.c, include port.c and portable.h from RVDS/ARM_CM3, do not include croutine.c and croutine.h
#include "cmsis_os.h"

#define configMINIMAL_STACK_SIZE_NEW                ( ( uint16_t ) 64 )
//Yeni tasklar acmak icin yermezse stack size dusurulerek cozum getirilebilir
//Stack tasarsa hata verir. Kodun memoryde kapladigi yer cok asiri olmayacak

//NVIC interruptlari 0-4 arasi FreeRTOS a ayrilir. Normal interruptlar icin
//5 ve 5+ sayilar kullanilmali.

//HAL_UART_Receive ve HAL_UART_Transmit blocking metodlar.
//Dolayisiyla Threadde HAL_UART_Receive varsa herhangi bir yerde
//HAL_UART_Transmit yapamayiz, HAL_BUSY state dondurur.
//Dolayisiyla bunu onlemek icin HAL_UART_Receive_IT ve HAL_UART_Transmit_IT
//kullaniyoruz.

//Modes to work
#define  NO_MODE 0
#define  MANUAL 1
#define  AUTOMATIC 2

//Misc
uint16_t 							ticks = 0;
uint16_t 							ctr10ms = 0;
uint8_t 							interrupts_exti9_5_flag = 1;
uint8_t 							Rotary_Direction = NO_MOVEMENT;
uint16_t 							Rotary_Counter = 5000;
uint16_t 							Prev_Rotary_Counter = 5000;
uint8_t 							uart_transmit_bufferX[6];

//Uart related
#define 							END_OF_WORD_CHAR 						((uint8_t)'#')
uint8_t 							uart_receive_buffer[UART_RX_BUFFER_SIZE];
uint8_t 							uart_transmit_buffer[UART_TX_BUFFER_SIZE];
uint8_t 							uart_line_buffer[UART_LINE_BUFFSIZE];
uint8_t 							UART_RECEIVED_COMMAND[UART_LINE_BUFFSIZE];
uint8_t 							uart_line_ready = 0; //flag
UART_HandleTypeDef 					huart1;
int 								k = 0;

//Received command related
Receive_Command_Type 	RECEIVED_COMMAND;
uint8_t              	RECEIVED_DATA [DATA_BUFFSIZE];
uint8_t              	SSID [DATA_BUFFSIZE];
uint8_t              	HOST [DATA_BUFFSIZE];
uint8_t 				connected_flag = 0;
uint8_t					selected_mode = NO_MODE;
uint8_t					ROTARY_EVENT = 0;

HAL_StatusTypeDef state  = HAL_OK;

typedef enum
{
	THREAD_1 = 0,
	THREAD_2,
	THREAD_3,
	THREAD_4
} Thread_TypeDef;

osThreadId OLEDThreadHandle, UARTReceiveThreadHandle, DataCommunicationThreadHandle, RLCThreadHandle;

static void OLED_Thread(void const *argument);
static void UART_Receive_Thread(void const *argument);
static void Rotary_Led_and_Ctr10ms_Thread(void const *argument);
static void Data_Communication_Thread (void const *argument);

int OLED_STATE = 0;
int LED_STATE = 0;

void SystemClock_Config(void);

void AssignReceivedData (int word_length)
{
		for (int i = 0; i < word_length-2; i++)
		{
				RECEIVED_DATA[i] = UART_RECEIVED_COMMAND[i+2];
		}
}

void AssignSSID (int word_length)
{
		for (int i = 0; i < word_length-2; i++)
		{
				SSID[i] = UART_RECEIVED_COMMAND[i+2];
		}
}

void AssignHOST (int word_length)
{
		for (int i = 0; i < word_length-2; i++)
		{
				HOST[i] = UART_RECEIVED_COMMAND[i+2];
		}
}

void ClearRotaryFlag(void)
{ 
	Rotary_Direction = NO_MOVEMENT;
}

int IsRotaryRight(void)
{
	if (Rotary_Direction == RIGHT)
	{
		return 1;
	}
	else
		return 0;
}

int IsRotaryLeft(void)
{
	if (Rotary_Direction == LEFT)
	{
		return 1;
	}
	else
		return 0;
}


//Use this, this is better than RequestWifiConnectionInfo
void RequestHostConnectionInfo(void)
{
		uint8_t nums_in_tr_buf;
		uart_transmit_buffer[0] = 'H';
		uart_transmit_buffer[1] = 'H';
		uart_transmit_buffer[2] = '#';
		nums_in_tr_buf = sizeof(uart_transmit_buffer)/sizeof(uint8_t);
		state = HAL_UART_Transmit_IT(&huart1, uart_transmit_buffer, nums_in_tr_buf);
}

void RequestWifiConnectionInfo(void)
{
		uint8_t nums_in_tr_buf;
		uart_transmit_buffer[0] = 'R';
		uart_transmit_buffer[1] = 'W';
		uart_transmit_buffer[2] = '#';
		nums_in_tr_buf = sizeof(uart_transmit_buffer)/sizeof(uint8_t);
		state = HAL_UART_Transmit_IT(&huart1, uart_transmit_buffer, nums_in_tr_buf);
}

void RequestData(void)
{
		uint8_t nums_in_tr_buf;
		uart_transmit_buffer[0] = 'R';
		uart_transmit_buffer[1] = 'R';
		uart_transmit_buffer[2] = '#';
		nums_in_tr_buf = sizeof(uart_transmit_buffer)/sizeof(uint8_t);
		state = HAL_UART_Transmit_IT(&huart1, uart_transmit_buffer, nums_in_tr_buf);
}

void SendResponse(void)
{
		//SR<Data>#
		uint8_t nums_in_tr_buf;
		uart_transmit_bufferX[0] = 'S';
		uart_transmit_bufferX[1] = 'R';
		uart_transmit_bufferX[2] = 'A';
		uart_transmit_bufferX[3] = 'C';
		uart_transmit_bufferX[4] = 'K';
		uart_transmit_bufferX[5] = '#';
		nums_in_tr_buf = sizeof(uart_transmit_bufferX)/sizeof(uint8_t);
		state = HAL_UART_Transmit_IT(&huart1, uart_transmit_bufferX, nums_in_tr_buf);
}

int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();

	MX_I2C2_Init();

	MX_USART1_UART_Init();

	/*Configure GPIO pins : PB5 PB6 PB7 */  //Causes rtos to crash..!
	GPIO_InitTypeDef GPIO_InitStruct2;
	GPIO_InitStruct2.Pin = GPIO_PIN_7 | GPIO_PIN_5 | GPIO_PIN_6;
	GPIO_InitStruct2.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct2.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct2);

	//PC13 - LED
	GPIO_InitTypeDef GPIO_InitStruct3;
	GPIO_InitStruct3.Pin = USER_LED_PIN;
	GPIO_InitStruct3.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct3.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct3.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(USER_LED_PORT, &GPIO_InitStruct3);
	HAL_GPIO_WritePin(USER_LED_PORT,USER_LED_PIN,1);

	//Relay
	GPIO_InitTypeDef GPIO_Relay_InitStruct;
	GPIO_Relay_InitStruct.Pin = USER_LED_PIN;
	GPIO_Relay_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_Relay_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Relay_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(RELAY_PORT, &GPIO_Relay_InitStruct);
	HAL_GPIO_WritePin(RELAY_PORT,RELAY_PIN,1);

	//PB15-WeMOS HW RST PIN
	GPIO_InitTypeDef GPIO_InitStruct4;
	GPIO_InitStruct4.Pin = WEMOS_HW_RST_PIN;
	GPIO_InitStruct4.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct4.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct4.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(WEMOS_HW_RST_PORT, &GPIO_InitStruct4);
	HAL_GPIO_WritePin(WEMOS_HW_RST_PORT,WEMOS_HW_RST_PIN,0);  //A quick reset to WeMoS
	HAL_Delay(100);
	HAL_GPIO_WritePin(WEMOS_HW_RST_PORT,WEMOS_HW_RST_PIN,1);
	HAL_Delay(100);

	//Attach interrupt to handler. Check handler.
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	//OLED Init
	uint8_t res = SSD1306_Init();

	//OLED Intro
	for(int i=0; i<2; i++)
	{
			SSD1306_Fill(SSD1306_COLOR_BLACK);
			SSD1306_DrawBitmapFromBuffer();
			HAL_Delay(400);
			
			SSD1306_Fill(SSD1306_COLOR_WHITE);
			SSD1306_DrawBitmapFromBuffer2();
			HAL_Delay(400);
	}

	//OLED Layout and Welcome screen
	SSD1306_Fill(SSD1306_COLOR_WHITE);
	SSD1306_DrawLayout();
	SSD1306_GotoXY(4,20);
	SSD1306_Puts("  Welcome", &Font_11x18, SSD1306_COLOR_WHITE);
	SSD1306_GotoXY(4,44);
	SSD1306_Puts("   to IOTA Home", &Font_7x10, SSD1306_COLOR_WHITE);
	SSD1306_UpdateScreen();

	//Threads
	osThreadDef(THREAD_1, OLED_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE_NEW);

	osThreadDef(THREAD_2, UART_Receive_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE_NEW);

	osThreadDef(THREAD_3, Rotary_Led_and_Ctr10ms_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE_NEW);

	osThreadDef(THREAD_4, Data_Communication_Thread, osPriorityNormal, 0, ( uint16_t ) 16 );

	OLEDThreadHandle = osThreadCreate(osThread(THREAD_1), NULL);

	UARTReceiveThreadHandle = osThreadCreate(osThread(THREAD_2), NULL);  

	RLCThreadHandle = osThreadCreate(osThread(THREAD_3), NULL);  

	DataCommunicationThreadHandle = osThreadCreate(osThread(THREAD_4), NULL);  

	osKernelStart();

	for (;;);

}



static void OLED_Thread(void const *argument)
{
  uint32_t count = 0;
  (void) argument;
	uint8_t nums_in_tr_buf = 0;

  for (;;)
  {
			if (connected_flag == 1)
			{
					if (ROTARY_EVENT == 1 && OLED_STATE == 1)
					{
							//Layout
							SSD1306_Fill(SSD1306_COLOR_WHITE);
							SSD1306_DrawLayout();
						
							//--Update Icon Bar--
							//Disable_Wifi_Icon();
							//Disable_Host_Icon();
							Disable_Data_Icon();
						
							SSD1306_GotoXY(4,18);
							SSD1306_Puts("Select Mode:", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_GotoXY(4,31);
							SSD1306_Puts("MANUAL", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_GotoXY(4,44);
							SSD1306_Puts("AUTOMATIC", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_UpdateScreen();
							ROTARY_EVENT = 0;
					}
					else if (ROTARY_EVENT == 1 && OLED_STATE == 2)
					{
							//Layout
							SSD1306_Fill(SSD1306_COLOR_WHITE);
							SSD1306_DrawLayout();
						
							//--Update Icon Bar--
							//Disable_Wifi_Icon();
							//Disable_Host_Icon();
							Disable_Data_Icon();
						
							SSD1306_GotoXY(4,18);
							SSD1306_Puts("Select Mode:", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_GotoXY(4,31);
							SSD1306_Puts("MANUAL", &Font_7x10, SSD1306_COLOR_BLACK);
							SSD1306_GotoXY(4,44);
							SSD1306_Puts("AUTOMATIC", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_UpdateScreen();
						
							selected_mode = MANUAL;
							ROTARY_EVENT = 0;
					}
					else if (ROTARY_EVENT == 1 && OLED_STATE == 3)
					{
							//Layout
							SSD1306_Fill(SSD1306_COLOR_WHITE);
							SSD1306_DrawLayout();
						
							//--Update Icon Bar--
							//Disable_Wifi_Icon();
							//Disable_Host_Icon();
							Disable_Data_Icon();
						
							SSD1306_GotoXY(4,18);
							SSD1306_Puts("Select Mode:", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_GotoXY(4,31);
							SSD1306_Puts("MANUAL", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_GotoXY(4,44);
							SSD1306_Puts("AUTOMATIC", &Font_7x10, SSD1306_COLOR_BLACK);
							SSD1306_UpdateScreen();
						
							selected_mode = AUTOMATIC;
							ROTARY_EVENT = 0;
					}
					
					if (ROTARY_EVENT == 1 && OLED_STATE == 4)
					{
							//Layout
							SSD1306_Fill(SSD1306_COLOR_WHITE);
							SSD1306_DrawLayout();
						
							//--Update Icon Bar--
							//Disable_Wifi_Icon();
							//Disable_Host_Icon();
							Disable_Data_Icon();
						
							SSD1306_GotoXY(4,18);
							SSD1306_Puts("Mode Selected:", &Font_7x10, SSD1306_COLOR_WHITE);
							if (selected_mode == MANUAL)
							{
									SSD1306_GotoXY(4,31);
									SSD1306_Puts("MANUAL", &Font_7x10, SSD1306_COLOR_WHITE);
							}
							else if (selected_mode == AUTOMATIC)
							{
									SSD1306_GotoXY(4,31);
									SSD1306_Puts("AUTOMATIC", &Font_7x10, SSD1306_COLOR_WHITE);
							}
							SSD1306_UpdateScreen();
							ROTARY_EVENT = 0;
					}
			}
		
			if (RECEIVED_COMMAND == _WW_)
			{
					//Layout
					SSD1306_Fill(SSD1306_COLOR_WHITE);
					SSD1306_DrawLayout();
				
					
					//Connecting
					SSD1306_GotoXY(4,18);
					SSD1306_Puts("Connecting to", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,31);
					SSD1306_Puts("Access Point", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_UpdateScreen();
					for(uint8_t i = 0; i<=2; i++)
					{
							SSD1306_Puts(".", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_UpdateScreen();
							osDelay(200);
					}
			}
			else if (RECEIVED_COMMAND == _WS_)
			{
					//Layout
					SSD1306_Fill(SSD1306_COLOR_WHITE);
					SSD1306_DrawLayout();
				
					//--Update Icon Bar--
					//Disable_Wifi_Icon();
					Disable_Host_Icon();
					Disable_Data_Icon();
				
					//Connection->OK
					SSD1306_GotoXY(4,18);
					SSD1306_Puts("Connected to:", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,31);
					SSD1306_Puts(SSID, &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_UpdateScreen();
					RECEIVED_COMMAND = _ID_;
				
					osDelay(1000);
					//osThreadSuspendAll();
					uart_transmit_buffer[0] = 'H';
					uart_transmit_buffer[1] = 'R';
					uart_transmit_buffer[2] = '#';
					nums_in_tr_buf = sizeof(uart_transmit_buffer)/sizeof(uint8_t);
					state = HAL_UART_Transmit_IT(&huart1, uart_transmit_buffer, nums_in_tr_buf);//, 5000);
					//osThreadResumeAll();
								
			}
			else if (RECEIVED_COMMAND == _WF_)
			{
					//Layout
					SSD1306_Fill(SSD1306_COLOR_WHITE);
					SSD1306_DrawLayout();
				
					//--Update Icon Bar--
					//Disable_Wifi_Icon();
					//Disable_Host_Icon();
					//Disable_Data_Icon();
				
					//Connection->OK
					SSD1306_GotoXY(4,18);
					SSD1306_Puts("Connection Failed.", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,31);
					SSD1306_Puts("Please reset the", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,44);
					SSD1306_Puts("device and retry.", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_UpdateScreen();
					RECEIVED_COMMAND = _ID_;
			}
			else if (RECEIVED_COMMAND == _HW_)
			{
					//Layout
					SSD1306_Fill(SSD1306_COLOR_WHITE);
					SSD1306_DrawLayout();
					
					//Connecting
					SSD1306_GotoXY(4,18);
					SSD1306_Puts("Connecting to", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,31);
					SSD1306_Puts("Host", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_UpdateScreen();
					for(uint8_t i = 0; i<=2; i++)
					{
							SSD1306_Puts(".", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_UpdateScreen();
							osDelay(200);
					}
			}
			else if (RECEIVED_COMMAND == _HS_)
			{
					//Layout
					SSD1306_Fill(SSD1306_COLOR_WHITE);
					SSD1306_DrawLayout();
				
					//--Update Icon Bar--
					//Disable_Wifi_Icon();
					//Disable_Host_Icon();
					Disable_Data_Icon();
				
					//Connection->OK
					SSD1306_GotoXY(4,18);
					SSD1306_Puts("Connected to:", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,31);
					SSD1306_Puts(SSID, &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,44);
					SSD1306_Puts(HOST, &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_UpdateScreen();
				
					osDelay(700);  //For users to see more clearly..
					//Connection phase completed..
					connected_flag = 1;
				
					ROTARY_EVENT = 1;
					OLED_STATE = 1;
				
					RECEIVED_COMMAND = _ID_;
			}
			else if (RECEIVED_COMMAND == _HF_)
			{
					//Layout
					SSD1306_Fill(SSD1306_COLOR_WHITE);
					SSD1306_DrawLayout();
				
					//--Update Icon Bar--
					//Disable_Wifi_Icon();
					Disable_Host_Icon();
					Disable_Data_Icon();
				
					//Connection->OK
					SSD1306_GotoXY(4,18);
					SSD1306_Puts("Connection Failed.", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,31);
					SSD1306_Puts("Please reset the", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,44);
					SSD1306_Puts("device and retry.", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_UpdateScreen();
					RECEIVED_COMMAND = _ID_;
			}
			else if (RECEIVED_COMMAND == _WI_)
			{
					if (RECEIVED_DATA[0] == '1')
					{

					}
					else
					{
							//Layout
							SSD1306_Fill(SSD1306_COLOR_WHITE);
							SSD1306_DrawLayout();
						
							//--Update Icon Bar--
							Disable_Wifi_Icon();
							Disable_Host_Icon();
							Disable_Data_Icon();
						
							SSD1306_GotoXY(4,18);
							SSD1306_Puts("Disconnected from", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_GotoXY(4,31);
							SSD1306_Puts("Access point", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_UpdateScreen();
						
							osThreadSuspend(UARTReceiveThreadHandle);
						
							//Exit from Connected phase
							connected_flag = 0;
					}
				
					RECEIVED_COMMAND = _ID_;
			}
			else if (RECEIVED_COMMAND == _HI_)
			{
					
				
					if (RECEIVED_DATA[0] == '1')
					{

					}
					else
					{
							//Layout
							SSD1306_Fill(SSD1306_COLOR_WHITE);
							SSD1306_DrawLayout();
						
							//--Update Icon Bar--
							Disable_Wifi_Icon();
							Disable_Host_Icon();
							Disable_Data_Icon();
						
							SSD1306_GotoXY(4,18);
							SSD1306_Puts("Disconnected from", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_GotoXY(4,31);
							SSD1306_Puts("Host", &Font_7x10, SSD1306_COLOR_WHITE);
							SSD1306_UpdateScreen();
						
							//!!! DONT RECEIVE ANY DATA, CUZ WHY SHOULD WE?? AFTER THIS POINT !!!
							osThreadSuspend(UARTReceiveThreadHandle);
						
							//Exit from Connected phase
							connected_flag = 0;
					}
					
					RECEIVED_COMMAND = _ID_;
			}
			else if (RECEIVED_COMMAND == _RS_ && RECEIVED_DATA[0] != '1' && RECEIVED_DATA[0] != '0' && RECEIVED_DATA[0] != 0)
			{
					osThreadSuspend(DataCommunicationThreadHandle);

					//Layout
					SSD1306_Fill(SSD1306_COLOR_WHITE);
					SSD1306_DrawLayout();
				
					//--Update Icon Bar--
					//Disable_Wifi_Icon();
					//Disable_Host_Icon();
					Disable_Data_Icon();
				
					SSD1306_GotoXY(4,18);
					SSD1306_Puts("Data from", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,31);
					SSD1306_Puts(HOST, &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_Puts(":", &Font_7x10, SSD1306_COLOR_WHITE);
					SSD1306_GotoXY(4,44);
					SSD1306_Puts(RECEIVED_DATA, &Font_7x10, SSD1306_COLOR_WHITE);
				
					SSD1306_UpdateScreen();
				
					osThreadSuspend(UARTReceiveThreadHandle);  ///----SOLVED SENDING PROBLEM
					
					osDelay(50);
					
					SendResponse();
					
					osThreadResume(UARTReceiveThreadHandle);   ///----SOLVED SENDING PROBLEM
					
					osThreadResume(DataCommunicationThreadHandle);
					
					RECEIVED_COMMAND = _ID_;
					
			}
			else if (RECEIVED_COMMAND == _SS_)
			{
					RECEIVED_COMMAND = _ID_;
			}
			osDelay(200);
		
  }
}

static void Data_Communication_Thread (void const *argument)
{
		(void) argument;
	
		for (;;)
		{
				if (connected_flag == 1)
				{
					  //Implement disconnection check!!
						//########################### <-- Do it!
					
						//RequestHostConnectionInfo();
						//osDelay(1000);
						RequestData();
						osDelay(1000);
				}
		}
}

static void Rotary_Led_and_Ctr10ms_Thread(void const *argument)
{
  uint32_t count;
  (void) argument;


  for (;;)
  {
		 if (LED_STATE == 0)
		 {
				HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN,1);
		 }
		 else if (LED_STATE == 1)
		 {
				HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN,0);
		 }

			/*if (Rotary_Direction == LEFT)
			{
					Rotary_Counter--;
			}
			else if (Rotary_Direction == RIGHT)
			{
					Rotary_Counter++;
			}*/
		 
		 if (IsRotaryRight())
		 {
				if (OLED_STATE == 1)
				{
						OLED_STATE = 2;
				}
				else if (OLED_STATE == 2)
				{
						OLED_STATE = 3;
				}
				else if (OLED_STATE == 3)
				{
						OLED_STATE = 2;
				}
				ROTARY_EVENT = 1;
				ClearRotaryFlag();
		 }
		 else if (IsRotaryLeft())
		 {
				if (OLED_STATE == 1)
				{
						OLED_STATE = 3;
				}
				else if (OLED_STATE == 2)
				{
						OLED_STATE = 3;
				}
				else if (OLED_STATE == 3)
				{
						OLED_STATE = 2;
				}
				ROTARY_EVENT = 1;
				ClearRotaryFlag();
		 }
		 
		 ctr10ms ++;
		 
		 osDelay(10);
  }
}

static void UART_Receive_Thread(void const *argument)
{
  uint32_t count;
  (void) argument;
	int j = 0, word_length = 0, previous_word_length = 0;
	
  for (;;)
  {
		  //uart_transmit_buffer[0] = 'x';
		  //HAL_UART_Transmit_IT(&huart1, uart_transmit_buffer, sizeof(uart_transmit_buffer));
			if (uart_line_ready == 0)
			{
					HAL_UART_Receive_IT(&huart1, uart_receive_buffer, UART_RX_BUFFER_SIZE);//, 0xFFFF);
					if (uart_receive_buffer[0] != 0)
					{
							if (uart_receive_buffer[0] != END_OF_WORD_CHAR)
							{
									uart_line_buffer[k] = uart_receive_buffer[0];
									uart_receive_buffer[0] = 0;
									k++;
							}
							else
							{
									uart_receive_buffer[0] = 0;
									uart_line_ready = 1;
									word_length = k;
									k = 0;
							}
					}
			}
			if (uart_line_ready == 1)
			{
					osThreadSuspend(OLEDThreadHandle);   // Is necessary not to miss any data that often...
					for (j = 0; j <= word_length; j++)
					{
							UART_RECEIVED_COMMAND[j] = uart_line_buffer[j];
					}
					for (j = 0; j <= word_length; j++)
					{
							uart_line_buffer[j] = 0;
					}
					uart_line_ready = 0;
					
					RECEIVED_COMMAND = ParseReceivedCommand(UART_RECEIVED_COMMAND);
					
					if (RECEIVED_COMMAND != _ID_)
					{
							//Clear Buffer
						  for ( j = 0; j <= previous_word_length; j++)
							{
									RECEIVED_DATA[j] = 0;
							}
							//Assign new data
							AssignReceivedData (word_length);  //Results in uint8_t * RECEIVED_DATA
							previous_word_length = word_length;
					}
					//osThreadSuspend(OLEDThreadHandle);
					//Assign SSID and HOST
					if (RECEIVED_COMMAND == _WS_)  //Wifi network connection succeeded
					{
							AssignSSID (word_length);
					}
					else if (RECEIVED_COMMAND == _HS_) //Host connection succeeded.
					{
							AssignHOST (word_length);
					}
					
					//Clear Buffer
					for (j = 0; j <= word_length; j++)
					{
							UART_RECEIVED_COMMAND[j] = 0;
					}
					
					osThreadResume(OLEDThreadHandle); // Is necessary not to miss any data that often...
			}
		  //osDelay(10);  Should be no delay in order not to miss any data..
  }
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

