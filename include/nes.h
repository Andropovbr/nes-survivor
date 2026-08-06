#ifndef NES_H
#define NES_H

#include <stdint.h>

#define NES_PPUCTRL  UINT16_C(0x2000)
#define NES_PPUMASK  UINT16_C(0x2001)
#define NES_PPUSTATUS UINT16_C(0x2002)
#define NES_OAMADDR  UINT16_C(0x2003)
#define NES_OAMDATA  UINT16_C(0x2004)
#define NES_PPUSCROLL UINT16_C(0x2005)
#define NES_PPUADDR  UINT16_C(0x2006)
#define NES_PPUDATA  UINT16_C(0x2007)
#define NES_OAMDMA   UINT16_C(0x4014)
#define NES_JOYPAD1  UINT16_C(0x4016)

extern uint8_t oam_shadow[256];
extern volatile uint8_t nes_frame_counter;

/*
 * Assembly ABI: nes_wait_frame
 * Waits until NMI advances the 8-bit frame counter. No parameters or return.
 * Clobbers A and processor flags; uses no zero-page temporary storage.
 * Must be called with NMI enabled; it is not interrupt-safe or reentrant.
 */
void nes_wait_frame(void);

/*
 * Assembly ABI: nes_read_controller
 * Strobes and reads controller port 1. No parameters; returns the button mask
 * in A. Clobbers A, X, processor flags and one private zero-page byte. Y is
 * preserved. Called only from the main loop; DMC DMA must remain disabled.
 */
uint8_t nes_read_controller(void);

#endif
