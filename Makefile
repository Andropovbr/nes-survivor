PROJECT := nes-survivor
BUILD_DIR := build

CC65 ?= cc65
CA65 ?= ca65
LD65 ?= ld65
CL65 ?= cl65
SIM65 ?= sim65
MESEN ?= Mesen
PYTHON ?= python3

CFLAGS := -t nes -Oirs --standard c99 --warnings-as-errors -I include
AFLAGS := -t nes --warnings-as-errors -I include --bin-include-dir assets
LDFLAGS := -C cfg/nrom.cfg --warnings-as-errors

C_MODULES := main game input rng animation metasprite player soldier_animation_data
ASM_MODULES := crt0 nmi nes chr
C_ASM := $(addprefix $(BUILD_DIR)/c_,$(addsuffix .s,$(C_MODULES)))
C_OBJECTS := $(addprefix $(BUILD_DIR)/c_,$(addsuffix .o,$(C_MODULES)))
ASM_OBJECTS := $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(ASM_MODULES)))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

ROM := $(BUILD_DIR)/$(PROJECT).nes
MAP := $(BUILD_DIR)/$(PROJECT).map
LABELS := $(BUILD_DIR)/$(PROJECT).lbl
TEST_BIN := $(BUILD_DIR)/test_logic

.PHONY: all clean test test-runtime

all: $(ROM)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/c_%.s: src/%.c | $(BUILD_DIR)
	$(CC65) $(CFLAGS) --add-source -o $@ $<

$(BUILD_DIR)/c_%.o: $(BUILD_DIR)/c_%.s
	$(CA65) $(AFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: src/%.s | $(BUILD_DIR)
	$(CA65) $(AFLAGS) -o $@ $<

$(BUILD_DIR)/chr.o: assets/game.chr

$(ROM): $(OBJECTS) cfg/nrom.cfg
	$(LD65) $(LDFLAGS) -m $(MAP) -Ln $(LABELS) -o $@ $(OBJECTS) nes.lib

TEST_SOURCES := tests/test_logic.c src/input.c src/rng.c src/animation.c \
	src/metasprite.c src/player.c src/soldier_animation_data.c

$(TEST_BIN): $(TEST_SOURCES) include/input.h include/rng.h include/nes.h \
	include/tuning.h include/animation.h include/metasprite.h include/player.h \
	include/soldier_animation_data.h | $(BUILD_DIR)
	$(CL65) -t sim6502 --standard c99 --warnings-as-errors -DUNIT_TEST -I include -o $@ $(TEST_SOURCES)

test: $(ROM) $(TEST_BIN)
	$(SIM65) $(TEST_BIN)
	$(PYTHON) tests/validate_rom.py $(ROM) $(MAP) $(LABELS)

test-runtime: $(ROM)
	$(MESEN) --testrunner $(ROM) tests/mesen_milestone1.lua

clean:
	rm -rf $(BUILD_DIR)
