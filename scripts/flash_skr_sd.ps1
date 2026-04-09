# Copie le firmware SKR sur la carte SD montée en USB (ex. F:).
# Supprime FIRMWARE.CUR s'il existe pour forcer la reprogrammation par le bootloader.
#
# Usage:
#   .\scripts\flash_skr_sd.ps1
#   .\scripts\flash_skr_sd.ps1 -Drive "G:"
#
param(
    [string]$Drive = $(if ($env:SKR_SD_DRIVE) { $env:SKR_SD_DRIVE } else { "F:" })
)

$ErrorActionPreference = "Stop"
$Drive = $Drive.TrimEnd('\')
$Root = if ($Drive.EndsWith(":")) { "$Drive\" } else { "$Drive\" }

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Firmware = Join-Path $RepoRoot ".pio\build\skr14turbo\firmware.bin"
$Dest = Join-Path $Root "FIRMWARE.BIN"
$Cur = Join-Path $Root "FIRMWARE.CUR"

if (-not (Test-Path $Root)) {
    Write-Error "Lecteur $Drive indisponible. Branche la SKR / la carte SD (USB MSC)."
}

if (-not (Test-Path $Firmware)) {
    Write-Error "Build introuvable: $Firmware`nLance: python -m platformio run -e skr14turbo"
}

if (Test-Path $Cur) {
    Remove-Item -LiteralPath $Cur -Force
    Write-Host "Supprimé: $Cur"
}

Copy-Item -LiteralPath $Firmware -Destination $Dest -Force
Write-Host "Copié: $Dest"
Write-Host "Ejecte la carte ou reset la carte pour flasher."
