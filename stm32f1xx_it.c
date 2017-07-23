/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"
#include "stm32f1xx_it.h"
#include "cmsis_os.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

//Interrupt icinde osTick alinca patliyor...!!

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;

/******************************************************************************/
/*            Cortex-M3 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/
/*void SysTick_Handler(void)
{
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
}*/
void SysTick_Handler(void)
{
  osSystickHandler();

  HAL_IncTick();
	
}

void EXTI9_5_IRQHandler(void)
{
	if (__HAL_GPIO_EXTI_GET_IT(ROTARY_SW_PIN) !=RESET)
	{
			__HAL_GPIO_EXTI_CLEAR_IT(ROTARY_SW_PIN);
		
			//Debounce button..
			//########################### <-- Do it!
		
			if (ctr10ms - ticks > 10)
			{
					ticks = ctr10ms;
					if (OLED_STATE == 2 || OLED_STATE == 3)
					{
							ROTARY_EVENT = 1;
							OLED_STATE = 4;
					}
			}

			/*if (OLED_STATE == 0)
				OLED_STATE = 1;
			else if (OLED_STATE == 1)
			{
				OLED_STATE = 0;
			}
			HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_STATE);*/

	}
	
	if (__HAL_GPIO_EXTI_GET_IT(ROTARY_A_PIN) !=RESET)
	{
			__HAL_GPIO_EXTI_CLEAR_IT(ROTARY_A_PIN);
		  if (ctr10ms - ticks > 5)
			{
	//			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
//				interrupts_exti9_5_flag = 0;
				ticks = ctr10ms;
				LED_STATE = 1;
				Rotary_Direction = LEFT;
				
				
//				if (OLED_STATE == 3)
//				{
//						OLED_STATE = 2;
//				}
//				else if (OLED_STATE == 2)
//				{
//						OLED_STATE = 1;
//				}
//				else if (OLED_STATE == 1)
//				{
//						OLED_STATE = 3;
//				}
				
				
			}
			
		
	}
	else if (__HAL_GPIO_EXTI_GET_IT(ROTARY_B_PIN) !=RESET)
	{
			
			__HAL_GPIO_EXTI_CLEAR_IT(ROTARY_B_PIN);
			if (ctr10ms - ticks > 5)
			{
//			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
//			interrupts_exti9_5_flag = 0;
				ticks = ctr10ms;
				LED_STATE = 0;
				Rotary_Direction = RIGHT;
				
				
//				if (OLED_STATE == 1)
//				{
//						OLED_STATE = 2;
//				}
//				else if (OLED_STATE == 2)
//				{
//						OLED_STATE = 3;
//				}
//				else if (OLED_STATE == 3)
//				{
//						OLED_STATE = 1;
//				}
				
			}
			
	}
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  
	//HAL_UART_Receive_IT(&huart1, uart_receive_buffer, UART_RX_BUFFER_SIZE);
	
}

/**
* @brief This function handles USART1 global interrupt.
*/
void USART1_IRQHandler(void)
{
	int j;
	// Surekli burada nedense....!!
  /* USER CODE BEGIN USART1_IRQn 0 */
	
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
