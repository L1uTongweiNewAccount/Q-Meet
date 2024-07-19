#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
    #include <windows.h>
    HANDLE Serial = NULL;
    DCB opt;
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <errno.h>
    int Serial = -1;
    struct termios opt;
#endif

struct HexMeta{
    uint8_t byte_count;
    uint16_t method;
    uint8_t slot;
    uint8_t checksum;
};
extern struct HexMeta failed;
extern struct HexMeta returned;
extern uint8_t SerialBuffer[0x100];
extern uint8_t OutputBuffer[0x100];

void SerialInit(const char* path){ //4800bps
    #ifdef _WIN32
        Serial = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if(Serial == (HANDLE)-1) exit(2);
        memset(&opt, 0, sizeof(opt));
        opt.DCBlength = sizeof(opt);
        opt.BaudRate = 4800;
        opt.ByteSize = 8;
        opt.Parity = NOPARITY;
        opt.StopBits = ONESTOPBIT;
        if(!SetCommState(Serial, &opt)) exit(3);
    #else
        Serial = open(path, O_RDWR | O_NOCTTY);
        if(Serial == -1) exit(2);
        tcgetattr(Serial, &opt);
        cfsetispeed(&opt, B4800);
        cfsetospeed(&opt, B4800);
        opt.c_cflag &= ~PARENB;
        opt.c_cflag &= ~CSTOPB;
        opt.c_cflag &= ~CSIZE;
        opt.c_cflag |= ~CS8;
        opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        opt.c_oflag &= ~OPOST;
        if(tcsetattr(Serial, TCSANOW, &opt)) exit(3);
        tcflush(Serial, TCIOFLUSH);
    #endif
}

void writeByte(uint8_t dat){
    char character = 0, writed = 0;
    if((dat >> 4) > 10) character = (dat >> 4) - 10 + 'A';
    else character = (dat >> 4) + '0';
    #ifdef _WIN32
        WriteFile(Serial, &character, 1, &writed, NULL);
    #else
        write(Serial, &character, 1);
    #endif
    if((dat & 0xF) > 10) character = (dat & 0xF) - 10 + 'A';
    else character = (dat & 0xF) + '0';
    #ifdef _WIN32
        WriteFile(Serial, &character, 1, &writed, NULL);
    #else
        write(Serial, &character, 1);
    #endif
}

int16_t readByte(void){
    uint8_t dat[2], result = 0, readed = 0;
    #ifdef _WIN32
        ReadFile(Serial, dat, 2, &readed, NULL);
    #else
        read(Serial, dat, 2);
    #endif
    if(dat[0] >= '0' && dat[0] <= '9') result += 16 * (dat[0] - '0');
    else if(dat[0] >= 'A' && dat[0] <= 'F') result += 16 * (dat[0] - 'A' + 10);
    else if(dat[0] >= 'a' && dat[0] <= 'f') result += 16 * (dat[0] - 'a' + 10);
    else return -1;
    if(dat[1] >= '0' && dat[1] <= '9') result += dat[1] - '0';
    else if(dat[1] >= 'A' && dat[1] <= 'F') result += dat[1] - 'A' + 10;
    else if(dat[1] >= 'a' && dat[1] <= 'f') result += dat[1] - 'a' + 10;
    else return -1;
    return result;
}

void SerialSend(uint8_t byte_count, uint16_t method, uint8_t slot){
    uint8_t sum = 0, character = ':', writed = 0;
    #ifdef _WIN32
        WriteFile(Serial, &character, 1, &writed, NULL);
    #else
        write(Serial, &character, 1);
    #endif
    writeByte(byte_count), sum += byte_count;
    writeByte(method >> 8), sum += (method >> 8);
    writeByte(method & 0xFF), sum += (method & 0xFF);
    writeByte(slot), sum += slot;
    for(uint8_t i = 0; i < byte_count; i++){
        writeByte(OutputBuffer[i]), sum += OutputBuffer[i];
    }
    writeByte(0x100 - sum);
    character = '\n';
    #ifdef _WIN32
        WriteFile(Serial, &character, 1, &writed, NULL);
    #else
        write(Serial, &character, 1);
    #endif
}

void SerialRecv(void){
    uint8_t dat = 0, sum = 0, readed = 0;
    int16_t status = 0;
    struct HexMeta meta;
    memset(SerialBuffer, 0, sizeof(SerialBuffer));
    while(dat != ':'){
        #ifdef _WIN32
            ReadFile(Serial, &dat, 1, &readed, NULL);
        #else
            read(Serial, &dat, 1);
        #endif
    }
    if((status = readByte()) == -1) {returned = failed; return;}
    meta.byte_count = (uint8_t)status, sum += status;
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
    if(meta.checksum + sum != 0) {returned = failed; return;}
    returned = meta;
}

#endif