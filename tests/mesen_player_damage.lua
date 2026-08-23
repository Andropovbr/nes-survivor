-- Mesen 2 runtime validation for contact damage, hit SFX and game over.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_player_damage.lua

local endFrames = 0
local failures = {}
local gameOverFrame = nil
local noiseLengthWrites = 0
local lastApuStatus = nil
local lastNoiseVolume = nil
local lastNoisePeriod = nil
local lastNoiseLength = nil

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

local function allSpritesHidden()
    for sprite = 0, 63 do
        if emu.read(sprite * 4, emu.memType.nesSpriteRam) ~= 0xFF then
            return false
        end
    end
    return true
end

local function tileHasPixels(address)
    for offset = 0, 15 do
        if emu.read(address + offset, emu.memType.nesPpuDebug) ~= 0 then
            return true
        end
    end
    return false
end

emu.addMemoryCallback(function(_, value)
    lastApuStatus = value
end, emu.callbackType.write, 0x4015, 0x4015)

emu.addMemoryCallback(function(_, value)
    lastNoiseVolume = value
end, emu.callbackType.write, 0x400C, 0x400C)

emu.addMemoryCallback(function(_, value)
    lastNoisePeriod = value
end, emu.callbackType.write, 0x400E, 0x400E)

emu.addMemoryCallback(function(_, value)
    lastNoiseLength = value
    noiseLengthWrites = noiseLengthWrites + 1
end, emu.callbackType.write, 0x400F, 0x400F)

emu.addEventCallback(function()
    local input = {
        a = false, b = false, select = false, start = false,
        up = false, down = false, left = false, right = false,
    }

    if (endFrames >= 10 and endFrames <= 12) or endFrames == 20 then
        input.start = true
    elseif gameOverFrame ~= nil and endFrames == gameOverFrame + 10 then
        input.start = true
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    endFrames = endFrames + 1

    if gameOverFrame == nil and readText(14, 11, 9) == "GAME OVER" then
        gameOverFrame = endFrames
        check(tileHasPixels(0x1000 + string.byte("M") * 16),
            "GAME OVER uses a blank M glyph")
        check(noiseLengthWrites == 5,
            "player did not receive exactly five audible contact hits")
        check(lastApuStatus == 0x08, "hit SFX did not enable the noise channel")
        check(lastNoiseVolume == 0x1C, "hit SFX volume register is unexpected")
        check(lastNoisePeriod == 0x04, "hit SFX period register is unexpected")
        check(lastNoiseLength == 0x10, "hit SFX length register is unexpected")
    elseif gameOverFrame ~= nil and endFrames == gameOverFrame + 2 then
        -- The screen transition hides shadow OAM immediately; the next NMI
        -- transfers it to hardware OAM through the normal one-frame pipeline.
        check(allSpritesHidden(), "game over left a hardware sprite visible")
    elseif gameOverFrame ~= nil and endFrames == gameOverFrame + 20 then
        check(readText(12, 10, 12) == "NES SURVIVOR",
            "START on game over did not return to the title screen")
        check(readText(16, 10, 11) == "PRESS START",
            "title prompt is missing after game over")
        check(allSpritesHidden(), "title screen left a hardware sprite visible")

        if #failures == 0 then
            local message = string.format(
                "Player-damage validation passed: game over at frame %d",
                gameOverFrame)
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
    elseif endFrames >= 1400 and gameOverFrame == nil then
        emu.log("FAIL: contact damage did not reach game over")
        emu.stop(1)
    end
end, emu.eventType.endFrame)
