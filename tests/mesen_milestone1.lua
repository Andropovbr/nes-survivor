-- Mesen 2 runtime validation for the first animated player milestone.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_milestone1.lua

local endFrames = 0
local nmis = 0
local controllerWrites = 0
local failures = {}
local samples = {}
local idleTiles = {}
local movementTiles = {}

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
    end
    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    endFrames = endFrames + 1

    if endFrames >= 10 and endFrames < 30 then
        idleTiles[oam(1)] = true
    elseif endFrames >= 30 and endFrames < 50 then
        movementTiles[oam(1)] = true
    end

    if endFrames == 20 then
        sample("idleRight")
    elseif endFrames == 45 then
        sample("moveRight")
    elseif endFrames == 65 then
        sample("moveUpFacingRight")
    elseif endFrames == 85 then
        sample("moveLeft")
    elseif endFrames == 105 then
        sample("moveDownLeft")
    elseif endFrames == 125 then
        sample("idleLeft")
    elseif endFrames == 130 then
        local visibleSprites = 0
        local distinctIdleTiles = 0
        local distinctMovementTiles = 0

        check(nmis >= 120, "NMI did not execute on every observed frame")
        check(controllerWrites >= (nmis - 3) * 2 and controllerWrites <= nmis * 2,
            "controller polling was not synchronized to one update per frame")

        for sprite = 0, 63 do
            if oam(sprite * 4) ~= 0xFF then
                visibleSprites = visibleSprites + 1
            end
        end
        check(visibleSprites == 7, "current metasprite does not contain exactly 7 visible sprites")
        for sprite = 7, 63 do
            check(oam(sprite * 4) == 0xFF,
                string.format("unused OAM sprite %d was not hidden", sprite))
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

        for _ in pairs(idleTiles) do
            distinctIdleTiles = distinctIdleTiles + 1
        end
        for _ in pairs(movementTiles) do
            distinctMovementTiles = distinctMovementTiles + 1
        end
        check(distinctIdleTiles >= 2, "idle animation did not advance through generated frames")
        check(distinctMovementTiles >= 2, "movement animation did not advance through generated frames")

        local expectedSpritePalette = {
            [0x10] = 0x0F, [0x11] = 0x00, [0x12] = 0x10, [0x13] = 0x37,
            [0x14] = 0x0F, [0x15] = 0x06, [0x16] = 0x16, [0x17] = 0x26,
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
            emu.log(string.format(
                "Player runtime validation passed: %d frames, %d NMIs, %d controller writes",
                endFrames, nmis, controllerWrites))
            emu.stop(0)
        else
            for _, failure in ipairs(failures) do
                emu.log("FAIL: " .. failure)
            end
            emu.stop(1)
        end
    end
end, emu.eventType.endFrame)
