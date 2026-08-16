-- Mesen 2 runtime validation for player, animated sword and Bat spawning.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_player.lua

local endFrames = 0
local nmis = 0
local controllerWrites = 0
local failures = {}
local samples = {}
local movementLegTiles = {}
local swordAttackStarts = {}
local swordAttackLengths = {}
local swordWasVisible = false
local swordVisibleFrames = 0
local sawSwordRight = false
local sawSwordLeft = false
local batAppearances = {}
local previousBatCount = 0
local currentBatCount = 0
local firstBatSample = nil
local laterBatSample = nil

local function check(condition, message)
    if not condition then
        table.insert(failures, message)
    end
end

local function oam(address)
    return emu.read(address, emu.memType.nesSpriteRam)
end

local function sample(name)
    samples[name] = {
        y = oam(0),
        tile = oam(1),
        attributes = oam(2),
        x = oam(3),
        secondAttributes = oam(6),
        secondX = oam(7),
    }
end

emu.addEventCallback(function()
    nmis = nmis + 1
end, emu.eventType.nmi)

emu.addMemoryCallback(function()
    controllerWrites = controllerWrites + 1
end, emu.callbackType.write, 0x4016, 0x4016)

-- Drive controller 1 through idle, right, vertical-up, left, down-left and idle.
-- Explicit false values prevent physical controller state from affecting the test.
emu.addEventCallback(function()
    local input = {
        a = false, b = false, select = false, start = false,
        up = false, down = false, left = false, right = false,
    }

    if endFrames >= 30 and endFrames < 50 then
        input.right = true
    elseif endFrames >= 50 and endFrames < 70 then
        input.up = true
    elseif endFrames >= 70 and endFrames < 90 then
        input.left = true
    elseif endFrames >= 90 and endFrames < 110 then
        input.down = true
        input.left = true
    elseif endFrames >= 130 and endFrames < 190 then
        input.right = true
    elseif endFrames >= 190 and endFrames < 250 then
        input.down = true
    elseif endFrames >= 250 and endFrames < 310 then
        input.left = true
    elseif endFrames >= 310 and endFrames < 370 then
        input.up = true
    elseif endFrames >= 370 and endFrames < 430 then
        input.right = true
    elseif endFrames >= 430 then
        input.down = true
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    endFrames = endFrames + 1

    local swordVisible = oam(28) ~= 0xFF and oam(29) == 0x08 and
        oam(32) ~= 0xFF and oam(33) == 0x09
    if swordVisible then
        local playerFacesLeft = (oam(2) & 0x40) ~= 0
        swordVisibleFrames = swordVisibleFrames + 1
        if not swordWasVisible then
            table.insert(swordAttackStarts, endFrames)
        end
        if playerFacesLeft then
            sawSwordLeft = true
            check(oam(30) == 0x40 and oam(31) == oam(3) - 16,
                "left-facing sword was not placed in front of the player")
        else
            sawSwordRight = true
            check(oam(30) == 0x00 and oam(31) == oam(3) + 16,
                "right-facing sword was not placed in front of the player")
        end
    elseif swordWasVisible then
        table.insert(swordAttackLengths, swordVisibleFrames)
        swordVisibleFrames = 0
    end
    swordWasVisible = swordVisible

    currentBatCount = 0
    local firstBat = nil
    for sprite = 0, 63 do
        local offset = sprite * 4
        local tile = oam(offset + 1)
        if oam(offset) ~= 0xFF and (tile == 0x0A or tile == 0x0C) then
            currentBatCount = currentBatCount + 1
            if firstBat == nil then
                firstBat = { x = oam(offset + 3), y = oam(offset), tile = tile }
            end
        end
    end
    if currentBatCount > previousBatCount then
        table.insert(batAppearances, endFrames)
    end
    previousBatCount = currentBatCount
    if firstBatSample == nil and firstBat ~= nil then
        firstBatSample = firstBat
    elseif firstBatSample ~= nil and laterBatSample == nil and
           endFrames >= batAppearances[1] + 20 and firstBat ~= nil then
        laterBatSample = firstBat
    end

    if endFrames >= 30 and endFrames < 50 then
        -- The simplified idle animation has one frame. Tile 5 is a leg tile
        -- that differs between the two walking frames; the head tile does not.
        movementLegTiles[oam(21)] = true
    end

    if endFrames == 20 then
        sample("idleRight")
    elseif endFrames == 51 then
        sample("moveRight")
    elseif endFrames == 71 then
        sample("moveUpFacingRight")
    elseif endFrames == 91 then
        sample("moveLeft")
    elseif endFrames == 111 then
        sample("moveDownLeft")
    elseif endFrames == 125 then
        sample("idleLeft")
    elseif endFrames == 450 then
        local visibleSprites = 0
        local distinctMovementLegTiles = 0

        print(string.format(
            "samples: idleRight=(%d,%d) moveRight=(%d,%d) upRight=(%d,%d) " ..
            "moveLeft=(%d,%d) downLeft=(%d,%d) idleLeft=(%d,%d)",
            samples.idleRight.x, samples.idleRight.y,
            samples.moveRight.x, samples.moveRight.y,
            samples.moveUpFacingRight.x, samples.moveUpFacingRight.y,
            samples.moveLeft.x, samples.moveLeft.y,
            samples.moveDownLeft.x, samples.moveDownLeft.y,
            samples.idleLeft.x, samples.idleLeft.y))

        check(nmis >= 440, "NMI did not execute on every observed frame")
        check(controllerWrites >= (nmis - 3) * 2 and controllerWrites <= nmis * 2,
            "controller polling was not synchronized to one update per frame")

        for sprite = 0, 63 do
            if oam(sprite * 4) ~= 0xFF then
                visibleSprites = visibleSprites + 1
            end
        end
        check(visibleSprites == 7 + (swordVisible and 2 or 0) +
                                   currentBatCount * 2,
            "current OAM does not match player, animated sword and Bat count")
        for sprite = visibleSprites, 63 do
            check(oam(sprite * 4) == 0xFF,
                string.format("unused OAM sprite %d was not hidden", sprite))
        end

        check(#swordAttackStarts >= 3,
            "automatic sword did not start at least three attacks")
        for attack = 2, #swordAttackStarts do
            check(swordAttackStarts[attack] - swordAttackStarts[attack - 1] == 60,
                "automatic sword attack period was not exactly 60 frames")
        end
        for _, length in ipairs(swordAttackLengths) do
            check(length == 12, "sword attack was not visible for 12 frames")
        end
        check(sawSwordRight, "sword was never rendered facing right")
        check(sawSwordLeft, "sword was never rendered facing left")
        check(#batAppearances >= 2,
            "two Bat spawn events were not observed")
        if #batAppearances >= 2 then
            check(batAppearances[1] >= 120 and batAppearances[1] <= 125,
                "first Bat did not appear after the two-second delay")
            check(batAppearances[2] - batAppearances[1] == 120,
                "Bat spawn interval was not exactly two seconds")
        end
        check(firstBatSample ~= nil and laterBatSample ~= nil,
            "first Bat could not be sampled while pursuing the player")
        if firstBatSample ~= nil and laterBatSample ~= nil then
            check(firstBatSample.x ~= laterBatSample.x or
                  firstBatSample.y ~= laterBatSample.y,
                "first Bat did not move toward the player")
            check((firstBatSample.tile == 0x0A or firstBatSample.tile == 0x0C) and
                  (laterBatSample.tile == 0x0A or laterBatSample.tile == 0x0C),
                "Bat did not animate with attached CHR tiles")
        end

        check(samples.idleRight.x == 124 and samples.idleRight.y == 107,
            "player did not begin centered at the expected logical anchor")
        check(samples.moveRight.x > samples.idleRight.x,
            "holding Right did not move the player right")
        check(samples.moveRight.attributes == 0x00 and
              samples.moveRight.secondAttributes == 0x40 and
              samples.moveRight.secondX > samples.moveRight.x,
            "movement-right frame did not preserve generated attributes/geometry")
        check(samples.moveUpFacingRight.x == samples.moveRight.x and
              samples.moveUpFacingRight.y < samples.moveRight.y,
            "vertical-only Up movement changed X or failed to move upward")
        check(samples.moveUpFacingRight.attributes == 0x00,
            "vertical-only Up movement did not preserve right facing")
        check(samples.moveLeft.x < samples.moveUpFacingRight.x and
              samples.moveLeft.attributes == 0x40 and
              samples.moveLeft.secondX < samples.moveLeft.x,
            "holding Left did not move left with generated left-facing data")
        check(samples.moveDownLeft.x < samples.moveLeft.x and
              samples.moveDownLeft.y > samples.moveLeft.y and
              samples.moveDownLeft.attributes == 0x40,
            "Down+Left did not move diagonally while facing left")
        check(samples.idleLeft.x == samples.moveDownLeft.x and
              samples.idleLeft.y == samples.moveDownLeft.y and
              samples.idleLeft.attributes == 0x40 and
              samples.idleLeft.secondX < samples.idleLeft.x,
            "released input did not select correctly mirrored idle-left")

        for _ in pairs(movementLegTiles) do
            distinctMovementLegTiles = distinctMovementLegTiles + 1
        end
        check(distinctMovementLegTiles >= 2,
            "walking animation did not advance through both generated frames")

        local expectedSpritePalette = {
            [0x10] = 0x0F, [0x11] = 0x00, [0x12] = 0x10, [0x13] = 0x37,
            [0x14] = 0x0F, [0x15] = 0x08, [0x16] = 0x19, [0x17] = 0x06,
            [0x18] = 0x0F, [0x19] = 0x09, [0x1A] = 0x19, [0x1B] = 0x29,
            [0x1C] = 0x0F, [0x1D] = 0x03, [0x1E] = 0x13, [0x1F] = 0x23,
        }
        for address, expected in pairs(expectedSpritePalette) do
            check(emu.read(address, emu.memType.nesPaletteRam) == expected,
                string.format("sprite palette mismatch at $%02X", address))
        end

        local screen = emu.getScreenBuffer()
        local nonBackgroundPixel = false
        check(#screen > 0, "Mesen returned an empty screen buffer")
        if #screen > 0 then
            local background = screen[1]
            for index = 2, #screen do
                if screen[index] ~= background then
                    nonBackgroundPixel = true
                    break
                end
            end
        end
        check(nonBackgroundPixel, "player produced no visible pixels")

        if #failures == 0 then
            local message = string.format(
                "Player/sword/Bat validation passed: %d frames, %d NMIs, %d controller writes",
                endFrames, nmis, controllerWrites)
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
