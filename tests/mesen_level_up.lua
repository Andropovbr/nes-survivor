-- Mesen 2 runtime validation for Full-Background HUD and Level-Up system.
-- Run with: Mesen --testrunner build/nes-survivor.nes tests/mesen_level_up.lua

local failures = {}
local endFrames = 0
local testPhase = "START_GAME"
local phaseTimer = 0

local function check(condition, message)
    if not condition then
        table.insert(failures, message)
    end
end

local function readText(row, column, length)
    local text = ""
    local address = 0x2000 + row * 32 + column
    for offset = 0, length - 1 do
        local b = emu.read(address + offset, emu.memType.nesPpuDebug)
        if b >= 32 and b <= 126 then
            text = text .. string.char(b)
        else
            text = text .. "?"
        end
    end
    return text
end

local function allSpritesHidden()
    for sprite = 0, 63 do
        local y = emu.read(0x0200 + sprite * 4, emu.memType.nesDebug)
        if y < 240 then
            return false
        end
    end
    return true
end

-- Inject input during inputPolled event
emu.addEventCallback(function()
    local input = {
        a = false, b = false, select = false, start = false,
        up = false, down = false, left = false, right = false,
    }

    if (endFrames >= 10 and endFrames <= 12) or endFrames == 20 then
        input.start = true
    elseif testPhase == "NAVIGATE_MODAL" and phaseTimer >= 5 and phaseTimer <= 7 then
        input.down = true
    elseif testPhase == "NAVIGATE_MODAL" and phaseTimer >= 15 and phaseTimer <= 17 then
        input.a = true
    end

    emu.setInput(input, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
    endFrames = endFrames + 1

    if testPhase == "START_GAME" then
        if endFrames == 35 then
            -- Verify gameplay HUD in rows 0 and 1
            local hpText = readText(0, 0, 3)
            check(hpText == "HP:", "HUD missing HP label, got: " .. hpText)
            local xpText = readText(0, 14, 3)
            check(xpText == "XP:", "HUD missing XP label, got: " .. xpText)
            local lvText = readText(0, 28, 4)
            check(lvText == "LV01", "HUD missing LV01, got: " .. lvText)
            local wpnText = readText(1, 0, 4)
            check(wpnText == "WPN:", "HUD missing WPN: label, got: " .. wpnText)
            local bnsText = readText(1, 16, 4)
            check(bnsText == "BNS:", "HUD missing BNS: label, got: " .. bnsText)

            -- Spawn XP gems directly at player position (116, 108) to trigger level up
            -- gem_x at $03A9, gem_y at $03B1, gem_active at $03B9, gem_drop_units at $03C1, pool_high_water at $03D1
            for i = 0, 4 do
                emu.write(0x03A9 + i, 116, emu.memType.nesDebug)
                emu.write(0x03B1 + i, 108, emu.memType.nesDebug)
                emu.write(0x03B9 + i, 1, emu.memType.nesDebug)
                emu.write(0x03C1 + i * 2, 1, emu.memType.nesDebug)
                emu.write(0x03C1 + i * 2 + 1, 0, emu.memType.nesDebug)
            end
            emu.write(0x03D1, 5, emu.memType.nesDebug)
            emu.write(0x03D2, 0, emu.memType.nesDebug)

            testPhase = "WAIT_FOR_LEVEL_UP"
            phaseTimer = 0
        end

    elseif testPhase == "WAIT_FOR_LEVEL_UP" then
        phaseTimer = phaseTimer + 1
        local modalText = readText(11, 8, 16)
        if modalText == "|  LEVEL UP!   |" then
            check(allSpritesHidden(), "Sprites must be hidden during level-up modal")
            local choice1 = readText(13, 8, 16)
            check(choice1 == "| > 1. SWORD +1|", "Choice 1 text incorrect, got: " .. choice1)
            local choice2 = readText(14, 8, 16)
            check(choice2 == "|   2. MAX HP+1|", "Choice 2 text incorrect, got: " .. choice2)
            local choice3 = readText(15, 8, 16)
            check(choice3 == "|   3. SPEED +1|", "Choice 3 text incorrect, got: " .. choice3)

            testPhase = "NAVIGATE_MODAL"
            phaseTimer = 0
        elseif phaseTimer > 150 then
            check(false, "Level up modal failed to appear within timeout")
            testPhase = "DONE"
        end

    elseif testPhase == "NAVIGATE_MODAL" then
        phaseTimer = phaseTimer + 1
        if phaseTimer == 10 then
            local choice1 = readText(13, 8, 16)
            local choice2 = readText(14, 8, 16)
            check(choice1 == "|   1. SWORD +1|", "Choice 1 cursor not cleared, got: " .. choice1)
            check(choice2 == "| > 2. MAX HP+1|", "Choice 2 cursor not set, got: " .. choice2)
        elseif phaseTimer == 20 then
            testPhase = "VERIFY_RESUME"
            phaseTimer = 0
        end

    elseif testPhase == "VERIFY_RESUME" then
        phaseTimer = phaseTimer + 1
        if phaseTimer == 15 then
            local modalRow = readText(11, 8, 16)
            check(modalRow == "????????????????", "Modal row not cleared back to blank tiles, got: " .. modalRow)

            local lvText = readText(0, 28, 4)
            check(lvText == "LV02", "HUD level did not advance to LV02, got: " .. lvText)

            testPhase = "DONE"
        end

    elseif testPhase == "DONE" then
        if #failures == 0 then
            local message = string.format("Level-up and HUD validation passed: %d frames", endFrames)
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
