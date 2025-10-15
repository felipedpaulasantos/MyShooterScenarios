# Fix Deprecated FTopLevelAssetPath Format - UE 5.5 Upgrade
# This script resaves all assets to update deprecated path formats

Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "Fix Deprecated FTopLevelAssetPath Format - UE 5.5 Upgrade" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "This will resave all assets to update deprecated path formats." -ForegroundColor Yellow
Write-Host "This may take several minutes depending on project size." -ForegroundColor Yellow
Write-Host ""

$confirmation = Read-Host "Do you want to continue? (Y/N)"
if ($confirmation -ne 'Y' -and $confirmation -ne 'y') {
    Write-Host "Operation cancelled." -ForegroundColor Red
    exit
}

# Try to find UE 5.5 installation
$UEPaths = @(
    "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    "C:\Program Files (x86)\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    "D:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
)

$UEEditor = $null
foreach ($path in $UEPaths) {
    if (Test-Path $path) {
        $UEEditor = $path
        break
    }
}

if (-not $UEEditor) {
    Write-Host "ERROR: Could not find UE 5.5 installation!" -ForegroundColor Red
    Write-Host "Please edit this script and set the correct path to UnrealEditor-Cmd.exe" -ForegroundColor Yellow
    pause
    exit
}

$ProjectFile = Join-Path $PSScriptRoot "MY_SHOOTER.uproject"

if (-not (Test-Path $ProjectFile)) {
    Write-Host "ERROR: Project file not found: $ProjectFile" -ForegroundColor Red
    pause
    exit
}

Write-Host ""
Write-Host "Using Unreal Engine: $UEEditor" -ForegroundColor Green
Write-Host "Project: $ProjectFile" -ForegroundColor Green
Write-Host ""
Write-Host "Starting asset resave process..." -ForegroundColor Cyan
Write-Host ""

# Run the ResavePackages commandlet
& $UEEditor $ProjectFile -run=ResavePackages -autocheckout -progressonly -unattended -NoShaderCompile -SkipDeveloperFolders

Write-Host ""
Write-Host "================================================================" -ForegroundColor Green
Write-Host "Resave Complete!" -ForegroundColor Green
Write-Host "================================================================" -ForegroundColor Green
Write-Host ""
Write-Host "The deprecated FTopLevelAssetPath warnings should now be fixed." -ForegroundColor Yellow
Write-Host "Restart the editor to verify." -ForegroundColor Yellow
Write-Host ""
pause

