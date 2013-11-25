#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "clock.h"

static uint32_t time_;

//------------------------------------------------------------------------------
ISR(TIMER0_COMPA_vect)
{
    time_++;
}

//------------------------------------------------------------------------------
void clockInit(void)
{
    time_ = 0;
    
    // Interrupt every millisecond
    TIMSK0 |= _BV(OCIE0A);
    OCR0A  = 249;
    TCCR0A = _BV(WGM01);
    TCCR0B = 0x03;
}

//------------------------------------------------------------------------------
uint32_t clockGetTime(void)
{
    volatile uint32_t ret = 0;
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        ret = time_;
    }
    return ret;
}

