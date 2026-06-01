@echo off
cd /d "%~dp0"

:: Set environment variables using MSYS2 style paths for the Makefile
set "DEVKITPRO=/c/Projects/V2NES/devkitPro"
set "DEVKITARM=/c/Projects/V2NES/devkitPro/devkitARM"
set "CTRULIB=/c/Projects/V2NES/devkitPro/libctru"

:: Add the Windows paths to the PATH so we can find make.exe and the compiler
set "PATH=C:\Projects\V2NES\devkitPro\msys2\usr\bin;C:\Projects\V2NES\devkitPro\tools\bin;C:\Projects\V2NES\devkitPro\devkitARM\bin;%PATH%"

echo Cleaning previous V2NES builds...
make -C 3ds_source clean DEVKITPRO=%DEVKITPRO% DEVKITARM=%DEVKITARM% CTRULIB=%CTRULIB%

echo Building V2NES...
:: Run make with the variables explicitly passed to be safe
make -C 3ds_source DEVKITPRO=%DEVKITPRO% DEVKITARM=%DEVKITARM% CTRULIB=%CTRULIB%

if %ERRORLEVEL% equ 0 (
    echo.
    echo Build Successful!
    echo Output: 3ds_source\V2NES.3dsx
) else (
    echo.
    echo Build Failed!
)
pause
