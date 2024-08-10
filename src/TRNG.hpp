#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>

uint32_t generating_random_seed(){
    uint32_t seed = 0;
    pinMode(A0, INPUT);
    for(uint8_t* val = (uint8_t*)&seed; val < (uint8_t*)&seed + 4; val++){
        for (unsigned i = 0; i < 8; ++i) {
            int init = analogRead(A0), count = 0;
            while(analogRead(A0) == init) count++;
            if(count == 0) *val = (*val << 1) | (init & 0x01);
            else *val = (*val << 1) | (count & 0x01);
        }
    }
    return 1;
}