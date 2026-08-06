#include <stdint.h>

#include "input.h"
#include "rng.h"
#include "tuning.h"

#if MAX_EQUIPPED_WEAPONS != 4U
#error "unexpected initial weapon capacity"
#endif

#if LEVEL_UP_CHOICE_COUNT != 3U
#error "unexpected initial choice count"
#endif

#if UINT8_MAX != 255U || UINT16_MAX != 65535U
#error "unexpected fixed-width integer representation"
#endif

static uint8_t failures;

#define CHECK(condition)       \
    do {                       \
        if (!(condition)) {    \
            ++failures;        \
        }                      \
    } while (0)

/* Test-only entry point compiled by input.c when UNIT_TEST is defined. */
void input_test_apply(uint8_t sample);

/* Satisfies input.c without NES hardware in the sim6502 test target. */
uint8_t nes_read_controller(void)
{
    return 0U;
}

static void test_rng(void)
{
    rng_seed(UINT16_C(0xACE1));
    CHECK(rng_next_u16() == UINT16_C(0xD30F));
    CHECK(rng_next_u16() == UINT16_C(0xF1A5));
    CHECK(rng_next_u8() == UINT8_C(0x34));

    rng_seed(UINT16_C(0xACE1));
    CHECK(rng_next_u16() == UINT16_C(0xD30F));

    rng_seed(UINT16_C(0));
    CHECK(rng_next_u16() == UINT16_C(0x8181));
}

static void test_input_edges(void)
{
    input_test_apply(0U);
    input_test_apply((uint8_t)(BUTTON_A | BUTTON_RIGHT));
    CHECK(input_current() == (uint8_t)(BUTTON_A | BUTTON_RIGHT));
    CHECK(input_pressed() == (uint8_t)(BUTTON_A | BUTTON_RIGHT));
    CHECK(input_released() == 0U);
    CHECK(input_down(BUTTON_A) == 1U);
    CHECK(input_down(BUTTON_LEFT) == 0U);

    input_test_apply((uint8_t)(BUTTON_A | BUTTON_LEFT));
    CHECK(input_pressed() == BUTTON_LEFT);
    CHECK(input_released() == BUTTON_RIGHT);

    input_test_apply(0U);
    CHECK(input_pressed() == 0U);
    CHECK(input_released() == (uint8_t)(BUTTON_A | BUTTON_LEFT));
}

int main(void)
{
    test_rng();
    test_input_edges();
    return failures;
}
