#include "player.h"

#include "animation.h"
#include "input.h"
#include "soldier_animation_data.h"
#include "tuning.h"

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t hit_cooldown;
    PlayerFacing facing;
    uint8_t moving;
    AnimationPlayer animation;
    uint16_t xp;
    uint16_t next_level_xp;
    uint8_t level;
    uint8_t level_up_pending;
    uint8_t weapons[MAX_EQUIPPED_WEAPONS];
    uint8_t bonuses[MAX_EQUIPPED_WEAPONS];
} PlayerState;

static const uint16_t level_thresholds[LEVEL_UP_MAX_TABLE_LEVEL - 1U] = {
    5U, 12U, 22U, 35U, 52U, 75U, 105U, 145U, 200U
};

static PlayerState player;

#if PLAYER_INITIAL_HP == 0U || PLAYER_INITIAL_HP > UINT8_MAX
#error "Player initial HP must fit in one byte and be nonzero"
#endif

#if PLAYER_HIT_COOLDOWN_FRAMES == 0U || \
    PLAYER_HIT_COOLDOWN_FRAMES > UINT8_MAX
#error "Player hit cooldown must fit in one byte and be nonzero"
#endif

#if PLAYER_HITBOX_WIDTH_PIXELS == 0U || PLAYER_HITBOX_HEIGHT_PIXELS == 0U || \
    PLAYER_HITBOX_RIGHT_X_OFFSET_PIXELS + PLAYER_HITBOX_WIDTH_PIXELS > \
        PLAYER_WIDTH_PIXELS || \
    PLAYER_HITBOX_LEFT_X_OFFSET_PIXELS + PLAYER_HITBOX_WIDTH_PIXELS > \
        PLAYER_WIDTH_PIXELS || \
    PLAYER_HITBOX_Y_OFFSET_PIXELS + PLAYER_HITBOX_HEIGHT_PIXELS > \
        PLAYER_HEIGHT_PIXELS
#error "Player damage hitbox must remain inside the logical metasprite"
#endif

static uint8_t selected_animation(void)
{
    if (player.moving == 0U) {
        return SOLDIER_ANIMATION_IDLE;
    }
    return SOLDIER_ANIMATION_MOVEMENT;
}

void player_init(void)
{
    uint8_t i;

    player.x = PLAYER_INITIAL_X;
    player.y = PLAYER_INITIAL_Y;
    player.hp = PLAYER_INITIAL_HP;
    player.max_hp = PLAYER_INITIAL_HP;
    player.hit_cooldown = 0U;
    player.facing = PLAYER_FACING_RIGHT;
    player.moving = 0U;
    player.level = 1U;
    player.xp = 0U;
    player.next_level_xp = level_thresholds[0];
    player.level_up_pending = 0U;
    player.weapons[0] = 0U;
    for (i = 1U; i < MAX_EQUIPPED_WEAPONS; ++i) {
        player.weapons[i] = WEAPON_SLOT_EMPTY;
    }
    for (i = 0U; i < MAX_EQUIPPED_WEAPONS; ++i) {
        player.bonuses[i] = BONUS_SLOT_EMPTY;
    }
    animation_player_init(&player.animation, &soldier_animation_data,
                          SOLDIER_ANIMATION_IDLE);
}

uint8_t player_update(uint8_t buttons)
{
    uint8_t move_left = (uint8_t)((buttons & BUTTON_LEFT) != 0U);
    uint8_t move_right = (uint8_t)((buttons & BUTTON_RIGHT) != 0U);
    uint8_t move_up = (uint8_t)((buttons & BUTTON_UP) != 0U);
    uint8_t move_down = (uint8_t)((buttons & BUTTON_DOWN) != 0U);
    uint8_t changed_animation;

    if (player.hit_cooldown != 0U) {
        --player.hit_cooldown;
    }

    if (move_left != 0U && move_right == 0U) {
        player.facing = PLAYER_FACING_LEFT;
        if (player.x > PLAYER_MIN_X) {
            if ((uint8_t)(player.x - PLAYER_MIN_X) <= PLAYER_MOVEMENT_SPEED) {
                player.x = PLAYER_MIN_X;
            } else {
                player.x = (uint8_t)(player.x - PLAYER_MOVEMENT_SPEED);
            }
        }
    } else if (move_right != 0U && move_left == 0U) {
        player.facing = PLAYER_FACING_RIGHT;
        if (player.x < PLAYER_MAX_X) {
            if ((uint8_t)(PLAYER_MAX_X - player.x) <= PLAYER_MOVEMENT_SPEED) {
                player.x = PLAYER_MAX_X;
            } else {
                player.x = (uint8_t)(player.x + PLAYER_MOVEMENT_SPEED);
            }
        }
    }

    if (move_up != 0U && move_down == 0U) {
        if (player.y > PLAYER_MIN_Y) {
            if ((uint8_t)(player.y - PLAYER_MIN_Y) <= PLAYER_MOVEMENT_SPEED) {
                player.y = PLAYER_MIN_Y;
            } else {
                player.y = (uint8_t)(player.y - PLAYER_MOVEMENT_SPEED);
            }
        }
    } else if (move_down != 0U && move_up == 0U) {
        if (player.y < PLAYER_MAX_Y) {
            if ((uint8_t)(PLAYER_MAX_Y - player.y) <= PLAYER_MOVEMENT_SPEED) {
                player.y = PLAYER_MAX_Y;
            } else {
                player.y = (uint8_t)(player.y + PLAYER_MOVEMENT_SPEED);
            }
        }
    }

    player.moving = (uint8_t)(((move_left ^ move_right) |
                               (move_up ^ move_down)) != 0U);
    changed_animation = animation_player_select(
        &player.animation, &soldier_animation_data, selected_animation());
    if (changed_animation == 0U) {
        animation_player_update(&player.animation, &soldier_animation_data);
    }
    return (uint8_t)(player.hit_cooldown == 0U);
}

uint8_t player_take_contact_damage(void)
{
    if (player.hp == 0U || player.hit_cooldown != 0U) {
        return 0U;
    }

    --player.hp;
    player.hit_cooldown = PLAYER_HIT_COOLDOWN_FRAMES;
    return 1U;
}

void player_render(OamRenderer *renderer)
{
    const AnimationDefinition *animation = animation_player_definition(
        &player.animation, &soldier_animation_data);
    const AnimationFrame *frame = animation_player_frame(
        &player.animation, &soldier_animation_data);
    int16_t anchor_x = player.x;
    uint8_t horizontal_flip =
        (uint8_t)(player.facing == PLAYER_FACING_LEFT);

    (void)oam_renderer_draw_metasprite(
        renderer, anchor_x, player.y,
        &soldier_animation_data.sprites[frame->sprite_offset],
        frame->sprite_count,
        (uint8_t)(animation->width_tiles * NES_SPRITE_WIDTH_PIXELS),
        horizontal_flip);
}

uint8_t player_x(void) { return player.x; }
uint8_t player_y(void) { return player.y; }
uint8_t player_hitbox_x(void)
{
    return (uint8_t)(player.x +
                     (player.facing == PLAYER_FACING_LEFT
                          ? PLAYER_HITBOX_LEFT_X_OFFSET_PIXELS
                          : PLAYER_HITBOX_RIGHT_X_OFFSET_PIXELS));
}
uint8_t player_hitbox_y(void)
{
    return (uint8_t)(player.y + PLAYER_HITBOX_Y_OFFSET_PIXELS);
}
uint8_t player_hp(void) { return player.hp; }
uint8_t player_hit_cooldown(void) { return player.hit_cooldown; }
PlayerFacing player_facing(void) { return player.facing; }
uint8_t player_is_moving(void) { return player.moving; }
uint8_t player_current_animation(void) { return player.animation.animation; }
uint8_t player_current_frame(void) { return player.animation.frame; }
uint8_t player_frame_timer(void) { return player.animation.frame_timer; }

void player_add_xp(uint16_t amount)
{
    if ((uint16_t)(UINT16_MAX - player.xp) < amount) {
        player.xp = UINT16_MAX;
    } else {
        player.xp = (uint16_t)(player.xp + amount);
    }
    if (player.xp >= player.next_level_xp) {
        player.level_up_pending = 1U;
    }
}

void player_apply_level_up(uint8_t choice_index)
{
    if (player.xp >= player.next_level_xp) {
        player.xp = (uint16_t)(player.xp - player.next_level_xp);
    } else {
        player.xp = 0U;
    }

    if (player.level < UINT8_MAX) {
        ++player.level;
    }

    if (player.level < LEVEL_UP_MAX_TABLE_LEVEL) {
        player.next_level_xp = level_thresholds[(uint8_t)(player.level - 1U)];
    } else if (player.next_level_xp <= (uint16_t)(UINT16_MAX - 60U)) {
        player.next_level_xp = (uint16_t)(player.next_level_xp + 60U);
    } else {
        player.next_level_xp = UINT16_MAX;
    }

    if (player.xp >= player.next_level_xp) {
        player.level_up_pending = 1U;
    } else {
        player.level_up_pending = 0U;
    }

    switch (choice_index) {
    case 0U:
        /* Placeholder: Sword upgrade */
        break;
    case 1U:
        if (player.max_hp < UINT8_MAX) {
            ++player.max_hp;
        }
        if (player.hp < UINT8_MAX) {
            ++player.hp;
        }
        break;
    case 2U:
        /* Placeholder: Speed upgrade */
        break;
    default:
        break;
    }
}

uint16_t player_xp(void) { return player.xp; }
uint16_t player_next_level_xp(void) { return player.next_level_xp; }
uint8_t player_level(void) { return player.level; }
uint8_t player_level_up_pending(void) { return player.level_up_pending; }
uint8_t player_max_hp(void) { return player.max_hp; }

uint8_t player_weapon(uint8_t slot)
{
    if (slot >= MAX_EQUIPPED_WEAPONS) {
        return WEAPON_SLOT_EMPTY;
    }
    return player.weapons[slot];
}

uint8_t player_bonus(uint8_t slot)
{
    if (slot >= MAX_EQUIPPED_WEAPONS) {
        return BONUS_SLOT_EMPTY;
    }
    return player.bonuses[slot];
}
