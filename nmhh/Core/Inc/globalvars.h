/*
 * globalvars.h
 *
 *  Created on: Jan 20, 2025
 *      Author: TB
 */

#ifndef INC_GLOBALVARS_H_
#define INC_GLOBALVARS_H_

extern uint32_t unixTimestamp; // Variable for storing unixtimestamp
extern uint8_t datatosend[16]; // Variable for storing data to be sent on i2c
extern uint8_t datasendtype; // Variable for storing the type of data to be sent on i2c, 1 -> normal, 2 -> bitflips
extern uint16_t startSectorToSend; // In bitflip mode the startsector where the checking should start from
extern uint16_t numOfSectorsToSend; // In bitflip mode the number of sectors to check
extern uint8_t datasent; // Variable for controllflow
extern uint8_t flashwrite; // For writing experiment data
extern uint8_t uartFlash; // Used to print a page onto UART
extern uint32_t uartFlashPage; // Used to specify which page should be written to UART
extern uint8_t UARTdebug; // 1 -> to enable UART debug

#endif /* INC_GLOBALVARS_H_ */
