/*
 * serial.h
 *
 *  Created on: 17.11.2012
 *      Author: andreas
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

#define SERIAL_START_CONFIG             0x01
#define SERIAL_DOWNLOAD_DATA            0x02
#define SERIAL_SPI_DATA                 0x03
#define SERIAL_SPI_SELECT               0x04
#define SERIAL_USART_DATA               0x05
#define SERIAL_USE_USART                0x06
#define SERIAL_USE_SPI                  0x07
#define SERIAL_FPGA_RESET               0x08
#define SERIAL_FPGA_START               0x09

#define SERIAL_END_COMMANDS             0x0a

#define SERIAL_ACK                      0x80
#define SERIAL_NAK                      0x81

void serialInit(void);
uint8_t serialAddByte(uint8_t byte);
uint8_t serialProcess(void);

#endif /* SERIAL_H_ */
