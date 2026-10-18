param([string]$Sgdk = $env:GDK, [string]$Java = 'java', [switch]$SmokeTest, [string]$Define)
$ErrorActionPreference = 'Stop'
if (!$Sgdk) { $Sgdk = 'D:/apps/sgdk' }
if (!(Test-Path -LiteralPath "$Sgdk/makefile.gen")) { throw 'Set -Sgdk or GDK to your SGDK installation.' }
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    $extra = @()
    if ($SmokeTest) {
        $extra = @('SRC_C=src/game.c src/map.c src/map_crateria_1.c src/map_crateria_2.c src/core/world.c src/core/weapon.c src/core/input.c src/core/camera.c tests/rom_smoke.c')
    }
    # A define changes objects without changing their sources, so variants get their own objects.
    $out = if ($Define) { 'out/variant' } else { 'out' }
    # Relink when switching entry points, without forcing SDK implicit rules.
    & "$Sgdk/bin/make.exe" @extra -W src/main.c -W tests/rom_smoke.c -f "$Sgdk/makefile.gen" "OUT=$out" "JAVA=$Java" "EXTRA_FLAGS=$Define" release
    if ($LASTEXITCODE -ne 0) { throw 'SGDK build failed.' }
    python scripts/finalize_rom.py "$out/rom.bin"
    if ($LASTEXITCODE -ne 0) { throw 'ROM validation failed.' }
    New-Item -ItemType Directory -Force tests/out | Out-Null
    if ($SmokeTest) {
        Copy-Item -LiteralPath "$out/rom.bin" -Destination tests/out/smoke.bin
    } elseif ($Define) {
        Copy-Item -LiteralPath "$out/rom.bin" -Destination tests/out/variant.bin
    } else {
        Copy-Item -LiteralPath "$out/rom.bin" -Destination build.bin
    }
} finally { Pop-Location }
