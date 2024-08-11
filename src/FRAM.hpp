#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

struct keypair{
    uint8_t slot;
    uint8_t privateKey[32];
    uint8_t publicKey[64];
    uint8_t password_hash[32];
};

namespace FRAM{
    bool writeProtected(){
        return !digitalRead(8);
    }
    void changeWriteProtect(bool status){
        digitalWrite(8, !status);
    }
    void init(){
        pinMode(8, OUTPUT);
        pinMode(SS, OUTPUT);
        digitalWrite(SS, LOW);
        digitalWrite(8, LOW);
        SPI.begin();
        SPI.setClockDivider(SPI_CLOCK_DIV2); //8Mhz for FRAM
    }
    inline uint8_t writeAddress(uint32_t address){
        SPI.transfer(0b00000010);
        SPI.transfer(address >> 16); //high value
        SPI.transfer((address & 0xFFFF) >> 8); //mid value
        return SPI.transfer(address & 0xFF); // low value
    }
    inline uint8_t read(uint32_t address){
        return writeAddress(address);
    }
    inline void write(uint32_t address, uint8_t data){
        SPI.transfer(0b00000110);
        writeAddress(address);
        SPI.transfer(data);
        SPI.transfer(0b00000100);
    }
    void writeKeypair(keypair* key){
        uint32_t address = key->slot * 128;
        if(writeProtected()) return;
        for(uint8_t i = 0; i < 32; i++){
            write(address, key->privateKey[i]);
            address++;
        }
        for(uint8_t i = 0; i < 64; i++){
            write(address, key->publicKey[i]);
            address++;
        }
        for(uint8_t i = 0; i < 32; i++){
            write(address, key->password_hash[i]);
            address++;
        }
    }
    keypair* readKeypair(uint8_t slot, keypair* key){
        uint32_t address = key->slot * 128;
        for(uint8_t i = 0; i < 32; i++){
            key->privateKey[i] = read(address);
            address++;
        }
        for(uint8_t i = 0; i < 64; i++){
            key->publicKey[i] = read(address);
            address++;
        }
        for(uint8_t i = 0; i < 32; i++){
            key->password_hash[i] = read(address);
            address++;
        }
        return key;
    }
}