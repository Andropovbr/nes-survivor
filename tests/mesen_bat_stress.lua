-- Mesen 2 performance regression test with four or more simultaneous Bats.
-- The performance build overrides initial HP to 255 so contact damage cannot
-- end this long-running load scenario; release/default builds still use 5 HP.
-- Run through make test-performance or tools/build.ps1 performance so the HP
-- override is applied safely.

local endFrames = 0
local nmis = 0
local controllerWrites = 0
local maxBatCount = 0
local maxGemCount = 0
local previousSkipped = 0
local gameplayBaselineSkipped = nil
local gameplayStartFrame = nil

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

local function gemCount()
    local count = 0
    for sprite = 0, 63 do
        local offset = sprite * 4
        if oam(offset) ~= 0xFF and oam(offset + 1) == 0x14 then
            count = count + 1
            if oam(offset + 2) ~= 0x03 then
                emu.log("FAIL: XP gem did not use sprite palette 3")
                emu.stop(1)
            end
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
    -- The legacy test began gameplay about four video frames after reset.
    -- Preserve that phase while anchoring it to the detected run start.
    local phase = gameplayStartFrame == nil and 0 or
        (endFrames - gameplayStartFrame + 4) % 400

    if (endFrames >= 10 and endFrames <= 12) or endFrames == 20 then
        input.start = true
    elseif phase < 100 then
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
    local currentGemCount

    endFrames = endFrames + 1
    currentBatCount = batCount()
    currentGemCount = gemCount()
    if currentGemCount > maxGemCount then
        maxGemCount = currentGemCount
    end
    if gameplayStartFrame == nil and oam(0) == 107 and oam(1) == 0x00 then
        gameplayStartFrame = endFrames
        print(string.format("stress gameplay start: frame=%d", gameplayStartFrame))
    end
    if currentBatCount > maxBatCount then
        maxBatCount = currentBatCount
        print(string.format(
            "stress spawn: frame=%d bats=%d nmis=%d updates=%d",
            endFrames, currentBatCount, nmis, math.floor(controllerWrites / 2)))
    end
    local updates = math.floor(controllerWrites / 2)
    local skipped = nmis - updates
    if gameplayStartFrame ~= nil and
       endFrames == gameplayStartFrame + 5 then
        gameplayBaselineSkipped = skipped
        previousSkipped = skipped
    elseif gameplayBaselineSkipped ~= nil and skipped > previousSkipped then
        local swordVisible = oam(28) ~= 0xFF and oam(29) == 0x08
        print(string.format(
            "stress skip: frame=%d bats=%d sword=%s nmis=%d updates=%d",
            endFrames, currentBatCount, tostring(swordVisible), nmis, updates))
    end
    previousSkipped = skipped

    if endFrames == 1750 then
        local gameplaySkipped = skipped - (gameplayBaselineSkipped or skipped)
        print(string.format(
            "stress result: frames=%d bats_max=%d gems_max=%d nmis=%d updates=%d gameplay_skipped=%d",
            endFrames, maxBatCount, maxGemCount, nmis, updates, gameplaySkipped))

        if maxBatCount < 12 then
            emu.log("FAIL: stress test never saturated the 12-Bat pool")
            emu.stop(1)
        elseif maxGemCount < 1 then
            emu.log("FAIL: stress test never observed an XP gem drop")
            emu.stop(1)
        elseif gameplaySkipped ~= 0 then
            emu.log("FAIL: gameplay updates were skipped under Bat load")
            emu.stop(1)
        else
            emu.log("Bat stress validation passed without skipped updates")
            emu.stop(0)
        end
    end
end, emu.eventType.endFrame)
