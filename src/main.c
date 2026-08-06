#include "game.h"
#include "input.h"
#include "nes.h"
#include "rng.h"
#include "tuning.h"

int main(void)
{
    rng_seed(INITIAL_RNG_SEED);
    game_init();

    for (;;) {
        nes_wait_frame();
        input_update();
        game_update();
    }

}
