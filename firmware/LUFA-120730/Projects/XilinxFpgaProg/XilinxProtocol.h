/*
 * XilinxProtocol.h
 *
 *  Created on: 11.11.2012
 *      Author: andreas
 */

#ifndef XILINXPROTOCOL_H_
#define XILINXPROTOCOL_H_

#include <avr/io.h>

#define PIN_PROGRAM_B                   PB5
#define PIN_INIT_B                      PB4
#define PIN_DONE                        PD6

#define clearProgramB()                 PORTB &= ~_BV(PIN_PROGRAM_B)
#define setProgramB()                   PORTB |=  _BV(PIN_PROGRAM_B)

#define isSetInitB()                    (PINB & _BV(PIN_INIT_B))
#define isSetDone()                     (PIND & _BV(PIN_DONE))

void xilinxInit(void);
uint8_t xilinxSetupConfiguration(void);
void xilinxSendData(uint8_t *buffer, uint16_t size);
void xilinxSend(uint8_t buffer);

#endif /* XILINXPROTOCOL_H_ */
