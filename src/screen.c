#include "screen.h"

#include <stdint.h>

#include "nes.h"

#define SCREEN_NAMETABLE_BASE UINT16_C(0x2000)
#define SCREEN_TILE_COLUMNS    32U
#define SCREEN_TILE_COUNT      UINT16_C(1024)
#define SCREEN_BLANK_TILE      UINT8_C(0x00)

#define PRESENTED_BY_ROW       12U
#define PRESENTED_BY_COLUMN    10U
#define CREDIT_ROW             14U
#define CREDIT_COLUMN           7U
#define TITLE_ROW              12U
#define TITLE_COLUMN           10U
#define TITLE_PROMPT_ROW       16U
#define TITLE_PROMPT_COLUMN    10U

#define PPU_REGISTER(address) (*(volatile uint8_t *)(address))

static const uint8_t presented_by_text[] = "Presented by";
static const uint8_t credit_text[] = "Codigo e Cartucho";
static const uint8_t title_text[] = "NES Survivor";
static const uint8_t title_prompt_text[] = "Press Start";

static void ppu_set_address(uint16_t address)
{
    (void)PPU_REGISTER(NES_PPUSTATUS);
    PPU_REGISTER(NES_PPUADDR) = (uint8_t)(address >> 8U);
    PPU_REGISTER(NES_PPUADDR) = (uint8_t)address;
}

static void screen_disable_rendering(void)
{
    PPU_REGISTER(NES_PPUCTRL) = 0U;
    PPU_REGISTER(NES_PPUMASK) = 0U;
}

static void screen_enable_rendering(void)
{
    PPU_REGISTER(NES_PPUSCROLL) = 0U;
    PPU_REGISTER(NES_PPUSCROLL) = 0U;
    PPU_REGISTER(NES_PPUCTRL) = NES_PPUCTRL_GAME;
    PPU_REGISTER(NES_PPUMASK) = NES_PPUMASK_GAME;
}

static void screen_wait_for_vblank(void)
{
    while ((PPU_REGISTER(NES_PPUSTATUS) & NES_PPUSTATUS_VBLANK) == 0U) {
    }
}

static void screen_clear_nametable(void)
{
    uint16_t remaining;

    ppu_set_address(SCREEN_NAMETABLE_BASE);
    for (remaining = SCREEN_TILE_COUNT; remaining != 0U; --remaining) {
        PPU_REGISTER(NES_PPUDATA) = SCREEN_BLANK_TILE;
    }
}

static void screen_load_background_palette(void)
{
    ppu_set_address(UINT16_C(0x3F00));
    PPU_REGISTER(NES_PPUDATA) = UINT8_C(0x0F);
    PPU_REGISTER(NES_PPUDATA) = UINT8_C(0x30);
    PPU_REGISTER(NES_PPUDATA) = UINT8_C(0x10);
    PPU_REGISTER(NES_PPUDATA) = UINT8_C(0x00);
}

static void screen_write_text(uint8_t row, uint8_t column,
                              const uint8_t *text)
{
    uint16_t address = (uint16_t)(SCREEN_NAMETABLE_BASE +
                                  (uint16_t)row * SCREEN_TILE_COLUMNS +
                                  column);

    ppu_set_address(address);
    while (*text != 0U) {
        PPU_REGISTER(NES_PPUDATA) = *text;
        ++text;
    }
}

static void screen_write_blank_text(uint8_t row, uint8_t column,
                                    const uint8_t *text)
{
    uint16_t address = (uint16_t)(SCREEN_NAMETABLE_BASE +
                                  (uint16_t)row * SCREEN_TILE_COLUMNS +
                                  column);

    ppu_set_address(address);
    while (*text != 0U) {
        PPU_REGISTER(NES_PPUDATA) = SCREEN_BLANK_TILE;
        ++text;
    }
}

static void screen_hide_all_sprites(void)
{
    uint8_t sprite;

    for (sprite = 0U; sprite < 64U; ++sprite) {
        oam_shadow[(uint8_t)(sprite * 4U)] = UINT8_C(0xFF);
    }
}

void screen_show_presented_by(void)
{
    screen_disable_rendering();
    screen_hide_all_sprites();
    screen_clear_nametable();
    screen_load_background_palette();
    screen_write_text(PRESENTED_BY_ROW, PRESENTED_BY_COLUMN,
                      presented_by_text);
    screen_write_text(CREDIT_ROW, CREDIT_COLUMN, credit_text);
    screen_wait_for_vblank();
    screen_enable_rendering();
}

void screen_show_title(void)
{
    screen_disable_rendering();
    screen_hide_all_sprites();
    screen_clear_nametable();
    screen_load_background_palette();
    screen_write_text(TITLE_ROW, TITLE_COLUMN, title_text);
    screen_write_text(TITLE_PROMPT_ROW, TITLE_PROMPT_COLUMN,
                      title_prompt_text);
    screen_wait_for_vblank();
    screen_enable_rendering();
}

void screen_set_title_prompt_visible(uint8_t visible)
{
    /* The update is only 11 tiles, but disabling rendering also prevents a
     * PPU address-latch race if a future title update grows toward VBlank. */
    screen_disable_rendering();
    if (visible != 0U) {
        screen_write_text(TITLE_PROMPT_ROW, TITLE_PROMPT_COLUMN,
                          title_prompt_text);
    } else {
        screen_write_blank_text(TITLE_PROMPT_ROW, TITLE_PROMPT_COLUMN,
                                title_prompt_text);
    }
    screen_enable_rendering();
}

void screen_show_gameplay(void)
{
    screen_disable_rendering();
    screen_hide_all_sprites();
    screen_clear_nametable();
    screen_wait_for_vblank();
    screen_enable_rendering();
}
