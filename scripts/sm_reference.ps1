param(
    [string]$Mesen = 'D:/emuladores/Mesen_2.1.1_Windows/Mesen.exe',
    [string]$Rom = 'D:/roms/snes/Super Metroid (Japan, USA) (En,Ja).zip',
    [string]$State = "$([Environment]::GetFolderPath('MyDocuments'))/Mesen2/SaveStates/Super Metroid (Japan, USA) (En,Ja)_1.mss",
    [string]$Out = 'tests/out/sm_reference.csv',
    [string]$Scenario,
    [string]$Script = (Join-Path $PSScriptRoot 'sm_reference.lua')
)
# Mede a física do Super Metroid original no Mesen 2, sem janela (ver scripts/sm_reference.lua).
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    if (!(Test-Path -LiteralPath $State)) { throw "Save state não encontrado: $State" }
    # O modo --testrunner não abre ROM dentro de .zip: extrai para uma pasta temporária.
    if ($Rom -like '*.zip') {
        $dir = Join-Path ([IO.Path]::GetTempPath()) 'mm_sm_rom'
        New-Item -ItemType Directory -Force $dir | Out-Null
        Expand-Archive -LiteralPath $Rom -DestinationPath $dir -Force
        $Rom = (Get-ChildItem -LiteralPath $dir -Include '*.sfc', '*.smc' -Recurse | Select-Object -First 1).FullName
    }
    New-Item -ItemType Directory -Force (Split-Path $Out -Parent) | Out-Null
    $env:MM_SM_STATE = (Resolve-Path -LiteralPath $State).Path
    $env:MM_SM_OUT = Join-Path (Get-Location) $Out
    if ($Scenario) { $env:MM_SM_SCENARIO = $Scenario } else { Remove-Item Env:MM_SM_SCENARIO -ErrorAction SilentlyContinue }
    # O script precisa de io/os para ler o state e gravar o CSV; a opção vale só nesta execução.
    & $Mesen --testrunner --timeout=300 --debug.scriptWindow.allowIoOsAccess=true `
        $Script $Rom | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Mesen terminou com código $LASTEXITCODE." }
    @{
        rom_sha256 = (Get-FileHash -LiteralPath $Rom -Algorithm SHA256).Hash.ToLower()
        state_sha256 = (Get-FileHash -LiteralPath $State -Algorithm SHA256).Hash.ToLower()
        script_sha256 = (Get-FileHash -LiteralPath $Script -Algorithm SHA256).Hash.ToLower()
    } | ConvertTo-Json | Set-Content -LiteralPath "$env:MM_SM_OUT.source.json" -Encoding utf8
    Write-Output "Medidas em $Out"
} finally { Pop-Location }
