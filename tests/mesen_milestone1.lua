-- Mesen 2 runtime validation for the Milestone 1 ROM.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_milestone1.lua

local endFrames = 0
local nmis = 0
local controllerWrites = 0
local failures = {}
local firstFrameCounter = nil

local function check(condition, message)
    if not condition then
        table.insert(failures, message)
    end
end

emu.addEventCallback(function()
    nmis = nmis + 1
end, emu.eventType.nmi)

emu.addMemoryCallback(function()
    controllerWrites = controllerWrites + 1
end, emu.callbackType.write, 0x4016, 0x4016)

emu.addEventCallback(function()
    endFrames = endFrames + 1

    if endFrames == 10 then
        firstFrameCounter = emu.read(0x0002, emu.memType.nesDebug)
    elseif endFrames == 130 then
        local finalFrameCounter = emu.read(0x0002, emu.memType.nesDebug)
        local counterDelta = (finalFrameCounter - firstFrameCounter) & 0xFF

        check(nmis >= 120, "NMI did not execute on every observed frame")
        check(counterDelta >= 118 and counterDelta <= 122,
            "zero-page frame counter did not advance once per frame")
        check(controllerWrites >= (nmis - 3) * 2 and controllerWrites <= nmis * 2,
            "controller polling was not synchronized to one update per frame")

        for address = 0, 255 do
            check(emu.read(address, emu.memType.nesSpriteRam) == 0xFF,
                string.format("visible or corrupt OAM byte at $%02X", address))
        end

        for address = 0, 31 do
            check(emu.read(address, emu.memType.nesPaletteRam) == 0x0F,
                string.format("palette byte at $%02X is not NES black", address))
        end

        local screen = emu.getScreenBuffer()
        check(#screen > 0, "Mesen returned an empty screen buffer")
        if #screen > 0 then
            local background = screen[1]
            for index = 2, #screen do
                if screen[index] ~= background then
                    check(false, "screen buffer is not uniformly black")
                    break
                end
            end
        end

        if #failures == 0 then
            emu.log(string.format(
                "Milestone 1 runtime validation passed: %d frames, %d NMIs, %d controller writes",
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
