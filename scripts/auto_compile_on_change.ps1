param(
    [string]$Environment = "skr14turbo",
    [int]$DebounceMs = 800
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

function Resolve-PlatformIo {
    $localPio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
    if (Test-Path $localPio) {
        return $localPio
    }

    $cmd = Get-Command platformio -ErrorAction SilentlyContinue
    if ($null -ne $cmd) {
        return $cmd.Source
    }

    throw "PlatformIO introuvable. Installe PlatformIO ou verifie le chemin .platformio\\penv\\Scripts\\platformio.exe"
}

$platformio = Resolve-PlatformIo
Write-Host "[auto-compile] Repo: $repoRoot"
Write-Host "[auto-compile] Env : $Environment"
Write-Host "[auto-compile] PIO : $platformio"

$buildQueued = $false
$buildRunning = $false
$lastEventAt = Get-Date

$watchedFiles = @(
    "platformio.ini",
    "Configuration.hpp",
    "Configuration_adv.hpp",
    "Configuration_local.hpp",
    "LocalConfiguration.hpp",
    "Version.h"
)

$watchers = @()
$dirs = @("src", "boards", "scripts")
foreach ($dir in $dirs) {
    $full = Join-Path $repoRoot $dir
    if (Test-Path $full) {
        $w = New-Object System.IO.FileSystemWatcher
        $w.Path = $full
        $w.Filter = "*.*"
        $w.IncludeSubdirectories = $true
        $w.EnableRaisingEvents = $true
        $watchers += $w
    }
}

$rootWatcher = New-Object System.IO.FileSystemWatcher
$rootWatcher.Path = $repoRoot
$rootWatcher.Filter = "*.*"
$rootWatcher.IncludeSubdirectories = $false
$rootWatcher.EnableRaisingEvents = $true
$watchers += $rootWatcher

$action = {
    $ext = [System.IO.Path]::GetExtension($Event.SourceEventArgs.Name)
    $name = $Event.SourceEventArgs.Name

    if ($Event.Sender.Path -eq $repoRoot) {
        if ($watchedFiles -notcontains $name) {
            return
        }
    } else {
        if (@(".hpp", ".h", ".cpp", ".c", ".ino", ".py", ".ini") -notcontains $ext) {
            return
        }
    }

    $script:lastEventAt = Get-Date
    $script:buildQueued = $true
    Write-Host "[auto-compile] Changement detecte: $name ($($Event.SourceEventArgs.ChangeType))"
}

$subscriptions = @()
foreach ($w in $watchers) {
    $subscriptions += Register-ObjectEvent -InputObject $w -EventName Changed -Action $action
    $subscriptions += Register-ObjectEvent -InputObject $w -EventName Created -Action $action
    $subscriptions += Register-ObjectEvent -InputObject $w -EventName Renamed -Action $action
    $subscriptions += Register-ObjectEvent -InputObject $w -EventName Deleted -Action $action
}

try {
    Write-Host "[auto-compile] Surveillance active. Ctrl+C pour arreter."
    while ($true) {
        if ($buildQueued -and -not $buildRunning) {
            $elapsed = (Get-Date) - $lastEventAt
            if ($elapsed.TotalMilliseconds -ge $DebounceMs) {
                $buildQueued = $false
                $buildRunning = $true
                Write-Host "[auto-compile] Build lancee..."

                & $platformio run -e $Environment
                $exitCode = $LASTEXITCODE
                if ($exitCode -eq 0) {
                    Write-Host "[auto-compile] Build OK"
                }
                else {
                    Write-Host "[auto-compile] Build FAILED (code $exitCode)"
                }

                $buildRunning = $false
            }
        }

        Start-Sleep -Milliseconds 200
    }
}
finally {
    foreach ($s in $subscriptions) {
        Unregister-Event -SourceIdentifier $s.Name -ErrorAction SilentlyContinue
    }
    foreach ($w in $watchers) {
        $w.EnableRaisingEvents = $false
        $w.Dispose()
    }
}
