-- Mesen 2 performance regression test with four or more simultaneous Bats.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_bat_stress.lua

local endFrames = 0
local nmis = 0
local controllerWrites = 0
local maxBatCount = 0
local previousSkipped = 0

local function oam(address)
    return emu.read(address, emu.memType.nesSpriteRam)
end

local function batCount()
    local count = 0
    for sprite = 0, 63 do
        local offset = sprite * 4
        local tile = oam(offset + 1)
        if oam(offset) ~= 0xFF and (tile == 0x0A or tile == 0x0C) then
            count = count + 1
        end
    end
    return count
end

emu.addEventCallback(function()
    nmis = nmis + 1
end, emu.eventType.nmi)

emu.addMemoryCallback(function()
    controllerWrites = controllerWrites + 1
end, emu.callbackType.write, 0x4016, 0x4016)

-- Keep Soldier moving in a 100x100 interior square. Soldier is faster than Bats,
-- so this preserves enemies long enough to measure simultaneous update cost.
emu.addEventCallback(function()
    local input = {
        a = false, b = false, select = false, start = false,
        up = false, down = false, left = false, right = false,
    }
    local phase = endFrames % 400

    if phase < 100 then
        input.right = true
    elseif phase < 200 then
        input.down = true
    elseif phase < 300 then
        input.left = true
    else
        input.up = true
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    local currentBatCount

    endFrames = endFrames + 1
    currentBatCount = batCount()
    if currentBatCount > maxBatCount then
        maxBatCount = currentBatCount
        print(string.format(
            "stress spawn: frame=%d bats=%d nmis=%d updates=%d",
            endFrames, currentBatCount, nmis, math.floor(controllerWrites / 2)))
    end
    local updates = math.floor(controllerWrites / 2)
    local skipped = nmis - updates
    if skipped > previousSkipped then
        local swordVisible = oam(28) ~= 0xFF and oam(29) == 0x08
        print(string.format(
            "stress skip: frame=%d bats=%d sword=%s nmis=%d updates=%d",
            endFrames, currentBatCount, tostring(swordVisible), nmis, updates))
    end
    previousSkipped = skipped

    if endFrames == 1700 then
        print(string.format(
            "stress result: frames=%d bats_max=%d nmis=%d updates=%d skipped=%d",
            endFrames, maxBatCount, nmis, updates, skipped))

        if maxBatCount < 12 then
            emu.log("FAIL: stress test never saturated the 12-Bat pool")
            emu.stop(1)
        elseif skipped ~= 0 then
            emu.log("FAIL: gameplay updates were skipped under Bat load")
            emu.stop(1)
        else
            emu.log("Bat stress validation passed without skipped updates")
            emu.stop(0)
        end
    end
end, emu.eventType.endFrame)
