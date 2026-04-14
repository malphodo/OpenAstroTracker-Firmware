# Build a single-file Windows executable (requires Python 3 + pip).
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
python -m pip install --upgrade pip | Out-Null
python -m pip install -r requirements.txt
python -m PyInstaller --noconfirm --clean --onefile --windowed --name "OAT_Test_Harness" oat_test_harness.py
Write-Host "Sortie : $PSScriptRoot\dist\OAT_Test_Harness.exe"
