/*
 * application.c
 *
 *  Created on: 24.05.2013
 *      Author: DI Andreas Auer
 */

#include <LUFA/Drivers/Peripheral/Serial.h>

#include "application.h"
#include "serial.h"

//------------------------------------------------------------------------------
void appInit(Application *app)
{
    RingBuffer_InitBuffer(&app->USARTtoUSB_Buffer, app->USARTtoUSB_Buffer_Data, sizeof(app->USARTtoUSB_Buffer_Data));
    RingBuffer_InitBuffer(&app->serial_rx_buffer, app->serial_rx_data, sizeof(app->serial_rx_data));

    timerSet(&app->serial_timeout, 100);
}

//------------------------------------------------------------------------------
void appService(Application *app)
{
/*
    int16_t data = Serial_ReceiveByte();
    if(data != -1)
    {
        RingBuffer_Insert(&app->serial_rx_buffer, data);
        timerRestart(&app->serial_timeout);
    }
*/
    if(timerExpired(&app->serial_timeout))
    {
        uint8_t count = RingBuffer_GetCount(&app->serial_rx_buffer);
        uint8_t i;
        if(count > 0)
        {
            RingBuffer_Insert(&app->USARTtoUSB_Buffer, SERIAL_USART_DATA);
            RingBuffer_Insert(&app->USARTtoUSB_Buffer, count);
            for(i=0; i<count; i++)
            {
                RingBuffer_Insert(&app->USARTtoUSB_Buffer, RingBuffer_Remove(&app->serial_rx_buffer));
            }
        }

        timerRestart(&app->serial_timeout);
    }
}

//------------------------------------------------------------------------------
void appSerialDataReceived(Application *app, uint8_t data)
{
    RingBuffer_Insert(&app->serial_rx_buffer, data);
    timerRestart(&app->serial_timeout);
}

//------------------------------------------------------------------------------
void appUsbDataReceived(Application *app)
{
    int16_t received_byte = 0;
    while((received_byte = CDC_Device_ReceiveByte(app->cdc_info)) >= 0)
    {
        if(serialAddByte(received_byte))
        {
            if(serialProcess())
            {
                RingBuffer_Insert(&app->USARTtoUSB_Buffer, SERIAL_ACK);
            }
            else
            {
                RingBuffer_Insert(&app->USARTtoUSB_Buffer, SERIAL_NAK);
            }
            RingBuffer_Insert(&app->USARTtoUSB_Buffer, 0);
        }
    }
}
