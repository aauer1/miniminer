/*
 * avalon.h
 *
 *  Created on: 06.07.2013
 *      Author: DI Andreas Auer
 */

#ifndef AVALON_H_
#define AVALON_H_

#include <stdint.h>

#define DATA_PORT       PORTB
#define DATA_N          _BV(PB4)
#define DATA_P          _BV(PB5)

#define RESET_PORT      PORTD
#define RESET_PIN       _BV(PD6)

#define asicEnableSpi()         DDRD |=  _BV(PD7)
#define asicDisableSpi()        DDRD &= ~_BV(PD7)
#define asicIdle()              DATA_PORT |= (DATA_N | DATA_P);

void asicInit(void);
void asicReset(void);
void asicResetSpi(void);
void asicStop(void);
uint8_t asicReportValid(void);
uint32_t asicGetNonce(void);
void asicSend(uint32_t *buffer, uint8_t size);
void asicPrecalc(uint32_t *midstate, uint32_t *data, uint32_t *precalc);

#endif /* AVALON_H_ */
