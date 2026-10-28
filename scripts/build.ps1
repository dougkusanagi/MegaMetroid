param([string]$Sgdk = $env:GDK, [string]$Java = 'java', [switch]$SmokeTest, [string]$Define, [string]$OutDir)
$ErrorActionPreference = 'Stop'
if (!$Sgdk) { $Sgdk = 'D:/apps/sgdk' }
if (!(Test-Path -LiteralPath "$Sgdk/makefile.gen")) { throw 'Set -Sgdk or GDK to your SGDK installation.' }
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    $extra = @()
    if ($SmokeTest) {
        # Generate the ROM's animation expectations from the versioned original traces.
        python scripts/sm_reference_cache.py check
        if ($LASTEXITCODE -ne 0) { throw 'Saved reference validation failed.' }
        $maps = (Get-ChildItem src -Filter 'map*.c' | Sort-Object Name | ForEach-Object { "src/$($_.Name)" }) -join ' '
        $extra = @("SRC_C=src/game.c $maps src/core/world.c src/core/weapon.c src/core/input.c src/core/camera.c src/core/door.c src/core/worldmap.c src/core/doorlock.c src/core/hud.c src/world_map.c tests/rom_smoke.c")
    }
    # Defines change objects without changing their sources; give each define a stable object directory.
    if ($Define) {
        $sha256 = [System.Security.Cryptography.SHA256]::Create()
        try {
            $variantHash = [BitConverter]::ToString($sha256.ComputeHash([Text.Encoding]::UTF8.GetBytes($Define))).Replace('-', '').Substring(0, 12).ToLowerInvariant()
        } finally { $sha256.Dispose() }
    }
    $out = if ($OutDir) { $OutDir } elseif ($Define) { "out/variants/$variantHash" } else { 'out' }
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
