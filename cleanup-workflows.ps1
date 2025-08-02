# PowerShell script to clean up failed GitHub Actions workflow runs
# This script will delete all failed workflow runs while keeping successful ones

Write-Host "🧹 Cleaning up failed GitHub Actions workflow runs..." -ForegroundColor Yellow

# Get all workflow runs with status 'failure' or 'cancelled'
$failedRuns = gh run list --status failure --limit 100 --json databaseId,conclusion,status | ConvertFrom-Json
$cancelledRuns = gh run list --status cancelled --limit 100 --json databaseId,conclusion,status | ConvertFrom-Json

$allFailedRuns = $failedRuns + $cancelledRuns

if ($allFailedRuns.Count -eq 0) {
    Write-Host "✅ No failed workflow runs found to clean up!" -ForegroundColor Green
    exit 0
}

Write-Host "Found $($allFailedRuns.Count) failed/cancelled workflow runs to delete..." -ForegroundColor Cyan

$deleted = 0
foreach ($run in $allFailedRuns) {
    try {
        Write-Host "Deleting run ID: $($run.databaseId)..." -ForegroundColor Gray
        gh run delete $run.databaseId 2>$null
        if ($LASTEXITCODE -eq 0) {
            $deleted++
            Write-Host "✓ Deleted run $($run.databaseId)" -ForegroundColor Green
        } else {
            Write-Host "⚠ Failed to delete run $($run.databaseId)" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "❌ Error deleting run $($run.databaseId): $($_.Exception.Message)" -ForegroundColor Red
    }
    
    # Small delay to avoid rate limiting
    Start-Sleep -Milliseconds 200
}

Write-Host ""
Write-Host "🎉 Cleanup completed! Deleted $deleted workflow runs." -ForegroundColor Green
Write-Host ""
Write-Host "Current workflow status:" -ForegroundColor Cyan
gh run list --limit 5