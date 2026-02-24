# Script for updating timestamp in RC_AUTO_NEW2.ino
# Run before commit: .\update_timestamp.ps1

$filename = "RC_AUTO_NEW2.ino"
$currentDate = Get-Date -Format "dddd MMM d, yyyy, HH:mm"

# Read current file content
$content = Get-Content $filename

# Replace first line with new timestamp
$content[0] = "// Last updated: $currentDate"

# Save back to file
$content | Set-Content $filename

Write-Host "Timestamp aktualizován: $currentDate"