/*
 * avalon.c
 *
 *  Created on: 06.07.2013
 *      Author: DI Andreas Auer
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "main.h"
#include "avalon.h"

static uint32_t report_ = 0;
static uint32_t report_valid_ = 0;
static uint8_t report_count_ = 0;

static uint16_t report_timeout = 0;

#define r(x)    ((x-n)&7)

//------------------------------------------------------------------------------
ISR(SPI_STC_vect)
{
    SPCR &= ~_BV(SPIE);
    report_ = SPDR;

    report_timeout = 0;
    report_count_ = 1;
    while(report_count_ != 4)
    {
        if(SPSR & _BV(SPIF))
        {
            report_ <<= 8;
            report_ |= SPDR;
            report_count_++;
            report_timeout = 0;
        }
        else
        {
            report_timeout++;
            if(report_timeout == 1000)
                break;
        }
    }

    report_valid_ = 1;
}

//------------------------------------------------------------------------------
static void send32(uint32_t word)
{
    uint8_t i;
    uint8_t bit;

    for(i=0; i<32; i++)
    {
        bit = word & 0x01;
        DATA_PORT &= ~(DATA_N | DATA_P);
        _delay_us(1);
        word >>= 1;
        if(bit)
        {
            DATA_PORT |= DATA_P;
        }
        else
        {
            DATA_PORT |= DATA_N;
        }
        _delay_us(1);
    }
}

//------------------------------------------------------------------------------
void asicInit(void)
{
    DDRB  = 0x38;
    PORTB = 0x30;

    DDRD  |= _BV(PD6);
    PORTD |= _BV(PD6);

    SPCR = _BV(SPIE) | _BV(SPE) | _BV(CPHA) | _BV(DORD);
}

//------------------------------------------------------------------------------
void asicResetSpi(void)
{
    report_count_ = 0;
    report_valid_ = 0;

    asicDisableSpi();
    _delay_us(5);
    asicEnableSpi();
    _delay_us(1);
}

//------------------------------------------------------------------------------
void asicStop(void)
{
    report_count_ = 0;
    report_valid_ = 0;

    RESET_PORT &= ~RESET_PIN;
}

//------------------------------------------------------------------------------
void asicReset(void)
{
    report_count_ = 0;
    report_valid_ = 0;

    RESET_PORT &= ~RESET_PIN;
    _delay_ms(1);
    RESET_PORT |=  RESET_PIN;
    _delay_ms(1);
}

//------------------------------------------------------------------------------
void asicSend(uint32_t *buffer, uint8_t size)
{
    uint8_t i=0;
    for(i=0; i<size; i++)
    {
        send32(buffer[i]);
    }
}

//------------------------------------------------------------------------------
uint8_t asicReportValid(void)
{
    return report_valid_;
}

//------------------------------------------------------------------------------
uint32_t asicGetNonce(void)
{
    report_count_ = 0;
    report_valid_ = 0;

    return report_;
}

//------------------------------------------------------------------------------
uint32_t rotate(uint32_t x, uint8_t y)
{
    return ((x<<y) | (x>>(32-y)));
}

//------------------------------------------------------------------------------
void asicPrecalc(uint32_t *midstate, uint32_t *data, uint32_t *precalc)
{
    const uint32_t K[3] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf };
#if 0
    uint32_t m[8];
    uint8_t n;

    uint32_t S1, ch, t1, S0, maj, t2;

    uint32_t temp1;

    for(n = 0; n < 8; n++)
        m[n] = midstate[n];

    for(n = 0; n < 3; n++)
    {
        S1 = rotate(m[4], 26) ^ rotate(m[4], 21) ^ rotate(m[4], 7);
        ch = (m[4] & m[5]) ^ (~m[4] & m[6]);
        t1 = m[7] + S1 + ch + K[n] + data[n];
        S0 = rotate(m[0], 30) ^ rotate(m[0], 19) ^ rotate(m[0], 10);
        maj = (m[0] & m[1]) ^ (m[0] & m[2]) ^ (m[1] & m[2]);
        t2 = S0 + maj;

        m[7] = m[6];
        m[6] = m[5];
        m[5] = m[4];
        m[4] = m[3] + t1;
        m[3] = m[2];
        m[2] = m[1];
        m[1] = m[0];
        m[0] = t1 + t2;

        precalc[2-n] = m[0];
        precalc[5-n] = m[4];
    }
#else
    uint32_t x, y, z;
    uint32_t m[8];
    uint8_t n;

    for(n = 0; n < 8; n++)
        m[n] = midstate[n];

    for(n = 0; n < 3; n++)
    {
        x = m[5-n] ^ m[6-n];
        x = x & m[4-n];
        x = m[6-n] ^ x;
        x += K[n];
        x += data[n];
        x += m[7-n];
        y = rotate(m[4-n], 26);
        z = rotate(m[4-n], 21);
        z = y^z;
        y = rotate(m[4-n], 7);
        z = y^z;
        m[7-n] = z+x;
        m[3-n] = m[3-n] + m[7-n];
        x = rotate(m[r(0)], 30);
        y = rotate(m[r(0)], 19);
        y = y^x;
        x = rotate(m[r(0)], 10);
        y = x^y;
        x = m[r(0)] | m[r(1)];
        x = m[r(2)] & x;
        z = m[r(0)] & m[r(1)];
        x = x | z;
        m[7-n] += y + x;

        precalc[2-n] = m[7-n];
        precalc[5-n] = m[3-n];
    }
#endif
}
