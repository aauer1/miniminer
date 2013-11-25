#include <avr/io.h>

#include "timer.h"
#include "clock.h"

//------------------------------------------------------------------------------
void timerSet(Timer *t, uint32_t interval)
{
    t->active = 1;
    t->start = clockGetTime();
    t->interval = interval;
}

//------------------------------------------------------------------------------
void timerStop(Timer *t)
{
    t->active = 0;
}

//------------------------------------------------------------------------------
void timerReset(Timer *t)
{
    t->active = 1;
    t->start += t->interval;
}

//------------------------------------------------------------------------------
void timerRestart(Timer *t)
{
    t->active = 1;
    t->start = clockGetTime();
}

//------------------------------------------------------------------------------
unsigned char timerExpired(Timer *t)
{
    uint32_t time = clockGetTime();

    if(!t->active)
    {
        return 0;
    }
    
    if(time > t->start)
    {
        uint32_t diff = (time - t->start);
        if(diff > t->interval)
        {
            t->active = 0;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if(((~t->start) + time + 1) >= t->interval)
        {
            t->active = 0;
            return 1;
        }
        else
        {
            return 0;
        }
    }
}
