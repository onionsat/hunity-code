/*
 * W25Qxx.h
 *
 *  Created on: Oct 31, 2024
 *      Author: TB
 */

#ifndef INC_W25QXX_H_
#define INC_W25QXX_H_

void csLOW();
void csHIGH();
void W25Q_Reset (void);
uint32_t W25Q_ReadID(void);
void W25Q_Read(uint32_t startPage, uint8_t offset, uint32_t size, uint8_t *rData);
void W25Q_Erase_Sector(uint16_t numsector);
void writePage(uint32_t page, uint8_t *data, uint32_t size);
void chipErase(void);
void writeExperiment(void);
uint32_t readExperiment(uint16_t startSector, uint16_t numberOfSectors);
void UARTlogPage(uint32_t pageNum);

#endif /* INC_W25QXX_H_ */
