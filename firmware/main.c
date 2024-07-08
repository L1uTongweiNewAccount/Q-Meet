#define uECC_CURVE uECC_secp256k1
#define uECC_ASM uECC_asm_none
#define uECC_PLATFORM uECC_arch_other
#define uECC_WORD_SIZE 1
#include "micro-ecc/uECC.h"
#include "micro-ecc/uECC.c"
#define BACK_TO_TABLES
#include "aes256/aes256.h"
#include "aes256/aes256.c"
#include "sha-2/sha-256.h"
#include "sha-2/sha-256.c"
#include "mt19937ar/mt19937ar.c"
#include <stdlib.h>

struct HexMeta failed = {0, 0, 0, 0};
struct HexMeta returned = {0, 0, 0, 0};
__xdata uint8_t FRAM[0x2000]; //Please place this first so that its addr = 0x0000
__xdata uint8_t SerialBuffer[0x100], OutputBuffer[0x100], PublicKeyBuffer[0x40], PrivateKeyBuffer[0x20];
__xdata uint8_t SharedKeyBuffer[0x20], OtherPublicKeyBuffer[0x40], HashBuffer[0x32];
__xdata aes256_context AES256Context;
__xdata struct Sha_256 SHA256Context;
__xdata unsigned long mt[N];
long timerCount = 0;
int16_t openedSlot = -1;

#include "reg8051.h"
#include "Serial.h"
#define SLOT(x) (FRAM + 0x20 * x)
void TimerInit(void){
	TMOD &= 0xF0;
	TL0 = 0xE8;
	TH0 = 0xFF;
	TF0 = 0;
	TR0 = 1;
}
void TimerInterrupt(void) __interrupt(1) {timerCount++;}
int RNG(uint8_t *dest, unsigned size){
    init_genrand(timerCount);
    while(size > 4){
        *(uint32_t*)dest = genrand_int32();
        dest += 4, size -= 4;
    }
    while(size--) *(dest++) = (uint8_t)genrand_int32();
    return 1;
}

int main(void){
    SerialInit();
    TimerInit();
    uECC_set_rng(RNG);
    while(1){
        SerialRecv();
        if(returned.method){
            switch(returned.method){
                case 0x8708:{ // Create a slot
                    static uint8_t* __xdata slot;
                    slot = SLOT(returned.slot);
                    memset(PrivateKeyBuffer, 0, 0x20);
                    memset(PublicKeyBuffer, 0, 0x40);
                    memset(HashBuffer, 0, 0x32);
                    calc_sha_256(HashBuffer, SerialBuffer, returned.byte_count);
                    aes256_init(&AES256Context, HashBuffer);
                    uECC_make_key(PublicKeyBuffer, PrivateKeyBuffer);
                    memcpy(slot, PrivateKeyBuffer, 0x20);
                    aes256_encrypt_ecb(&AES256Context, slot);
                    aes256_done(&AES256Context);
                    openedSlot = returned.slot;
                    returned.byte_count = 1;
                    OutputBuffer[0] = 0;
                    SerialSend(returned.byte_count, returned.method, returned.slot);
                    break;
                }
                case 0x1c8d:{ // Open a slot
                    static uint8_t* __xdata slot;
                    slot = SLOT(returned.slot);
                    memset(PrivateKeyBuffer, 0, 0x20);
                    memset(PublicKeyBuffer, 0, 0x40);
                    memset(HashBuffer, 0, 0x32);
                    calc_sha_256(HashBuffer, SerialBuffer, returned.byte_count);
                    aes256_init(&AES256Context, HashBuffer);
                    memcpy(PrivateKeyBuffer, slot, 0x20);
                    aes256_decrypt_ecb(&AES256Context, slot);
                    aes256_done(&AES256Context);
                    openedSlot = returned.slot;
                    returned.byte_count = 1;
                    OutputBuffer[0] = 0;
                    SerialSend(returned.byte_count, returned.method, returned.slot);
                    break;
                }
                case 0xe6c5: case 0x4acb: case 0x4bd8: case 0xe64a:{ // En/Decrypt with secret/shared
                    static uint8_t i;
                    if(openedSlot == -1){
                        returned.byte_count = 1;
                        OutputBuffer[0] = 1;
                        SerialSend(returned.byte_count, returned.method, returned.slot);
                        break;
                    }
                    memcpy(OutputBuffer, SerialBuffer, returned.byte_count);
                    if(returned.method == 0xe6c5 || returned.method == 0x4acb) aes256_init(&AES256Context, PrivateKeyBuffer);
                    else{
                        if(!uECC_valid_public_key(OtherPublicKeyBuffer)){
                            returned.byte_count = 1;
                            OutputBuffer[0] = 2;
                            SerialSend(returned.byte_count, returned.method, returned.slot);
                            break;
                        }
                        uECC_shared_secret(OtherPublicKeyBuffer, PrivateKeyBuffer, SharedKeyBuffer);
                        aes256_init(&AES256Context, SharedKeyBuffer);
                    }
                    for(i = 0; i < 8; i++){
                        if(0x20 * i > returned.byte_count){
                            break;
                        }
                        if(returned.method == 0xe6c5 || returned.method == 0x4bd8) aes256_encrypt_ecb(&AES256Context, OutputBuffer + 0x20 * i);
                        else aes256_decrypt_ecb(&AES256Context, OutputBuffer + 0x20 * i);
                    }
                    aes256_done(&AES256Context);
                    returned.byte_count = 0x20 * (i + 1);
                    SerialSend(returned.byte_count, returned.method, returned.slot);
                    memset(SharedKeyBuffer, 0, 0x40);
                    break;
                }
                case 0xd720:{ // Get Public Key
                    if(openedSlot == -1){
                        returned.byte_count = 1;
                        OutputBuffer[0] = 1;
                        SerialSend(returned.byte_count, returned.method, returned.slot);
                        break;
                    }
                    memcpy(OutputBuffer, PublicKeyBuffer, 0x40);
                    returned.byte_count = 0x40;
                    SerialSend(returned.byte_count, returned.method, returned.slot);
                    break;
                }
                case 0xd3a2:{ // Clear Other's Public Key
                    memset(OtherPublicKeyBuffer, 0, 0x40);
                    returned.byte_count = 1;
                    OutputBuffer[0] = 0;
                    SerialSend(returned.byte_count, returned.method, returned.slot);
                    break;
                }
                case 0xdcab:{ // Close a slot
                    openedSlot = -1;
                    memset(PrivateKeyBuffer, 0, 0x20);
                    memset(PublicKeyBuffer, 0, 0x40);
                    OutputBuffer[0] = 0;
                    SerialSend(returned.byte_count, returned.method, returned.slot);
                    break;
                }
            }
            memset(OutputBuffer, 0, 0x100);
            memset(&returned, 0, sizeof(returned));
        }
    }
}