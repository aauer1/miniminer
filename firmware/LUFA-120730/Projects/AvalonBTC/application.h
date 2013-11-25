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

#define DEFAULT_HASHCLOCK   256 

#define CLOCK_LOW_CHG       0x00030007UL
#define CLOCK_HALF_CHG      0x00030017UL
#define CLOCK_HIGH_CFG      0x00000172UL

typedef struct Application_     Application;
typedef struct Identity_        Identity;
typedef struct WorkUnit_        WorkUnit;
typedef struct State_           State;

#define STATE_WORKING           'W'
#define STATE_RESET             'R'

struct Identity_
{
    uint8_t version;
    char product[8];
    uint32_t serial;
};

struct WorkUnit_
{
    uint32_t midstate[8];
    uint32_t data[3];
    uint32_t precalc[6];
};

struct State_
{
    uint8_t state;
    uint8_t nonce_valid;
    uint32_t nonce;
};

struct Application_
{
    USB_ClassInfo_CDC_Device_t *cdc_info;

    //private:
    RingBuffer_t USARTtoUSB_Buffer;
    uint8_t USARTtoUSB_Buffer_Data[256];

    RingBuffer_t buffer_;
    uint8_t buffer_data_[128];

    WorkUnit worktask;
    State state;

    Timer work_timer;
};

void appInit(Application *app);
void appService(Application *app);
void appUsbDataReceived(Application *app);

#endif /* APPLICATION_H_ */
