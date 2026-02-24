@echo off
REM Script for updating timestamp in RC_AUTO_NEW2.ino
REM Run before commit: update_timestamp.bat

powershell -Command "& { [System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::CreateSpecificCulture('en-US'); $filename = 'RC_AUTO_NEW2.ino'; $currentDate = Get-Date -Format 'dddd MMM d, yyyy, HH:mm'; $content = Get-Content $filename; $content[0] = '// Last updated: ' + $currentDate; $content | Set-Content $filename; Write-Host 'Timestamp updated:' $currentDate }"