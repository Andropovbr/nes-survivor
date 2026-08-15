#include "weapon_sword.h"

#include "tuning.h"

typedef struct {
    uint8_t active_frames;
    uint8_t frames_until_attack;
} SwordRuntime;

/* Generated sword frame: signed x/y, CHR tile index and OAM attributes. */
static const MetaspriteTile sword_attack_tiles[] = {
    { 0, 0, 0x08, 0x00 },
    { 0, 8, 0x09, 0x00 },
};

static SwordRuntime sword;

void weapon_sword_init(void)
{
    sword.active_frames = 0U;
    sword.frames_until_attack = 0U;
}

void weapon_sword_update(void)
{
    if (sword.active_frames != 0U) {
        --sword.active_frames;
    }

    if (sword.frames_until_attack != 0U) {
        --sword.frames_until_attack;
        return;
    }

    sword.active_frames = SWORD_ATTACK_ACTIVE_FRAMES;
    /* The current frame is the first frame in the complete attack period. */
    sword.frames_until_attack = (uint8_t)(SWORD_ATTACK_COOLDOWN_FRAMES - 1U);
}

uint8_t weapon_sword_render(OamRenderer *renderer,
                            uint8_t player_x,
                            uint8_t player_y,
                            uint8_t facing_left)
{
    WeaponSwordHitbox hitbox;

    if (weapon_sword_hitbox(&hitbox, player_x, player_y, facing_left) == 0U) {
        return 0U;
    }

    return oam_renderer_draw_metasprite(
        renderer, hitbox.x, hitbox.y, sword_attack_tiles,
        SWORD_ATTACK_SPRITE_COUNT, hitbox.width, facing_left);
}

uint8_t weapon_sword_hitbox(WeaponSwordHitbox *hitbox,
                            uint8_t player_x,
                            uint8_t player_y,
                            uint8_t facing_left)
{
    int16_t anchor_x;

    if (sword.active_frames == 0U) {
        return 0U;
    }

    if (facing_left != 0U) {
        if (player_x < SWORD_WIDTH_PIXELS) {
            return 0U;
        }
        anchor_x = (int16_t)(player_x - SWORD_WIDTH_PIXELS);
    } else {
        anchor_x = (int16_t)(player_x + PLAYER_WIDTH_PIXELS);
        if (anchor_x > (int16_t)(256U - SWORD_WIDTH_PIXELS)) {
            return 0U;
        }
    }

    hitbox->x = anchor_x;
    hitbox->y = (int16_t)(player_y + SWORD_VERTICAL_OFFSET_PIXELS);
    hitbox->width = SWORD_WIDTH_PIXELS;
    hitbox->height = (uint8_t)(SWORD_ATTACK_SPRITE_COUNT * 8U);
    return 1U;
}

uint8_t weapon_sword_is_attacking(void)
{
    return (uint8_t)(sword.active_frames != 0U);
}

uint8_t weapon_sword_active_frames(void)
{
    return sword.active_frames;
}

uint8_t weapon_sword_frames_until_attack(void)
{
    return sword.frames_until_attack;
}
