param([string]$Compiler)
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    New-Item -ItemType Directory -Force tests/out | Out-Null
    python scripts/sm_reference_cache.py check
    if ($LASTEXITCODE -ne 0) { throw 'Saved reference validation failed.' }
    python tests/check_assets.py
    if ($LASTEXITCODE -ne 0) { throw 'Asset validation failed.' }
    python tests/check_rooms.py
    if ($LASTEXITCODE -ne 0) { throw 'Room exporter validation failed.' }
    python tests/check_world.py
    if ($LASTEXITCODE -ne 0) { throw 'World graph validation failed.' }
    if (!$Compiler) {
        $vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
        $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (!$vs) { throw 'MSVC not found. Supply -Compiler gcc or clang.' }
        $vc = Get-ChildItem "$vs/VC/Tools/MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1
        $sdk = Get-ChildItem "${env:ProgramFiles(x86)}/Windows Kits/10/Lib" -Directory | Sort-Object Name -Descending | Select-Object -First 1
        $Compiler = "$($vc.FullName)/bin/Hostx64/x64/cl.exe"
        $savedInclude = $env:INCLUDE; $savedLib = $env:LIB
        $env:INCLUDE = "$($vc.FullName)/include;${env:ProgramFiles(x86)}/Windows Kits/10/Include/$($sdk.Name)/ucrt;${env:ProgramFiles(x86)}/Windows Kits/10/Include/$($sdk.Name)/shared;${env:ProgramFiles(x86)}/Windows Kits/10/Include/$($sdk.Name)/um"
        $env:LIB = "$($vc.FullName)/lib/x64;$($sdk.FullName)/ucrt/x64;$($sdk.FullName)/um/x64"
    }
    $sources = @('tests/core_test.c','src/core/world.c','src/core/weapon.c','src/core/input.c','src/core/camera.c','src/core/door.c','src/core/worldmap.c','src/core/doorlock.c','src/core/hud.c','src/world_map.c')
    if ((Split-Path $Compiler -Leaf) -eq 'cl.exe') {
        & $Compiler /nologo /W4 /WX /TC /D_CRT_SECURE_NO_WARNINGS /Iinc @sources /Fe:tests/out/core_test.exe /Fo:tests/out/
    } else {
        & $Compiler -std=c99 -Wall -Wextra -Werror -Iinc @sources -o tests/out/core_test.exe
    }
    if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed.' }
    & ./tests/out/core_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Regression tests failed.' }
} finally {
    if (Test-Path variable:savedInclude) { $env:INCLUDE = $savedInclude; $env:LIB = $savedLib }
    Pop-Location
}
