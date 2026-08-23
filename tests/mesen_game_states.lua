-- Mesen 2 runtime validation for initial screens and START edge behavior.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_game_states.lua

local endFrames = 0
local failures = {}
local sawHiddenPrompt = false
local sawVisiblePromptAfterHidden = false
local sawPartialPrompt = false
local promptTransitions = 0
local previousPromptState = nil
local ppuControlWritesDuringBlink = 0
local ppuMaskWritesDuringBlink = 0
local lastPpuCtrl = nil

local function check(condition, message)
    if not condition then
        table.insert(failures, message)
    end
end

local function readText(row, column, length)
    local text = ""
    local address = 0x2000 + row * 32 + column
    for offset = 0, length - 1 do
        text = text .. string.char(
            emu.read(address + offset, emu.memType.nesPpuDebug))
    end
    return text
end

local function promptIsBlank()
    local address = 0x2000 + 16 * 32 + 10
    for offset = 0, 10 do
        if emu.read(address + offset, emu.memType.nesPpuDebug) ~= 0 then
            return false
        end
    end
    return true
end

local function promptState()
    if promptIsBlank() then
        return "hidden"
    elseif readText(16, 10, 11) == "PRESS START" then
        return "visible"
    end
    return "partial"
end

local function tileHasPixels(address)
    for offset = 0, 15 do
        if emu.read(address + offset, emu.memType.nesPpuDebug) ~= 0 then
            return true
        end
    end
    return false
end

local function allSpritesHidden()
    for sprite = 0, 63 do
        if emu.read(sprite * 4, emu.memType.nesSpriteRam) ~= 0xFF then
            return false
        end
    end
    return true
end

emu.addEventCallback(function()
    local input = {
        a = false, b = false, select = false, start = false,
        up = false, down = false, left = false, right = false,
    }

    -- Hold START across several title frames, then release before a new press.
    if endFrames >= 20 and endFrames <= 24 then
        input.start = true
    elseif endFrames == 160 then
        input.start = true
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addMemoryCallback(function(_, value)
    lastPpuCtrl = value
    if endFrames >= 30 and endFrames < 155 then
        ppuControlWritesDuringBlink = ppuControlWritesDuringBlink + 1
    end
end, emu.callbackType.write, 0x2000, 0x2000)

emu.addMemoryCallback(function()
    if endFrames >= 30 and endFrames < 155 then
        ppuMaskWritesDuringBlink = ppuMaskWritesDuringBlink + 1
    end
end, emu.callbackType.write, 0x2001, 0x2001)

emu.addEventCallback(function()
    endFrames = endFrames + 1

    if endFrames == 12 then
        check(readText(12, 10, 12) == "PRESENTED BY",
            "PRESENTED BY text is missing or misplaced")
        check(readText(14, 7, 17) == "CODIGO E CARTUCHO",
            "credit text is missing or misplaced")
        check(allSpritesHidden(), "intro left a hardware sprite visible")
    elseif endFrames == 30 then
        check(readText(12, 10, 12) == "NES SURVIVOR",
            "held START did not enter or remain on the title screen")
        check(readText(16, 10, 11) == "PRESS START",
            "title prompt is missing")
        check(allSpritesHidden(), "title left a hardware sprite visible")
        check(lastPpuCtrl == 0x90,
            "PPUCTRL is not $90 after entering the title screen")
        check((lastPpuCtrl & 0x10) ~= 0,
            "background pattern table 1 is not selected")
        check((lastPpuCtrl & 0x08) == 0,
            "sprite pattern table 0 is not selected")
        check(tileHasPixels(0x1000 + string.byte("P") * 16),
            "font glyph P is missing from background pattern table 1")
        previousPromptState = "visible"
    elseif endFrames > 30 and endFrames < 155 then
        local currentPromptState = promptState()
        if currentPromptState == "partial" then
            sawPartialPrompt = true
        elseif currentPromptState == "hidden" then
            sawHiddenPrompt = true
        elseif sawHiddenPrompt and
               currentPromptState == "visible" then
            sawVisiblePromptAfterHidden = true
        end
        if currentPromptState ~= "partial" and
           currentPromptState ~= previousPromptState then
            promptTransitions = promptTransitions + 1
            previousPromptState = currentPromptState
        end
    elseif endFrames == 155 then
        check(sawHiddenPrompt, "Press Start did not enter its hidden phase")
        check(sawVisiblePromptAfterHidden,
            "Press Start did not return to its visible phase")
        check(not sawPartialPrompt,
            "Press Start was partially updated during a blink")
        check(promptTransitions >= 3,
            "Press Start did not complete several blink phases")
        check(ppuControlWritesDuringBlink == 0,
            "blink wrote PPUCTRL instead of using the NMI VRAM update")
        check(ppuMaskWritesDuringBlink == 0,
            "blink wrote PPUMASK instead of using the NMI VRAM update")
    elseif endFrames == 175 then
        check(readText(12, 10, 12) == string.rep(string.char(0), 12),
            "title nametable was not cleared on gameplay entry")
        check(not allSpritesHidden(),
            "new START press did not initialize and render gameplay")

        if #failures == 0 then
            local message = string.format(
                "Game-state validation passed: %d frames", endFrames)
            print(message)
            emu.log(message)
            emu.stop(0)
        else
            for _, failure in ipairs(failures) do
                print("FAIL: " .. failure)
                emu.log("FAIL: " .. failure)
            end
            emu.stop(1)
        end
    end
end, emu.eventType.endFrame)
