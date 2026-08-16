param(
    [ValidateSet("build", "clean", "test", "runtime", "performance")]
    [string]$Action = "build"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$BuildDir = [System.IO.Path]::GetFullPath((Join-Path $ProjectRoot "build"))

if ([System.IO.Path]::GetDirectoryName($BuildDir) -ne $ProjectRoot) {
    throw "Refusing to operate on a build directory outside the project root."
}

function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE"
    }
}

function Build-Rom {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

    $cModules = @("main", "game", "input", "rng", "animation", "metasprite", "player", "soldier_animation_data", "weapon_sword", "enemy", "bat_animation_data")
    $asmModules = @("crt0", "nmi", "nes", "chr")
    $objects = [System.Collections.Generic.List[string]]::new()

    foreach ($module in $asmModules) {
        $object = Join-Path $BuildDir "$module.o"
        $arguments = @("-t", "nes", "--warnings-as-errors", "-I", "include", "--bin-include-dir", "assets", "-o", $object, "src/$module.s")
        Invoke-Checked "ca65" $arguments
        $objects.Add($object)
    }

    foreach ($module in $cModules) {
        $assembly = Join-Path $BuildDir "c_$module.s"
        $object = Join-Path $BuildDir "c_$module.o"
        Invoke-Checked "cc65" @("-t", "nes", "-Oirs", "--standard", "c99", "--warnings-as-errors", "-I", "include", "--add-source", "-o", $assembly, "src/$module.c")
        Invoke-Checked "ca65" @("-t", "nes", "--warnings-as-errors", "-I", "include", "-o", $object, $assembly)
        $objects.Add($object)
    }

    $rom = Join-Path $BuildDir "nes-survivor.nes"
    $map = Join-Path $BuildDir "nes-survivor.map"
    $labels = Join-Path $BuildDir "nes-survivor.lbl"
    $linkArguments = @("-C", "cfg/nrom.cfg", "--warnings-as-errors", "-m", $map, "-Ln", $labels, "-o", $rom) + $objects + @("nes.lib")
    Invoke-Checked "ld65" $linkArguments
}

function Clean-Build {
    if (Test-Path -LiteralPath $BuildDir) {
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    }
}

function Run-Tests {
    Build-Rom
    $testBinary = Join-Path $BuildDir "test_logic"
    Invoke-Checked "cl65" @("-t", "sim6502", "--standard", "c99", "--warnings-as-errors", "-DUNIT_TEST", "-I", "include", "-o", $testBinary, "tests/test_logic.c", "src/input.c", "src/rng.c", "src/animation.c", "src/metasprite.c", "src/player.c", "src/soldier_animation_data.c", "src/weapon_sword.c", "src/enemy.c", "src/bat_animation_data.c")
    Invoke-Checked "sim65" @($testBinary)
    Invoke-Checked "python" @("tests/validate_rom.py", "build/nes-survivor.nes", "build/nes-survivor.map", "build/nes-survivor.lbl")
}

function Run-MesenRuntime {
    param(
        [string]$TestScript = "tests/mesen_player.lua",
        [string]$LogPrefix = "mesen-runtime"
    )

    $mesenPath = (Get-Command "mesen" -ErrorAction Stop).Source
    $stdout = Join-Path $BuildDir "$LogPrefix.stdout.log"
    $stderr = Join-Path $BuildDir "$LogPrefix.stderr.log"
    $arguments = @("--testrunner", "build/nes-survivor.nes", $TestScript)
    $process = Start-Process -FilePath $mesenPath -ArgumentList $arguments `
        -WorkingDirectory $ProjectRoot -Wait -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr

    if (Test-Path -LiteralPath $stdout) {
        Get-Content -LiteralPath $stdout
    }
    if (Test-Path -LiteralPath $stderr) {
        Get-Content -LiteralPath $stderr | Write-Error
    }
    if ($process.ExitCode -ne 0) {
        throw "Mesen runtime validation failed with exit code $($process.ExitCode)"
    }
}

Push-Location $ProjectRoot
try {
    switch ($Action) {
        "build" { Build-Rom }
        "clean" { Clean-Build }
        "test" { Run-Tests }
        "runtime" {
            Build-Rom
            Run-MesenRuntime
        }
        "performance" {
            Build-Rom
            Run-MesenRuntime "tests/mesen_bat_stress.lua" "mesen-performance"
        }
    }
}
finally {
    Pop-Location
}
