param(
    [string]$Root = "F:\UnrealProjects\MyShooterScenarios",
    [int]$TailLines = 200,
    [int]$MaxCrashes = 5
)

$crashesDir = Join-Path $Root "Saved\Crashes"

Write-Host "== Latest crash logs (tail $TailLines lines) ==" -ForegroundColor Cyan

if (-not (Test-Path $crashesDir)) {
    Write-Host "No crashes directory found at: $crashesDir" -ForegroundColor Yellow
    exit 0
}

Get-ChildItem -Path $crashesDir -Directory -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First $MaxCrashes |
    ForEach-Object {
        $crashDir = $_.FullName

        # Prefer the project log if present, but fall back to any .log under the crash directory.
        $preferredLogPath = Join-Path $crashDir "MY_SHOOTER.log"

        $logCandidates = @()
        if (Test-Path -LiteralPath $preferredLogPath) {
            $logCandidates += Get-Item -LiteralPath $preferredLogPath -ErrorAction SilentlyContinue
        }

        $logCandidates += Get-ChildItem -LiteralPath $crashDir -File -Filter "*.log" -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -ne $preferredLogPath } |
            Sort-Object LastWriteTime -Descending

        if ($logCandidates.Count -eq 0) {
            return
        }

        # Tail the first few log files (some crash folders can contain multiple logs).
        $maxLogsPerCrash = 3
        $logCandidates |
            Select-Object -First $maxLogsPerCrash |
            ForEach-Object {
                $logPath = $_.FullName
                Write-Host "\n--- $($_.Directory.Name) :: $($_.Name) ($($_.LastWriteTime)) ---" -ForegroundColor Green
                try {
                    Get-Content -LiteralPath $logPath -Tail $TailLines -ErrorAction Stop
                }
                catch {
                    Write-Host "Failed to read ${logPath}: $($_.Exception.Message)" -ForegroundColor Red
                }
            }
    }

Write-Host "\nTip: search for specific errors" -ForegroundColor Cyan
Write-Host "  Select-String -Path \"$crashesDir\UECC-Windows-*\*.log\" -Pattern \"Fatal error:\",\"Assertion failed:\",\"Access violation\",\"Ran out of memory\" -Context 2,20"

