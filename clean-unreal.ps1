# clean-unreal.ps1
param(
    [string]$Root = (Get-Location)
)

# Pastas que podem ser deletadas com segurança
$safeToDelete = @("Binaries", "Intermediate", "Saved", "DerivedDataCache")

Write-Host "Cleaning Unreal project in $Root ..." -ForegroundColor Cyan

# Função recursiva
function Traverse-And-Clean {
    param([string]$Dir)

    foreach ($entry in Get-ChildItem -LiteralPath $Dir -Force) {
        if ($entry.PSIsContainer) {
            if ($safeToDelete -contains $entry.Name) {
                try {
                    Remove-Item -LiteralPath $entry.FullName -Recurse -Force -ErrorAction Stop
                    Write-Host "Deleted: $($entry.FullName)" -ForegroundColor Green
                }
                catch {
                    Write-Host "Failed to delete: $($entry.FullName) - $_" -ForegroundColor Red
                }
            }
            else {
                Traverse-And-Clean $entry.FullName
            }
        }
    }
}

Traverse-And-Clean $Root

Write-Host "Done." -ForegroundColor Cyan
