/*
 * XilinxProtocol.c
 *
 *  Created on: 11.11.2012
 *      Author: andreas
 */

#include <avr/io.h>
#include <util/delay.h>

#include "XilinxProtocol.h"

#include <LUFA/Drivers/Peripheral/SPI.h>

//------------------------------------------------------------------------------
void xilinxInit(void)
{
    SPI_Init(SPI_SPEED_FCPU_DIV_8  |
             SPI_SCK_LEAD_RISING   |
             SPI_SAMPLE_LEADING    |
             SPI_ORDER_MSB_FIRST   |
             SPI_MODE_MASTER);
    DDRB  |= (1 << 0);
    PORTB |= (1 << 0);
}

//------------------------------------------------------------------------------
uint8_t xilinxSetupConfiguration()
{
    uint8_t ret = true;
    uint16_t timeout = 1000;

    clearProgramB();
    _delay_us(5);
    setProgramB();
    _delay_us(5);
    while(!isSetInitB() && timeout)
    {
        timeout--;
        _delay_ms(1);
    }

    if(timeout == 0)
    {
        ret = false;
    }

    return ret;
}

//------------------------------------------------------------------------------
void xilinxSend(uint8_t buffer)
{
    SPI_SendByte(buffer);
}
