#define uECC_CURVE uECC_secp256k1
#include "micro-ecc/uECC.h"
#include "../libs/sha-2/sha-256.h"
#include "../libs/argparse/argparse.h"
#include "../libs/SerialEmulator.h"
#include "../libs/base64/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct HexMeta failed;
struct HexMeta returned;
uint8_t SerialBuffer[0x100];
uint8_t OutputBuffer[0x100];
uint8_t PublicKeyBuffer[0x100];
FILE *inputFile, *outputFile;
struct Sha_256 SHA256Context;

int main(int argc, const char** argv){
    const char *publicKey = NULL, *serialPort = NULL;
    const char *input = NULL, *output = NULL, *password = NULL;
    int getPublicKey = 0, slot = -1, disableSign = 0, signature = 0;
    int directEncrypt = 0, directDecrypt = 0, create = 0, verify = 0;
    int ecdheEncrypt = 0, ecdheDecrypt = 0, addition = 0;
    const char* usages[] = {
        "qmeet-cli [Mode] [options]",
        NULL
    };
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Modes"),
        OPT_BOOLEAN(0, "direct-encrypt", &directEncrypt, "Encrypt a file directly.", NULL, 0, 0),
        OPT_BOOLEAN(0, "direct-decrypt", &directDecrypt, "Decrypt a file directly.", NULL, 0, 0),
        OPT_BOOLEAN(0, "ecdhe-encrypt", &ecdheEncrypt, "Encrypt a message with ECDHE algorithm.", NULL, 0, 0),
        OPT_BOOLEAN(0, "ecdhe-decrypt", &ecdheDecrypt, "Decrypt a message with ECDHE algorithm.", NULL, 0, 0),
        OPT_BOOLEAN(0, "get-publickey", &getPublicKey, "Get your public key.", NULL, 0, 0),
        OPT_BOOLEAN(0, "create", &create, "Create a new slot. (May delete old data!)", NULL, 0, 0),
        OPT_BOOLEAN(0, "signature", &signature, "Signature a file.", NULL, 0, 0),
        OPT_BOOLEAN(0, "verify", &verify, "Verify a signature.", NULL, 0, 0),
        OPT_GROUP("Required Options (Not required for verify)"),
        OPT_STRING('c', "serialport", &serialPort, "The serial port of Q-Meet hardware.", NULL, 0, 0),
        OPT_INTEGER('s', "slot", &serialPort, "The key slot of Q-Meet hardware. (0-255)", NULL, 0, 0),
        OPT_STRING('p', "password", &password, "The password of slot.", NULL, 0, 0),
        OPT_GROUP("Input/Output Options"),
        OPT_STRING('i', "input", &input, "The input file. If not designated, I will input it in stdin.", NULL, 0, 0),
        OPT_STRING('o', "output", &output, "The output file. If not designated, I will output it in stdout.", NULL, 0, 0),
        OPT_BOOLEAN('a', "addition", &addition, "Addition for output file"),
        OPT_GROUP("ECDHE or verify Required Option"),
        OPT_STRING(0, "publickey", &publicKey, "The public key file of counterpart.", NULL, 0, 0),
        OPT_GROUP("Signature Option"),
        OPT_BOOLEAN(0, "disable-sign", &disableSign, "Disable automatic signature.", NULL, 0, 0),
        OPT_END()
    };
    struct argparse argparse;
    uint16_t magicNumber = 0;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\nA utility for encrypt/decrypt your file with Q-Meet hardware", "\nThis program is free software: it under the GPL-3.0+ license.");
    argc = argparse_parse(&argparse, argc, argv);

    if(directEncrypt + directDecrypt + ecdheEncrypt + ecdheDecrypt + getPublicKey + create + signature + verify != 1){
        puts("Error: More than 1 mode or no mode designated.");
        puts("Run qmeet-cli -h to see help message.");
        return 1;
    }
    if(input == NULL) inputFile = stdin;
    else inputFile = fopen(input, "r");
    if(output == NULL) outputFile = stdout;
    else if(addition) outputFile = fopen(output, "a");
    else outputFile = fopen(output, "w");

    if((ecdheEncrypt || ecdheDecrypt || verify) && publicKey == NULL){
        puts("Error: Public key not designated.");
        puts("Run qmeet-cli -h to see help message.");
        return 1;
    }else if(ecdheEncrypt || ecdheDecrypt || verify){
        char* buf = malloc(0x100);
        FILE* pub = fopen(publicKey, "r");
        fgets(buf, 0x100, pub);
        base64_decode(buf, strlen(buf), PublicKeyBuffer);
        free(buf);
    }

    if(verify){
        char* buf = malloc(0x100);
        fgets(buf, 0x100, inputFile);
        base64_decode(buf, strlen(buf), SerialBuffer);
        free(buf);
        size_t fileSize = 0, curOffset = 0;
        curOffset = ftell(inputFile);
        fseek(inputFile, 0, SEEK_END);
        fileSize = ftell(inputFile);
        fseek(inputFile, curOffset, SEEK_SET);
        uint8_t *buf2 = malloc(fileSize);
        fread(buf2, fileSize, 1, inputFile);
        calc_sha_256(OutputBuffer, buf2, fileSize);
        free(buf2);
        if(uECC_verify(PublicKeyBuffer, OutputBuffer, SerialBuffer)){
            puts("Verify Successed.");
            return 0;
        }else{
            puts("Incorrect signature.");
            return 1;
        }
    }

    if(slot == -1 || serialPort == NULL || password == NULL){
        puts("Error: slot, password or serial port not designated.");
        puts("Run qmeet-cli -h to see help message.");
        return 1;
    }
    SerialInit(serialPort);
    if(create){
        strcpy((char*)OutputBuffer, password);
        SerialSend(strlen(password), 0x8708, slot); //Create a slot
        SerialRecv();
        puts("Create done. Please remember your password, because nowhere will save it and wrong password leads to wrong key.");
        return 0;
    }

    strcpy((char*)OutputBuffer, password);
    SerialSend(strlen(password), 0x1c8d, slot); //Open a slot
    SerialRecv();
    if(signature){
        size_t fileSize = 0, curOffset = 0;
        curOffset = ftell(inputFile);
        fseek(inputFile, 0, SEEK_END);
        fileSize = ftell(inputFile);
        fseek(inputFile, curOffset, SEEK_SET);
        uint8_t* buf = malloc(fileSize);
        fread(buf, fileSize, 1, inputFile);
        calc_sha_256(OutputBuffer, buf, fileSize);
        free(buf);
        SerialSend(0x20, 0x6258, slot);
        SerialRecv();
        if(returned.byte_count == 1) exit(SerialBuffer[0]);
        char* out = malloc(0x100);
        base64_encode(SerialBuffer, 0x40, out);
        fputs(out, outputFile);
        free(out);
        return 0;
    }
    if(getPublicKey){
        SerialSend(0, 0xd720, slot);
        char* out = malloc(0x100);
        base64_encode(SerialBuffer, 0x40, out);
        fputs(out, outputFile);
        free(out);
        return 0;
    }

    if(ecdheEncrypt || ecdheDecrypt){
        memcpy(OutputBuffer, PublicKeyBuffer, 0x40);
        SerialSend(0x40, 0x7898, slot);
        memset(OutputBuffer, 0, sizeof(OutputBuffer));
    }
    uint16_t readSize = 0, oper = 0;
    if(directEncrypt) oper = 0xe6c5;
    else if(directDecrypt) oper = 0x4acb;
    else if(ecdheEncrypt) oper = 0x4bd8;
    else if(ecdheDecrypt) oper = 0xe64a;
    while((readSize = fread(OutputBuffer, 1, 0x100, inputFile)) != 0){
        SerialSend(readSize, oper, slot);
        SerialRecv();
        fwrite(SerialBuffer, 1, 0x100, outputFile);
    }

    SerialSend(0, 0xd3a2, slot);
    SerialSend(0, 0xdcab, slot);
    return 0;
}