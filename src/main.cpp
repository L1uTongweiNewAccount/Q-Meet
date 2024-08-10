#define uECC_PLATFORM uECC_avr
#define uECC_CURVE uECC_secp256k1
#define uECC_ASM uECC_asm_small
#define uECC_SQUARE_FUNC 0
#define BACK_TO_TABLES
//#define SERIAL_RX_BUFFER_SIZE 512
//HardwareSerial.h:113 RX buffer's size = 512
#include <Arduino.h>
#include <aes256.h>
#include <sha-256.h>
#include <uECC.h>
#include "UART.hpp"
#include "FRAM.hpp"
#include "TRNG.hpp"

#define ERROR 0
#define LOGIN 1
#define REGISTER 2
#define EXIT 3
#define ENCRYPT 4
#define DECRYPT 5
#define ENCRYPT_ECDH 6
#define DECRYPT_ECDH 7
#define CALC_ECDH_SECRET 8

#define SUCCESSED 0
#define PASSWORD_INCORRECT 1
#define HAS_NOT_LOGINED 2
#define INCORRECT_COMMAND 3
#define INCORRECT_SIGNATURE 4
#define NO_PUBLIC_KEY 5
#define ERROR_LENGTH 6
#define VERIFY_FAILED 7
#define ERROR_PUBLIC_KEY 8

struct HexMeta failed = {0, ERROR, 0, 0};
struct HexMeta returned;
uint8_t SerialBuffer[64];
uint8_t hash[32], secret[32], other_publicKey[64];
keypair key;
bool logined = false, publiced = false;
aes256_context context;

inline void returnStatus(uint8_t status){
    memset(SerialBuffer, 0, sizeof(SerialBuffer));
    SerialBuffer[0] = status;
    SerialSend(1, returned.method, returned.slot);
}
static int RNG(uint8_t *dest, unsigned size) {
    uint32_t randomBuffer = 0;
    while(size > 0){
        if(size > 32){
            randomBuffer = generating_random_seed();
            calc_sha_256(dest, &randomBuffer, sizeof(randomBuffer));
            dest += 32, size -= 32;
        }else{
            for(uint8_t* p = dest; size > 0; size--){
                *p = (uint8_t)generating_random_seed();
            }
        }
    }
    return 1;
}

int main(){
	init(); FRAM::init();
    uECC_set_rng(RNG);
    SerialInit();
    while(true){
        memset(hash, 0, sizeof(hash));
        SerialRecv();
        if(returned.method == ERROR){
            returnStatus(INCORRECT_COMMAND);
            continue;
        }
        switch(returned.method){
            case LOGIN: //Login to terminal
                FRAM::readKeypair(returned.slot, &key);
                calc_sha_256(hash, SerialBuffer, returned.byte_count);
                if(memcmp(hash, key.password_hash, 32)){
                    returnStatus(PASSWORD_INCORRECT);
                    break;
                }
                aes256_init(&context, SerialBuffer);
                aes256_decrypt_ecb(&context, key.privateKey);
                aes256_done(&context);
                logined = true;
                returnStatus(SUCCESSED);
                break;
            case REGISTER:
                key.slot = returned.slot;
                calc_sha_256(key.password_hash, SerialBuffer, returned.byte_count);
                uECC_make_key(key.publicKey, key.privateKey);
                aes256_init(&context, SerialBuffer);
                aes256_encrypt_ecb(&context, key.privateKey);
                FRAM::writeKeypair(&key);
                aes256_decrypt_ecb(&context, key.privateKey);
                aes256_done(&context);
                logined = true;
                returnStatus(SUCCESSED);
                break;
            case EXIT:
                logined = publiced = false;
                memset(&key, 0, sizeof(key));
                memset(secret, 0, sizeof(secret));
                returnStatus(SUCCESSED);
                break;
            case ENCRYPT: case ENCRYPT_ECDH:
                if(logined == false){
                    returnStatus(HAS_NOT_LOGINED);
                    break;
                }
                if(returned.method == ENCRYPT_ECDH && publiced == false){
                    returnStatus(NO_PUBLIC_KEY);
                    break;
                }
                if(returned.byte_count > 32){
                    returnStatus(ERROR_LENGTH);
                    break;
                }
                calc_sha_256(hash, SerialBuffer, returned.byte_count);
                if(returned.method == ENCRYPT) aes256_init(&context, key.privateKey);
                else aes256_init(&context, secret);
                aes256_encrypt_ecb(&context, SerialBuffer);
                aes256_done(&context);
                uECC_sign(key.privateKey, hash, SerialBuffer + 32);
                SerialSend(64, returned.method, returned.slot);
                break;
            case DECRYPT: case DECRYPT_ECDH:
                if(logined == false){
                    returnStatus(HAS_NOT_LOGINED);
                    break;
                }
                if(publiced == false){
                    returnStatus(NO_PUBLIC_KEY);
                    break;
                }
                if(returned.byte_count != 64){
                    returnStatus(ERROR_LENGTH);
                    break;
                }
                if(returned.method == DECRYPT) aes256_init(&context, key.privateKey);
                else aes256_init(&context, secret);
                aes256_decrypt_ecb(&context, SerialBuffer);
                aes256_done(&context);
                calc_sha_256(hash, SerialBuffer, returned.byte_count);
                if(!uECC_verify(other_publicKey, hash, SerialBuffer + 32)){
                    returnStatus(VERIFY_FAILED);
                    break;
                }
                SerialSend(32, returned.method, returned.slot);
                break;
            case CALC_ECDH_SECRET:
                if(logined == false){
                    returnStatus(HAS_NOT_LOGINED);
                    break;
                }
                if(returned.byte_count != 32){
                    returnStatus(ERROR_LENGTH);
                    break;
                }
                if(!uECC_valid_public_key(SerialBuffer)){
                    returnStatus(ERROR_PUBLIC_KEY);
                    break;
                }
                memcpy(other_publicKey, SerialBuffer, 32);
                uECC_shared_secret(other_publicKey, key.privateKey, secret);
                returnStatus(SUCCESSED);
                break;
            default:
                returnStatus(INCORRECT_COMMAND);
                break;
        }
    }
	return 0;
}