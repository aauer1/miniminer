/*
 * application.h
 *
 *  Created on: 24.05.2013
 *      Author: DI Andreas Auer
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Misc/RingBuffer.h>

#include "timer.h"

typedef struct Application_ Application;

struct Application_
{
    USB_ClassInfo_CDC_Device_t *cdc_info;

    //private:
    RingBuffer_t USARTtoUSB_Buffer;
    uint8_t USARTtoUSB_Buffer_Data[128];

    Timer serial_timeout;
    RingBuffer_t serial_rx_buffer;
    uint8_t serial_rx_data[64];
};

void appInit(Application *app);
void appService(Application *app);
void appSerialDataReceived(Application *app, uint8_t data);
void appUsbDataReceived(Application *app);

#endif /* APPLICATION_H_ */
