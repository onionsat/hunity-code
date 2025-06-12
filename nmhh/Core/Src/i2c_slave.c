/*
 * i2c_slave.c
 *
 *  Created on: Oct 22, 2024
 *      Author: tamas
 */

#include "main.h"
#include "i2c_slave.h"
#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <globalvars.h>
#include "stm32f1xx_hal.h"


extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

uint8_t RxData[8];
uint32_t rxcount = 0;
uint32_t txcount = 0;
uint32_t generallcallError = 0;
uint32_t receiveError = 0;
uint32_t transmitError = 0;
uint32_t counterror = 0;

char uart_i2c_buffer[50]; // For uart debugging in this file


// I2C listen callback - Here we reactivate i2c, for being in listening mode
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_EnableListen_IT(hi2c);
}

void process_data(void)
{
	// processing write data
	switch (RxData[0])
	{
		case 0x01:
			datasendtype = 1;

			break;
		case 0x02:
			datasendtype = 2;

			startSectorToSend = ((uint16_t)RxData[1] << 8) | RxData[2];
			numOfSectorsToSend = ((uint16_t)RxData[3] << 8) | RxData[4];

			break;
		case 0x54:
			unixTimestamp = ((uint32_t)RxData[1]) | ((uint32_t)RxData[2] << 8) | ((uint32_t)RxData[3] << 16) | ((uint32_t)RxData[4] << 24);

			break;
		case 0x03:
			flashwrite = 1;

			break;
		case 0x04:
			uartFlash = 1;

			uartFlashPage = ((uint32_t)RxData[1] << 24) | ((uint32_t)RxData[2] << 16) | ((uint32_t)RxData[3] << 8) | (uint32_t)RxData[4];

			break;
	}
}

// I2C address callback
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	if (TransferDirection == I2C_DIRECTION_TRANSMIT) // If the direction is write
	{
    	if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_GENCALL) == 0) // Normal write
		{
    		if(HAL_I2C_Slave_Sequential_Receive_IT(hi2c, RxData, 8, I2C_FIRST_AND_LAST_FRAME) != HAL_OK)
    		{
    			receiveError++;
    		}
		}
		else // Generalcall
		{
			if(HAL_I2C_Slave_Sequential_Receive_IT(hi2c, RxData, 5, I2C_FIRST_AND_LAST_FRAME) != HAL_OK)
			{
				generallcallError++;
			}
		}
	}
    else // If the direction is read
    {
    	if(HAL_I2C_Slave_Seq_Transmit_IT(hi2c, datatosend, 16, I2C_FIRST_AND_LAST_FRAME) != HAL_OK)
    	{
    		transmitError++;
    	}
    }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	txcount++;
	datasent = 1;
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	rxcount++;
	process_data();
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	counterror++;
	uint32_t errorcode = HAL_I2C_GetError(hi2c);

	if(errorcode != HAL_I2C_ERROR_NONE)
	{
		// Sending errorcode on UART if UART debug is enabled
		if(UARTdebug == 1)
		{
			snprintf(uart_i2c_buffer, sizeof(uart_i2c_buffer), "I2C error: %lu\nErrorcount: %lu\n", errorcode, counterror);
			HAL_UART_Transmit(&huart1, (uint8_t *)uart_i2c_buffer, strlen(uart_i2c_buffer), 1000);
		}

		if(errorcode & HAL_I2C_ERROR_BERR) // Bus error
		{
			__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_BERR);

			//HAL_I2C_DeInit(hi2c);
			__HAL_RCC_I2C1_FORCE_RESET();
			__HAL_RCC_I2C1_RELEASE_RESET();

			if(HAL_I2C_Init(hi2c) != HAL_OK)
			{
				NVIC_SystemReset(); // Reset
			}
		}

		if(errorcode & HAL_I2C_ERROR_AF) // Acknowledge Failure
		{

		}

		if (errorcode & HAL_I2C_ERROR_ARLO) // Arbitration lost
		{
		    __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_ARLO);
		}

		if(errorcode & HAL_I2C_ERROR_OVR) // Overrun error
		{
			__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_OVR);

			//HAL_I2C_DeInit(hi2c);
			__HAL_RCC_I2C1_FORCE_RESET();
			__HAL_RCC_I2C1_RELEASE_RESET();

			if(HAL_I2C_Init(hi2c) != HAL_OK)
			{
				NVIC_SystemReset(); // Reset
			}
		}

		if(errorcode & HAL_I2C_ERROR_TIMEOUT) // Timeout Error
		{
			//HAL_I2C_DeInit(hi2c);
            __HAL_RCC_I2C1_FORCE_RESET();
            __HAL_RCC_I2C1_RELEASE_RESET();

			if(HAL_I2C_Init(hi2c) != HAL_OK)
			{
				NVIC_SystemReset(); // Reset
			}
		}
	}

    HAL_I2C_EnableListen_IT(hi2c);
}

