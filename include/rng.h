#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void rng_seed(uint16_t seed);
uint8_t rng_next_u8(void);
uint16_t rng_next_u16(void);

#endif
