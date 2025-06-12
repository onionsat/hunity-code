/*
 * W25Qxx.c
 *
 *  Created on: Oct 31, 2024
 *      Author: TB
 */


#include "main.h"
#include "W25Qxx.h"
#include "globalvars.h"
#include "string.h"
#include <stdio.h>

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
#define W25Q_SPI hspi1

void csLOW()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}
void csHIGH()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void SPI_Write(uint8_t *data, uint16_t len)
{
	HAL_SPI_Transmit(&W25Q_SPI, data, len, 2000);
}

void SPI_Read(uint8_t *data, uint32_t len)
{
	HAL_SPI_Receive(&W25Q_SPI, data, len, 5000);
}

void W25Q_Reset (void)
{
	uint8_t tData[2];
	tData[0] = 0x66; //enable reset
	tData[1] = 0x99; //reset

	csLOW();

	SPI_Write(tData, 2);
	csHIGH();

	HAL_Delay(100);
}

uint32_t W25Q_ReadID(void)
{
	uint8_t tData = 0x9F; //read JEDEC ID

	uint8_t rData[3];

	csLOW();

	SPI_Write(&tData, 1);
	SPI_Read(rData, 3);

	csHIGH();

	return((rData[0]<<16)|(rData[1]<<8)|rData[2]);
}



void W25Q_Read(uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData)
{
	uint8_t tData[5];

	// Calculating address
	uint32_t memAddr = (startPage * 256) + offset;

	tData[0] = 0x03; // Enable Read
	tData[1] = (memAddr>>16)&0xFF;
	tData[2] = (memAddr>>8)&0xFF;
	tData[3] = (memAddr)&0xFF;

	csLOW();

	// Sending command and address
	SPI_Write(tData, 4);
	// Reading size bytes of data
	SPI_Read(rData, size);

	csHIGH();
}

void write_enable(void)
{
	uint8_t tData = 0x06; // Write enable instruction
	csLOW();
	SPI_Write(&tData, 1);
	csHIGH();
	HAL_Delay(5);
}

void write_disable(void)

{	uint8_t tData = 0x04; // Write disable instruction
	csLOW();
	SPI_Write(&tData, 1);
	csHIGH();
	HAL_Delay(5);
}

void W25Q_Erase_Sector(uint16_t numsector)
{
	uint8_t tData[4];
	// Calculating address
	uint32_t memAddr = numsector*16*256;

	write_enable();

	tData[0] = 0x20; // Erase sector command
	tData[1] = (memAddr>>16)&0xFF;
	tData[2] = (memAddr>>8)&0xFF;
	tData[3] = (memAddr)&0xFF;

	csLOW();
	SPI_Write(tData, 4);
	csHIGH();

	HAL_Delay(450);

	write_disable();
}

// writes a whole page
void writePage(uint32_t page, uint8_t *data, uint32_t size)
{
	uint8_t tData[260];
	uint32_t memAddr = page * 256;

	tData[0] = 0x02;  // page program
	tData[1] = (memAddr>>16)&0xFF;
	tData[2] = (memAddr>>8)&0xFF;
	tData[3] = (memAddr)&0xFF;

	for(int i = 4; i < size + 4; i++)
	{
		tData[i] = *(data+i-4);
	}

	write_enable();

	csLOW();
	SPI_Write(tData, 4+size);
	csHIGH();

	HAL_Delay(5);
	write_disable();
}

// No reset
void chipErase(void)
{
	uint8_t command = 0x60;

	write_enable();

	csLOW();
	SPI_Write(&command, 1);
	csHIGH();

	HAL_Delay(105*1000);
	write_disable();
}

// writes the experiment data onto the flash (0,1,0,1,0,1,0,1,0...), indicates start of the process with led on indicates end of the process with led off
void writeExperiment(void)
{
	// Setting LED High to indicate start of writing experiment data
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);

	// Setting up the data that should be written
	uint8_t expData[256];
	uint8_t *ptrExpData = expData;

	for(int i = 0; i < 256; i++)
	{
		expData[i] = 0xAA;
	}

	// Erasing the chip
	chipErase();

	// Writing the data onto the chip
	for(uint32_t i = 0; i < 16*16*128; i++)
	{
		writePage(i, ptrExpData, 256);
	}

	// Setting LED Low to indicate the end of writing experiment data
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
}

// Gives back the number of bitflips
uint32_t readExperiment(uint16_t startSector, uint16_t numberOfSectors)
{
	uint32_t countOfBitflips = 0;
	uint8_t currentBit;

	uint8_t rxData[4096];
	uint8_t *ptrRxData = rxData;

	for(uint32_t i = startSector; i < startSector + numberOfSectors; i++)
	{
		W25Q_Read(i, 0, 4096, ptrRxData);

		for(int i = 0; i < 4096; i++)
		{
			for(int k = 0; k < 8; k++)
			{
				currentBit = (rxData[i] >> k) & 1;

				if(k % 2 == 0 && currentBit == 1)
				{
					countOfBitflips++;
				}
				if(k % 2 == 1 && currentBit == 0)
				{
					countOfBitflips++;
				}
			}
		}
	}

	return countOfBitflips;
}

// writes out a page onto UART, start indicated by LED on, end indicated by LED off
void UARTlogPage(uint32_t pageNum)
{
	// Setting LED High to indicate start of writing experiment data
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);

	char databuffer[26];
	uint8_t rxData[256];
	uint8_t *ptrRxData = rxData;

	// Reading out the desired page
	W25Q_Read(pageNum, 0, 256, ptrRxData);

	// Sending page number
	snprintf(databuffer, sizeof(databuffer), "Data at page: %lu\n", pageNum);
	HAL_UART_Transmit(&huart1, (uint8_t *)databuffer, strlen(databuffer), 1000);

	for(int i = 0; i < 256; i++)
	{
		snprintf(databuffer, sizeof(databuffer), "%d index: %u\n", i, rxData[i]);
		HAL_UART_Transmit(&huart1, (uint8_t *)databuffer, strlen(databuffer), 1000);
	}

	// Setting LED Low to indicate the end of writing experiment data
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
}
