#pragma once

#include <stdint.h>
#define MIN_LOOP 8
#define PRE_LOOP 8
#define TINYMT32_MEXP 127
#define TINYMT32_SH0 1
#define TINYMT32_SH1 10
#define TINYMT32_SH8 8
#define TINYMT32_MASK UINT32_C(0x7fffffff)
#define TINYMT32_MUL (1.0f / 16777216.0f)

typedef struct {
    uint32_t status[4];
    uint32_t mat1;
    uint32_t mat2;
    uint32_t tmat;
}tinymt32_t;

inline static void tinymt32_next_state(tinymt32_t * random) {
    uint32_t x;
    uint32_t y;

    y = random->status[3];
    x = (random->status[0] & TINYMT32_MASK)
        ^ random->status[1]
        ^ random->status[2];
    x ^= (x << TINYMT32_SH0);
    y ^= (y >> TINYMT32_SH0) ^ x;
    random->status[0] = random->status[1];
    random->status[1] = random->status[2];
    random->status[2] = x ^ (y << TINYMT32_SH1);
    random->status[3] = y;
    int32_t const a = -((int32_t)(y & 1)) & (int32_t)random->mat1;
    int32_t const b = -((int32_t)(y & 1)) & (int32_t)random->mat2;
    random->status[1] ^= (uint32_t)a;
    random->status[2] ^= (uint32_t)b;
}
inline static uint32_t tinymt32_temper(tinymt32_t * random) {
    uint32_t t0, t1;
    t0 = random->status[3];
    t1 = random->status[0]
        ^ (random->status[2] >> TINYMT32_SH8);
    t0 ^= t1;
    if ((t1 & 1) != 0) {
        t0 ^= random->tmat;
    }
    return t0;
}
void tinymt32_init(tinymt32_t * random, uint32_t seed){
    random->status[0] = seed;
    random->status[1] = random->mat1;
    random->status[2] = random->mat2;
    random->status[3] = random->tmat;
    for (unsigned int i = 1; i < MIN_LOOP; i++) {
        random->status[i & 3] ^= i + UINT32_C(1812433253)
            * (random->status[(i - 1) & 3]
               ^ (random->status[(i - 1) & 3] >> 30));
    }
    for (unsigned int i = 0; i < PRE_LOOP; i++) {
        tinymt32_next_state(random);
    }
}
inline static uint32_t tinymt32_generate_uint32(tinymt32_t * random) {
    tinymt32_next_state(random);
    return tinymt32_temper(random);
}