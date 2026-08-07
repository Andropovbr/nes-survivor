#include "animation.h"

static uint8_t frame_duration(const AnimationPlayer *player,
                              const AnimationData *data)
{
    const AnimationDefinition *animation = &data->animations[player->animation];
    const AnimationFrame *frame =
        &data->frames[animation->frame_offset + player->frame];

    /* A zero duration from malformed generated data still advances safely. */
    return frame->duration != 0U ? frame->duration : 1U;
}

void animation_player_init(AnimationPlayer *player,
                           const AnimationData *data,
                           uint8_t animation)
{
    player->animation = animation < data->animation_count ? animation : 0U;
    player->frame = 0U;
    player->frame_timer = frame_duration(player, data);
}

uint8_t animation_player_select(AnimationPlayer *player,
                                const AnimationData *data,
                                uint8_t animation)
{
    if (animation >= data->animation_count) {
        animation = 0U;
    }
    if (player->animation == animation) {
        return 0U;
    }

    player->animation = animation;
    player->frame = 0U;
    player->frame_timer = frame_duration(player, data);
    return 1U;
}

void animation_player_update(AnimationPlayer *player,
                             const AnimationData *data)
{
    const AnimationDefinition *animation;

    if (player->frame_timer > 1U) {
        --player->frame_timer;
        return;
    }

    animation = &data->animations[player->animation];
    ++player->frame;
    if (player->frame >= animation->frame_count) {
        player->frame = 0U;
    }
    player->frame_timer = frame_duration(player, data);
}

const AnimationDefinition *animation_player_definition(
    const AnimationPlayer *player, const AnimationData *data)
{
    return &data->animations[player->animation];
}

const AnimationFrame *animation_player_frame(const AnimationPlayer *player,
                                             const AnimationData *data)
{
    const AnimationDefinition *animation =
        animation_player_definition(player, data);
    return &data->frames[animation->frame_offset + player->frame];
}
