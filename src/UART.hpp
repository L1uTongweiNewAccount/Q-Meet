#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

struct HexMeta{
    uint8_t byte_count;
    uint16_t method;
    uint8_t slot;
    uint8_t checksum;
};
extern struct HexMeta failed;
extern struct HexMeta returned;
extern uint8_t SerialBuffer[64];

void SerialInit(){ //4800bps@16Mhz
    Serial.begin(4800);
}

void writeByte(uint8_t dat){
    if((dat >> 4) >= 10) Serial.write((dat >> 4) - 10 + 'A');
    else Serial.write((dat >> 4) + '0');
    if((dat & 0xF) >= 10) Serial.write((dat & 0xF) - 10 + 'A');
    else Serial.write((dat & 0xF) + '0');
}

int16_t readByte(void){
    uint8_t datA = Serial.read(), datB = Serial.read(), result = 0;
    if(datA >= '0' && datA <= '9') result += 16 * (datA - '0');
    else if(datA >= 'A' && datA <= 'F') result += 16 * (datA - 'A' + 10);
    else if(datA >= 'a' && datA <= 'f') result += 16 * (datA - 'a' + 10);
    else return -1;
    if(datB >= '0' && datB <= '9') result += datB - '0';
    else if(datB >= 'A' && datB <= 'F') result += datB - 'A' + 10;
    else if(datB >= 'a' && datB <= 'f') result += datB - 'a' + 10;
    else return -1;
    return result;
}

void SerialSend(uint8_t byte_count, uint16_t method, uint8_t slot){
    uint8_t sum = 0;
    Serial.write(':');
    writeByte(byte_count), sum += byte_count;
    writeByte(method >> 8), sum += (method >> 8);
    writeByte(method & 0xFF), sum += (method & 0xFF);
    writeByte(slot), sum += slot;
    for(uint8_t i = 0; i < byte_count; i++){
        writeByte(SerialBuffer[i]), sum += SerialBuffer[i];
    }
    writeByte(0x100 - sum);
    Serial.write('\n');
}

void SerialRecv(void){
    uint8_t dat = 0, sum = 0;
    int16_t status = 0;
    struct HexMeta meta;
    memset(SerialBuffer, 0, sizeof(SerialBuffer));
    while((dat = Serial.read()) != ':');
    if((status = readByte()) == -1) {returned = failed; return;}
    meta.byte_count = (uint8_t)status, sum += status;
    if(meta.byte_count > 64) {returned = failed; return;}
    if((status = readByte()) == -1) {returned = failed; return;}
    meta.method = (status << 8), sum += status;
    if((status = readByte()) == -1) {returned = failed; return;}
    meta.method |= status, sum += status;
    if((status = readByte()) == -1) {returned = failed; return;}
    meta.slot = (uint8_t)status, sum += status;
    for(uint8_t i = 0; i < meta.byte_count; i++){
        if((status = readByte()) == -1){
            memset(SerialBuffer, 0, sizeof(SerialBuffer));
            returned = failed;
            return;
        }
        SerialBuffer[i] = (uint8_t)status, sum += status;
    }
    if((status = readByte()) == -1) {returned = failed; return;}
    meta.checksum = (uint8_t)status;
    if((uint8_t)(meta.checksum + sum) != 0) {returned = failed; return;}
    returned = meta;
}

#endif