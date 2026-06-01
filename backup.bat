@echo off
setlocal enabledelayedexpansion

:: Build timestamp from built-in DATE and TIME variables (no wmic needed)
:: DATE is typically: Mon 05/19/2026  ->  tokens 2 = 05/19/2026
for /f "tokens=2 delims= " %%D in ("%DATE%") do set DATEVAL=%%D
:: TIME is typically: 18:15:09.12  ->  strip colons and dots
set TIMEVAL=%TIME: =0%
set TIMEVAL=%TIMEVAL::=%
set TIMEVAL=%TIMEVAL:.=%
:: Take first 6 digits of time (HHMMSS)
set TIMEVAL=%TIMEVAL:~0,6%
:: Reformat date from MM/DD/YYYY to YYYYMMDD
for /f "tokens=1,2,3 delims=/" %%A in ("%DATEVAL%") do (
    set MM=%%A
    set DD=%%B
    set YYYY=%%C
)
set TIMESTAMP=%YYYY%%MM%%DD%_%TIMEVAL%

set BACKUP_DIR=Backups\Backup_%TIMESTAMP%

echo ====================================================
echo             V2NES Code Backup Utility
echo ====================================================
echo.
echo Creating backup: %BACKUP_DIR%
mkdir "%BACKUP_DIR%"
mkdir "%BACKUP_DIR%\3ds_source\source"
mkdir "%BACKUP_DIR%\companion_app"

echo.
echo [1/3] Copying emulator source files...
copy "3ds_source\source\main.cpp"    "%BACKUP_DIR%\3ds_source\source\" >nul
copy "3ds_source\source\content.cpp" "%BACKUP_DIR%\3ds_source\source\" >nul
copy "3ds_source\source\content.h"   "%BACKUP_DIR%\3ds_source\source\" >nul
copy "3ds_source\source\ui.cpp"      "%BACKUP_DIR%\3ds_source\source\" >nul
copy "3ds_source\source\ui.h"        "%BACKUP_DIR%\3ds_source\source\" >nul
echo     Done.

echo.
echo [2/3] Copying companion app source files...
copy "companion_app\v2m_compiler.cs" "%BACKUP_DIR%\companion_app\" >nul
echo     Done.

echo.
echo [3/3] Copying project documentation...
if exist "CHANGELOG.md"           copy "CHANGELOG.md"           "%BACKUP_DIR%\" >nul
if exist "MAPS_TROUBLESHOOTING.md" copy "MAPS_TROUBLESHOOTING.md" "%BACKUP_DIR%\" >nul
echo     Done.

echo.
echo ====================================================
echo  Backup saved to: %BACKUP_DIR%
echo ====================================================
echo.
pause
