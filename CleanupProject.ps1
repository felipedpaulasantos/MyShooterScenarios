param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Safe", "DeepScan", "Analysis")]
    [string]$Mode = "Analysis"
)

$ProjectRoot = "F:\UnrealProjects\MyShooterScenarios"
$LogFile = Join-Path $ProjectRoot "cleanup_log.txt"

function Write-Log {
    param([string]$Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "$timestamp - $Message"
    Write-Host $logMessage
    Add-Content -Path $LogFile -Value $logMessage
}

function Get-FolderSize {
    param([string]$Path)
    if (Test-Path $Path) {
        $size = (Get-ChildItem -Path $Path -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum
        return [math]::Round($size / 1GB, 2)
    }
    return 0
}

function Remove-SafeFolders {
    Write-Log "=== Starting Safe Cleanup ==="
    
    $foldersToClean = @(
        "Binaries",
        "Intermediate", 
        "Saved",
        "DerivedDataCache",
        ".vs"
    )
    
    $totalSaved = 0
    
    foreach ($folder in $foldersToClean) {
        $fullPath = Join-Path $ProjectRoot $folder
        if (Test-Path $fullPath) {
            $sizeBefore = Get-FolderSize $fullPath
            Write-Log "Removing $folder ($sizeBefore GB)..."
            try {
                Remove-Item $fullPath -Recurse -Force -ErrorAction Stop
                $totalSaved += $sizeBefore
                Write-Log "[OK] Successfully removed $folder"
            } catch {
                $errorMsg = $_.Exception.Message
                Write-Log "[ERROR] Error removing $folder - $errorMsg"
            }
        } else {
            Write-Log "[-] $folder not found (already clean)"
        }
    }
    
    # Clean plugin intermediate folders
    $pluginPath = Join-Path $ProjectRoot "Plugins"
    if (Test-Path $pluginPath) {
        Get-ChildItem -Path $pluginPath -Directory -Recurse -Filter "Intermediate" -ErrorAction SilentlyContinue | ForEach-Object {
            $size = Get-FolderSize $_.FullName
            Write-Log "Removing plugin intermediate: $($_.FullName) ($size GB)"
            Remove-Item $_.FullName -Recurse -Force -ErrorAction SilentlyContinue
            $totalSaved += $size
        }
    }
    
    Write-Log "=== Safe Cleanup Complete ==="
    Write-Log "Total space saved: $totalSaved GB"
}

function Get-LargeFiles {
    Write-Log "=== Analyzing Large Files ==="
    
    $largeFiles = Get-ChildItem -Path (Join-Path $ProjectRoot "Content") -Recurse -File -ErrorAction SilentlyContinue | 
        Where-Object { $_.Length -gt 100MB } |
        Sort-Object Length -Descending |
        Select-Object -First 50 FullName, @{Name="SizeMB";Expression={[math]::Round($_.Length / 1MB, 2)}}
    
    Write-Log "Found $($largeFiles.Count) files larger than 100 MB:"
    $largeFiles | ForEach-Object {
        Write-Log "$($_.SizeMB) MB - $($_.FullName)"
    }
    
    # Export to CSV for review
    $csvPath = Join-Path $ProjectRoot "large_files_report.csv"
    $largeFiles | Export-Csv -Path $csvPath -NoTypeInformation
    Write-Log "Large files report saved to: $csvPath"
}

function Get-ContentPackSizes {
    Write-Log "=== Analyzing Content Pack Sizes ==="
    
    $contentPath = Join-Path $ProjectRoot "Content"
    if (Test-Path $contentPath) {
        $packs = Get-ChildItem -Path $contentPath -Directory | ForEach-Object {
            $size = Get-FolderSize $_.FullName
            [PSCustomObject]@{
                Name = $_.Name
                SizeGB = $size
                Path = $_.FullName
            }
        } | Sort-Object SizeGB -Descending
        
        Write-Log "Content packs by size:"
        $packs | ForEach-Object {
            Write-Log "$($_.SizeGB) GB - $($_.Name)"
        }
        
        # Export to CSV
        $csvPath = Join-Path $ProjectRoot "content_packs_report.csv"
        $packs | Export-Csv -Path $csvPath -NoTypeInformation
        Write-Log "Content packs report saved to: $csvPath"
    }
}

function Find-SourceFiles {
    Write-Log "=== Finding Source Files (potentially removable after import) ==="
    
    $sourceExtensions = @("*.fbx", "*.FBX", "*.psd", "*.blend", "*.max", "*.ma", "*.mb")
    $sourceFiles = @()
    
    foreach ($ext in $sourceExtensions) {
        $files = Get-ChildItem -Path (Join-Path $ProjectRoot "Content") -Filter $ext -Recurse -File -ErrorAction SilentlyContinue
        $sourceFiles += $files
    }
    
    $totalSize = ($sourceFiles | Measure-Object -Property Length -Sum).Sum / 1GB
    Write-Log "Found $($sourceFiles.Count) source files totaling $([math]::Round($totalSize, 2)) GB"
    
    $sourceFiles | Group-Object Extension | ForEach-Object {
        $groupSize = ($_.Group | Measure-Object -Property Length -Sum).Sum / 1GB
        Write-Log "  $($_.Name): $($_.Count) files, $([math]::Round($groupSize, 2)) GB"
    }
    
    # Export to CSV
    $csvPath = Join-Path $ProjectRoot "source_files_report.csv"
    $sourceFiles | Select-Object FullName, Extension, @{Name="SizeMB";Expression={[math]::Round($_.Length / 1MB, 2)}} | 
        Export-Csv -Path $csvPath -NoTypeInformation
    Write-Log "Source files report saved to: $csvPath"
}

function Get-ProjectStats {
    Write-Log "=== Project Statistics ==="
    
    $stats = @{
        "Total Files" = (Get-ChildItem -Path $ProjectRoot -Recurse -File -ErrorAction SilentlyContinue).Count
        "Content Size (GB)" = Get-FolderSize (Join-Path $ProjectRoot "Content")
        "Source Size (GB)" = Get-FolderSize (Join-Path $ProjectRoot "Source")
        "Plugins Size (GB)" = Get-FolderSize (Join-Path $ProjectRoot "Plugins")
        "Binaries Size (GB)" = Get-FolderSize (Join-Path $ProjectRoot "Binaries")
        "Intermediate Size (GB)" = Get-FolderSize (Join-Path $ProjectRoot "Intermediate")
        "Saved Size (GB)" = Get-FolderSize (Join-Path $ProjectRoot "Saved")
    }
    
    $stats.GetEnumerator() | Sort-Object Name | ForEach-Object {
        Write-Log "$($_.Key): $($_.Value)"
    }
}

# Main execution
Write-Log "=== Unreal Engine Project Cleanup Tool ==="
Write-Log "Project: $ProjectRoot"
Write-Log "Mode: $Mode"

switch ($Mode) {
    "Safe" {
        Write-Log "Running SAFE cleanup (removes only regenerable folders)..."
        $response = Read-Host "This will delete Binaries, Intermediate, and Saved folders. Continue? (Y/N)"
        if ($response -eq "Y" -or $response -eq "y") {
            Remove-SafeFolders
        } else {
            Write-Log "Cleanup cancelled by user."
        }
    }
    "DeepScan" {
        Write-Log "Running DEEP SCAN (analysis + safe cleanup)..."
        Get-ProjectStats
        Get-ContentPackSizes
        Get-LargeFiles
        Find-SourceFiles
        
        Write-Log "`n=== Analysis Complete ==="
        Write-Log "Review the generated CSV reports before deleting content."
        
        $response = Read-Host "`nAlso run safe cleanup? (Y/N)"
        if ($response -eq "Y" -or $response -eq "y") {
            Remove-SafeFolders
        }
    }
    "Analysis" {
        Write-Log "Running ANALYSIS ONLY (no files will be deleted)..."
        Get-ProjectStats
        Get-ContentPackSizes
        Get-LargeFiles
        Find-SourceFiles
        Write-Log "`n=== Analysis Complete ==="
        Write-Log "Review the generated reports in the project root folder."
    }
}

Write-Log "`n=== Script Complete ==="
Write-Log "Log saved to: $LogFile"
