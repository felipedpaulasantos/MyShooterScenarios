param(
    [string]$Root = "F:\UnrealProjects\MyShooterScenarios",
    [int]$TailLines = 200,
    [int]$PollSeconds = 1
)

$logDir = Join-Path $Root "Saved\Logs"
$logPath = Join-Path $logDir "MY_SHOOTER.log"

if (-not (Test-Path $logDir)) {
    Write-Host "Logs directory not found: $logDir" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path $logPath)) {
    $candidates = Get-ChildItem -Path $logDir -Filter "*.log" -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending

    if ($candidates.Count -eq 0) {
        Write-Host "No .log files found in: $logDir" -ForegroundColor Yellow
        exit 1
    }

    $logPath = $candidates[0].FullName
    Write-Host "MY_SHOOTER.log not found; tailing newest log instead: $logPath" -ForegroundColor Yellow
}

Write-Host "== Tailing: $logPath ==" -ForegroundColor Cyan
Write-Host "(Ctrl+C to stop)" -ForegroundColor Cyan

# Print a short tail first (helpful when attaching after the editor is already running)
try {
    Get-Content -Path $logPath -Tail $TailLines -ErrorAction Stop
}
catch {
    Write-Host "Failed to read initial tail from ${logPath}: $($_.Exception.Message)" -ForegroundColor Red
}

# Follow
try {
    Get-Content -Path $logPath -Wait -Tail 0 -ErrorAction Stop
}
catch {
    Write-Host "Stopped tailing ${logPath}: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

