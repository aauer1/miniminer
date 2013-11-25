/*
 * serial.c
 *
 *  Created on: 17.11.2012
 *      Author: andreas
 */

#include "serial.h"
#include "XilinxProtocol.h"
#include "XilinxFpgaProg.h"

#include <LUFA/Drivers/Peripheral/SPI.h>
#include <LUFA/Drivers/Misc/RingBuffer.h>

static RingBuffer_t buffer_;
static uint8_t buffer_data_[128];

static uint8_t frame_size_;


//------------------------------------------------------------------------------
void serialInit(void)
{
    RingBuffer_InitBuffer(&buffer_, buffer_data_, sizeof(buffer_data_));
    frame_size_ = 0;
}

//------------------------------------------------------------------------------
uint8_t serialAddByte(uint8_t byte)
{
    uint16_t size;

    size = RingBuffer_GetCount(&buffer_);

    if(size == 0)
    {
        if(byte >= SERIAL_END_COMMANDS)
        {
            return false;
        }
    }
    else if(size == 1)
    {
        frame_size_ = byte+2;
    }

    RingBuffer_Insert(&buffer_, byte);
    size = RingBuffer_GetCount(&buffer_);

    if(size == frame_size_)
    {
        frame_size_ = 0;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
uint8_t serialProcess(void)
{
    uint8_t cmd = RingBuffer_Remove(&buffer_);
    uint8_t size = RingBuffer_Remove(&buffer_);

    if(cmd == SERIAL_START_CONFIG)
    {
        return xilinxSetupConfiguration();
    }
    else if(cmd == SERIAL_DOWNLOAD_DATA)
    {
        uint8_t i=0;
        LEDs_ToggleLEDs(LEDS_LED1);
        for(i=0; i<size; i++)
        {
            xilinxSend(RingBuffer_Remove(&buffer_));
        }
        
        if(isSetDone())
        {
            return true;
        }
        
        if(!isSetInitB())
        {
            return false;
        }
    }
    else if(cmd == SERIAL_USE_SPI)
    {
        SPI_Init(SPI_SPEED_FCPU_DIV_8  |
                 SPI_SCK_LEAD_RISING   |
                 SPI_SAMPLE_LEADING    |
                 SPI_ORDER_MSB_FIRST   |
                 SPI_MODE_MASTER);
        DDRB  |= (1 << 0);
        PORTB |= (1 << 0);
    }
    else if(cmd == SERIAL_SPI_DATA)
    {
        uint8_t i=0;
        LEDs_ToggleLEDs(LEDS_LED2);
        for(i=0; i<size; i++)
        {
            SPI_SendByte(RingBuffer_Remove(&buffer_));
        }
    }
    else if(cmd == SERIAL_SPI_SELECT)
    {
        uint8_t prop = RingBuffer_Remove(&buffer_);
        if(prop)
        {
            PORTD &= ~_BV(PD3);
        }
        else
        {
            PORTD |=  _BV(PD3);
        }
    }
    else if(cmd == SERIAL_USE_USART)
    {
        SPI_Disable();
        PORTB &= ~((1 << 1) | (1 << 2));
        DDRB  |=   (1 << 1) | (1 << 2);
    }
    else if(cmd == SERIAL_USART_DATA)
    {
        uint8_t i=0;
        for(i=0; i<size; i++)
        {
            Serial_SendByte(RingBuffer_Remove(&buffer_));
        }
    }
    else if(cmd == SERIAL_FPGA_RESET)
    {
        PORTB |=  _BV(PB1);
        _delay_us(5);
        PORTB &= ~_BV(PB1);
    }
    else if(cmd == SERIAL_FPGA_START)
    {
        PORTB |=  _BV(PB2);
        _delay_us(5);
        PORTB &= ~_BV(PB2);
    }
    else
    {
        uint8_t i=0;
        for(i=0; i<size; i++)
            RingBuffer_Remove(&buffer_);
        return false;
    }

    return true;
}
