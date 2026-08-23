-- Mesen 2 runtime validation for initial screens and START edge behavior.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_game_states.lua

local endFrames = 0
local failures = {}
local sawHiddenPrompt = false
local sawVisiblePromptAfterHidden = false

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
    elseif endFrames == 100 then
        input.start = true
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    endFrames = endFrames + 1

    if endFrames == 12 then
        check(readText(12, 10, 12) == "Presented by",
            "Presented by text is missing or misplaced")
        check(readText(14, 7, 17) == "Codigo e Cartucho",
            "credit text is missing or misplaced")
        check(allSpritesHidden(), "intro left a hardware sprite visible")
    elseif endFrames == 30 then
        check(readText(12, 10, 12) == "NES Survivor",
            "held START did not enter or remain on the title screen")
        check(readText(16, 10, 11) == "Press Start",
            "title prompt is missing")
        check(allSpritesHidden(), "title left a hardware sprite visible")
    elseif endFrames > 30 and endFrames < 95 then
        if promptIsBlank() then
            sawHiddenPrompt = true
        elseif sawHiddenPrompt and
               readText(16, 10, 11) == "Press Start" then
            sawVisiblePromptAfterHidden = true
        end
    elseif endFrames == 95 then
        check(sawHiddenPrompt, "Press Start did not enter its hidden phase")
        check(sawVisiblePromptAfterHidden,
            "Press Start did not return to its visible phase")
    elseif endFrames == 110 then
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
