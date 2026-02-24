# Timestamp Update in RC_AUTO_NEW2.ino

This project has an automatic system for updating the timestamp comment in the main file `RC_AUTO_NEW2.ino`.

## Update Methods

### 1. Automatic Rule (Recommended)
Cursor IDE automatically reminds you to update the timestamp when working on this project thanks to the rule in `.cursor/rules/update-timestamp.mdc`.

### 2. Manual Scripts (Windows)
Before committing, run one of the following scripts:

```batch
# Batch file (simplest)
update_timestamp.bat

# Or PowerShell
.\update_timestamp.ps1
```

The script automatically updates the first line in `RC_AUTO_NEW2.ino` with the current date and time.

## Timestamp Format
```
// Last updated: [Day] [Month] [Day], [Year], [HH:MM]
// Example: // Last updated: Monday Feb 23, 2026, 19:12
```

## Notes
- Timestamp is updated only in the main file `RC_AUTO_NEW2.ino`
- English day and month names are used
- Time is in 24-hour format