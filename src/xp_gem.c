#include "xp_gem.h"

#include "nes.h"
#include "tuning.h"

#define OAM_BYTES_PER_SPRITE 4U
#define XP_GEM_TILE_INDEX    UINT8_C(0x14)
#define XP_GEM_PALETTE       UINT8_C(0x03)

#if 7U + SWORD_ATTACK_SPRITE_COUNT + (MAX_ACTIVE_ENEMIES * 2U) + \
        MAX_ACTIVE_XP_GEMS > NES_OAM_SPRITE_CAPACITY
#error "Player, sword, enemy and XP gem pools exceed OAM capacity"
#endif

static uint8_t gem_x_positions[MAX_ACTIVE_XP_GEMS];
static uint8_t gem_y_positions[MAX_ACTIVE_XP_GEMS];
static uint8_t gem_active[MAX_ACTIVE_XP_GEMS];
/* Condensed drop units are bookkeeping only; collection grants no XP yet. */
static uint16_t gem_drop_units[MAX_ACTIVE_XP_GEMS];
static uint8_t pool_high_water;
static uint8_t collection_scan_index;

static uint8_t approximate_distance(uint8_t first_x, uint8_t first_y,
                                    uint8_t second_x, uint8_t second_y)
{
    uint8_t delta_x = first_x < second_x
                          ? (uint8_t)(second_x - first_x)
                          : (uint8_t)(first_x - second_x);
    uint8_t delta_y = first_y < second_y
                          ? (uint8_t)(second_y - first_y)
                          : (uint8_t)(first_y - second_y);

    return delta_x > delta_y ? delta_x : delta_y;
}

void xp_gem_init(void)
{
    uint8_t index;

    for (index = 0U; index < MAX_ACTIVE_XP_GEMS; ++index) {
        gem_active[index] = 0U;
        gem_drop_units[index] = 0U;
    }
    pool_high_water = 0U;
    collection_scan_index = 0U;
}

void xp_gem_spawn(uint8_t x, uint8_t y)
{
    uint8_t index;

    for (index = 0U; index < pool_high_water; ++index) {
        if (gem_active[index] == 0U) {
            break;
        }
    }
    if (index < MAX_ACTIVE_XP_GEMS) {
        if (index == pool_high_water) {
            ++pool_high_water;
        }
        gem_x_positions[index] = x;
        gem_y_positions[index] = y;
        gem_drop_units[index] = 1U;
        gem_active[index] = 1U;
        return;
    }

    /* A full pool condenses the drop into the nearest visible gem. This scan
     * runs only on kills, not every frame. uint16_t is ample for one run. */
    {
        uint8_t nearest = 0U;
        uint8_t nearest_distance = approximate_distance(
            x, y, gem_x_positions[0], gem_y_positions[0]);

        for (index = 1U; index < MAX_ACTIVE_XP_GEMS; ++index) {
            uint8_t distance = approximate_distance(
                x, y, gem_x_positions[index], gem_y_positions[index]);
            if (distance < nearest_distance) {
                nearest = index;
                nearest_distance = distance;
            }
        }
        if (gem_drop_units[nearest] != UINT16_MAX) {
            ++gem_drop_units[nearest];
        }
    }
}

void xp_gem_update(uint8_t player_x, uint8_t player_y)
{
    uint8_t index;

    if (pool_high_water == 0U) {
        return;
    }
    if (collection_scan_index >= pool_high_water) {
        collection_scan_index = 0U;
    }
    index = collection_scan_index;
    ++collection_scan_index;

    if (gem_active[index] != 0U) {
        uint8_t gem_x = gem_x_positions[index];
        uint8_t gem_y = gem_y_positions[index];

        if (((uint8_t)(gem_x - player_x) < PLAYER_WIDTH_PIXELS ||
             (uint8_t)(player_x - gem_x) < XP_GEM_WIDTH_PIXELS) &&
            ((uint8_t)(gem_y - player_y) < PLAYER_HEIGHT_PIXELS ||
             (uint8_t)(player_y - gem_y) < XP_GEM_HEIGHT_PIXELS)) {
            gem_active[index] = 0U;
            gem_drop_units[index] = 0U;
        }
    }

    while (pool_high_water != 0U &&
           gem_active[pool_high_water - 1U] == 0U) {
        --pool_high_water;
    }
    if (collection_scan_index >= pool_high_water) {
        collection_scan_index = 0U;
    }
}

void xp_gem_render(OamRenderer *renderer)
{
    uint8_t index;

    for (index = 0U; index < pool_high_water; ++index) {
        if (gem_active[index] != 0U) {
            uint8_t offset;

            if (renderer->next_sprite >= NES_OAM_SPRITE_CAPACITY) {
                return;
            }
            offset = (uint8_t)(renderer->next_sprite * OAM_BYTES_PER_SPRITE);
            oam_shadow[offset] = (uint8_t)(gem_y_positions[index] - 1U);
            oam_shadow[offset + 1U] = XP_GEM_TILE_INDEX;
            oam_shadow[offset + 2U] = XP_GEM_PALETTE;
            oam_shadow[offset + 3U] = gem_x_positions[index];
            ++renderer->next_sprite;
        }
    }
}

#ifdef UNIT_TEST
uint8_t xp_gem_active_count(void)
{
    uint8_t index;
    uint8_t count = 0U;

    for (index = 0U; index < pool_high_water; ++index) {
        count = (uint8_t)(count + (uint8_t)(gem_active[index] != 0U));
    }
    return count;
}

uint8_t xp_gem_is_active(uint8_t index)
{
    return index < MAX_ACTIVE_XP_GEMS ? gem_active[index] : 0U;
}

uint8_t xp_gem_x(uint8_t index)
{
    return index < MAX_ACTIVE_XP_GEMS ? gem_x_positions[index] : 0U;
}

uint8_t xp_gem_y(uint8_t index)
{
    return index < MAX_ACTIVE_XP_GEMS ? gem_y_positions[index] : 0U;
}

uint16_t xp_gem_drop_units(uint8_t index)
{
    return index < MAX_ACTIVE_XP_GEMS ? gem_drop_units[index] : 0U;
}
#endif
