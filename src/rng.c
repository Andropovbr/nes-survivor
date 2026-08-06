#include "rng.h"

/* xorshift16: compact and deterministic, but not suitable for cryptography. */
static uint16_t rng_state;

void rng_seed(uint16_t seed)
{
    rng_state = (seed == 0U) ? UINT16_C(1) : seed;
}

uint16_t rng_next_u16(void)
{
    uint16_t value = rng_state;

    value ^= (uint16_t)(value << 7);
    value ^= (uint16_t)(value >> 9);
    value ^= (uint16_t)(value << 8);
    rng_state = value;

    return value;
}

uint8_t rng_next_u8(void)
{
    return (uint8_t)rng_next_u16();
}
