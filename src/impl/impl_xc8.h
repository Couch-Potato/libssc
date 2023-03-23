#pragma once
#include "libssc.h"

void __BEGIN(uint32_t baud_rate)
{
    int16_t n = (_XTAL_FREQ / (16 * baud_rate)) - 1;

    if (n < 0)
        n = 0;

    if (n > 255) // low speed
    {
        n = (_XTAL_FREQ / (64 * baud_rate)) - 1;
        if (n > 255)
            n = 255;
        SPBRG = n;
        TXSTA = 0x20; // transmit enabled, low speed mode
    }

    else // high speed
    {
        SPBRG = n;
        TXSTA = 0x24; // transmit enabled, high speed mode
    }

    RCSTA = 0x90; // serial port enabled, continues receive enabled
}

int __READY(){
    return RCIF;
}

uint8_t __READ(){
    while (RCIF == 0)
        ;     // wait for data receive
    if (OERR) // if there is overrun error
    {         // clear overrun error bit
        CREN = 0;
        CREN = 1;
    }
    return RCREG; // read from EUSART receive data register
}

void __WRITE(char a){
    while (TRMT == 0);         // wait for transmit shift register to be empty
    TXREG = data; // update EUSART transmit data register
}

SCommunicationAdapter *GetDeviceSerialAdapter()
{
    return SCreateAdapter(
        &__BEGIN,
        &__READY,
        &__READ,
        &__WRITE
    );
}