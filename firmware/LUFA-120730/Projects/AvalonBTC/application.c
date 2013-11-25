/*
 * application.c
 *
 *  Created on: 24.05.2013
 *      Author: DI Andreas Auer
 */

#include <LUFA/Drivers/Peripheral/Serial.h>

#include "application.h"
#include "avalon.h"
#include "timer.h"
#include "main.h"

#define GOOD_MIDSTATE       { 0x5fddb5bc,0x00bdafd2,0x144684c7,0x19c68fa2,0x27d0a8e3,0x34ad84b2,0xa92c66be,0x3e99a4fd }
#define GOOD_DATA           { 0xf64684bb,0x51bc1508,0x1a011337 }

const static Identity ID =
{
    .version = 1,
    .product = "MiniOne",
    .serial  = 0x20130712,
};

const static WorkUnit GOOD_WORK =
{
    .midstate = GOOD_MIDSTATE,
    .data = GOOD_DATA,
};

uint32_t CLOCK_CONFIG[2] = { (((uint32_t)DEFAULT_HASHCLOCK) << 18) | CLOCK_LOW_CHG, CLOCK_HIGH_CFG };

//------------------------------------------------------------------------------
static void usbWrite(Application *app, uint8_t *data, uint8_t size)
{
    uint8_t i=0;
    for(i=0; i<size; i++)
        RingBuffer_Insert(&app->USARTtoUSB_Buffer, data[i]);
}

//------------------------------------------------------------------------------
static void sendCmdReply(Application *app, uint8_t cmd, uint8_t *data, uint8_t size)
{
    usbWrite(app, &cmd, 1);
    usbWrite(app, data, size);
}

//------------------------------------------------------------------------------
static void pushWork(Application *app, WorkUnit *work)
{
    uint32_t nonce = 0;

    asicSend(CLOCK_CONFIG, 2);

    asicSend(work->data, 3);
    asicSend(&(work->precalc[1]), 5);
    asicSend(work->midstate, 8);
    asicSend(work->precalc, 1);
    asicSend(&nonce, 1);

    asicIdle();
}

//------------------------------------------------------------------------------
static uint8_t serialProcess(Application *app)
{
    uint16_t count = RingBuffer_GetCount(&app->buffer_);
    if(count == 0)
        return true;

    uint8_t cmd = RingBuffer_Peek(&app->buffer_);
    switch(cmd)
    {
        case 'L':
            RingBuffer_Remove(&app->buffer_);
            LEDs_ToggleLEDs(LEDS_LED1);
            break;

        case 'R':
            RingBuffer_Remove(&app->buffer_);
            asicReset();
            asicResetSpi();
            app->state.state = STATE_RESET;
            sendCmdReply(app, cmd, (uint8_t *)&app->state, sizeof(State));
            break;

        case 'I':
            RingBuffer_Remove(&app->buffer_);
            sendCmdReply(app, cmd, (uint8_t *)&ID, sizeof(ID));
            break;

        case 'A':
            RingBuffer_Remove(&app->buffer_);
            app->state.state = STATE_RESET;
            asicStop();
            sendCmdReply(app, cmd, (uint8_t *)&app->state, sizeof(State));
            break;

        case 'S':
            RingBuffer_Remove(&app->buffer_);
            sendCmdReply(app, cmd, (uint8_t *)&app->state, sizeof(State));
            break;

        case 'W':
            if(count > 44) // 32 bytes midstate + 12 bytes data
            {
                RingBuffer_Remove(&app->buffer_);

                uint8_t i = 0;
                uint8_t *midstate = (uint8_t *)app->worktask.midstate;
                uint8_t *data = (uint8_t *)app->worktask.data;
                for(i=0; i<32; i++)
                {
                    midstate[i] = RingBuffer_Remove(&app->buffer_);
                }
                for(i=0; i<12; i++)
                {
                    data[i] = RingBuffer_Remove(&app->buffer_);
                }

                asicPrecalc(app->worktask.midstate, app->worktask.data, app->worktask.precalc);

                app->state.state = STATE_WORKING;
                app->state.nonce_valid = 0;
                app->state.nonce = 0;

                asicReset();
                asicResetSpi();
                pushWork(app, &app->worktask);
                timerSet(&app->work_timer, 16000);

                sendCmdReply(app, cmd, (uint8_t *)&app->state, sizeof(State));
            }
            break;

        default:
            RingBuffer_Remove(&app->buffer_);
            break;
    }

    return true;
}

//------------------------------------------------------------------------------
void appInit(Application *app)
{
    RingBuffer_InitBuffer(&app->USARTtoUSB_Buffer, app->USARTtoUSB_Buffer_Data, sizeof(app->USARTtoUSB_Buffer_Data));
    RingBuffer_InitBuffer(&app->buffer_, app->buffer_data_, sizeof(app->buffer_data_));

    app->state.state = STATE_RESET;

    asicInit();
    asicStop();
}

//------------------------------------------------------------------------------
void appService(Application *app)
{
    if(asicReportValid())
    {
        uint32_t nonce = asicGetNonce();
        uint32_t nonce_reversed = 0;

        timerStop(&app->work_timer);

        asicStop();
        asicResetSpi();

        app->state.state = STATE_RESET;

        SPCR |= _BV(SPIE);

        nonce_reversed |= nonce & 0xFF;
        nonce >>= 8;
        nonce_reversed <<= 8;
        nonce_reversed |= nonce & 0xFF;
        nonce >>= 8;
        nonce_reversed <<= 8;
        nonce_reversed |= nonce & 0xFF;
        nonce >>= 8;
        nonce_reversed <<= 8;
        nonce_reversed |= nonce & 0xFF;

        nonce_reversed = nonce_reversed - 0xC0;
        app->state.nonce = nonce_reversed;
        app->state.nonce_valid = 1;

        sendCmdReply(app, 'S', (uint8_t *)&app->state, sizeof(State));
    }

    if(timerExpired(&app->work_timer))
    {
        asicStop();
        asicResetSpi();
        app->state.state = STATE_RESET;

        app->state.nonce = 0;
        app->state.nonce_valid = 0;
        sendCmdReply(app, 'S', (uint8_t *)&app->state, sizeof(State));
    }
}

//------------------------------------------------------------------------------
void appUsbDataReceived(Application *app)
{
    int16_t received_byte = 0;
    while((received_byte = CDC_Device_ReceiveByte(app->cdc_info)) >= 0)
    {
        RingBuffer_Insert(&app->buffer_, received_byte);
    }
    serialProcess(app);
}

