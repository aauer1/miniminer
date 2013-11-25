/*
 LUFA Library
 Copyright (C) Dean Camera, 2012.

 dean [at] fourwalledcubicle [dot] com
 www.lufa-lib.org
 */

/*
 Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

 Permission to use, copy, modify, distribute, and sell this
 software and its documentation for any purpose is hereby granted
 without fee, provided that the above copyright notice appear in
 all copies and that both that the copyright notice and this
 permission notice and warranty disclaimer appear in supporting
 documentation, and that the name of the author not be used in
 advertising or publicity pertaining to distribution of the
 software without specific, written prior permission.

 The author disclaim all warranties with regard to this
 software, including all implied warranties of merchantability
 and fitness.  In no event shall the author be liable for any
 special, indirect or consequential damages or any damages
 whatsoever resulting from loss of use, data or profits, whether
 in an action of contract, negligence or other tortious action,
 arising out of or in connection with the use or performance of
 this software.
 */

/** \file
 *
 *  Main source file for the USBtoSerial project. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include <stdio.h>

#include "XilinxFpgaProg.h"
#include "XilinxProtocol.h"
#include "clock.h"
#include "timer.h"

#include "application.h"

static void debug(char c, FILE *stream);
static FILE debug_stdout = FDEV_SETUP_STREAM(debug, NULL, _FDEV_SETUP_WRITE);

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
{
    .Config =
    {
            .ControlInterfaceNumber = 0,
            .DataINEndpoint =
            {
                .Address = CDC_TX_EPADDR,
                .Size = CDC_TXRX_EPSIZE,
                .Banks = 1,
            },
            .DataOUTEndpoint =
            {
                .Address = CDC_RX_EPADDR,
                .Size = CDC_TXRX_EPSIZE,
                .Banks = 1,
            },
            .NotificationEndpoint =
            {
                .Address = CDC_NOTIFICATION_EPADDR,
                .Size = CDC_NOTIFICATION_EPSIZE,
                .Banks = 1,
            },
    },
};

static Application app =
{
    .cdc_info = &VirtualSerial_CDC_Interface,
};

//------------------------------------------------------------------------------
ISR(USART1_RX_vect, ISR_BLOCK)
{
    uint8_t ReceivedByte = UDR1;
    if (USB_DeviceState == DEVICE_STATE_Configured)
        appSerialDataReceived(&app, ReceivedByte);
}

//------------------------------------------------------------------------------
static void debug(char c, FILE *stream)
{
    if(c == '\n')
        RingBuffer_Insert(&app.USARTtoUSB_Buffer, '\r');
    RingBuffer_Insert(&app.USARTtoUSB_Buffer, c);
}

//------------------------------------------------------------------------------
int main(void)
{
    Timer timer;

    stdout = &debug_stdout;

    SetupHardware();

    appInit(&app);

    LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);

    timerSet(&timer, 10);
    sei();

    for(;;)
    {
        if(CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface))
        {
            appUsbDataReceived(&app);
        }

        uint16_t BufferCount = RingBuffer_GetCount(&app.USARTtoUSB_Buffer);
        if(timerExpired(&timer) || (BufferCount > 0))
        {
            timerRestart(&timer);
            while(BufferCount--)
            {
                if(CDC_Device_SendByte(&VirtualSerial_CDC_Interface, RingBuffer_Peek(&app.USARTtoUSB_Buffer)) != ENDPOINT_READYWAIT_NoError)
                {
                    break;
                }

                RingBuffer_Remove(&app.USARTtoUSB_Buffer);
            }
        }

        appService(&app);
        CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
        USB_USBTask();
    }
}

//------------------------------------------------------------------------------
/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    DDRB = 0x27;
    PORTB = 0x38;

    DDRC = 0x20;
    PORTC = 0x00;

    DDRD = 0x08;
    PORTD = 0x44;

    TCCR1A = _BV(COM1B0);
    OCR1B  = 0;
    TCCR1B = _BV(WGM12) | _BV(CS10);
    
    clock_prescale_set(clock_div_1);

    clockInit();

    LEDs_Init();
    USB_Init();
    //Serial_Init(38400, false);
    
    serialInit();
    xilinxInit();
}

//------------------------------------------------------------------------------
/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
    LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

//------------------------------------------------------------------------------
/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

//------------------------------------------------------------------------------
/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;

    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

    LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

//------------------------------------------------------------------------------
/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

//------------------------------------------------------------------------------
/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
    uint8_t ConfigMask = 0;

    switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
    {
            case CDC_PARITY_Odd:
                    ConfigMask = ((1 << UPM11) | (1 << UPM10));
                    break;
            case CDC_PARITY_Even:
                    ConfigMask = (1 << UPM11);
                    break;
    }

    if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
        ConfigMask |= (1 << USBS1);

    switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
    {
        case 6:
            ConfigMask |= (1 << UCSZ10);
            break;
        case 7:
            ConfigMask |= (1 << UCSZ11);
            break;
        case 8:
            ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
            break;
    }

    /* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
    UCSR1B = 0;
    UCSR1A = 0;
    UCSR1C = 0;

    /* Set the new baud rate before configuring the USART */
    UBRR1  = SERIAL_2X_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS);

    /* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
    UCSR1C = ConfigMask;
    UCSR1A = (1 << U2X1);
    UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
}

