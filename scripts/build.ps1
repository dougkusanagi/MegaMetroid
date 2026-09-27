param([string]$Sgdk = $env:GDK, [string]$Java = 'java', [switch]$SmokeTest)
$ErrorActionPreference = 'Stop'
if (!$Sgdk) { $Sgdk = 'D:/apps/sgdk' }
if (!(Test-Path -LiteralPath "$Sgdk/makefile.gen")) { throw 'Set -Sgdk or GDK to your SGDK installation.' }
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    # Isolated output prevents stale tracked .o files from entering the ROM.
    $extra = @()
    if ($SmokeTest) {
        $extra = @('SRC_C=src/game.c src/beam_sprite.c src/map.c src/map_crateria_1.c src/map_crateria_2.c src/core/world.c src/core/weapon.c src/core/input.c src/core/camera.c tests/rom_smoke.c')
    }
    # Relink when switching entry points, without forcing SDK implicit rules.
    & "$Sgdk/bin/make.exe" @extra -W src/main.c -W tests/rom_smoke.c -f "$Sgdk/makefile.gen" 'OUT=build' "JAVA=$Java" release
    if ($LASTEXITCODE -ne 0) { throw 'SGDK build failed.' }
    python scripts/finalize_rom.py build/rom.bin
    if ($LASTEXITCODE -ne 0) { throw 'ROM validation failed.' }
    if ($SmokeTest) {
        New-Item -ItemType Directory -Force tests/out | Out-Null
        Copy-Item -LiteralPath build/rom.bin -Destination tests/out/smoke.bin
    } else {
        Copy-Item -LiteralPath build/rom.bin -Destination build.bin
    }
} finally { Pop-Location }
