@echo off
echo ====================================================
echo        V2NES Companion App Compiler
echo ====================================================
echo.
echo Searching for MSBuild/csc.exe...
set CSC_PATH=C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe

if not exist "%CSC_PATH%" (
    echo [ERROR] Could not find csc.exe at %CSC_PATH%
    echo Make sure .NET Framework 4.0 or higher is installed.
    pause
    exit /b 1
)

echo Compiling v2m_compiler.cs to v2m_compiler.exe...
"%CSC_PATH%" /target:winexe /out:v2m_compiler.exe v2m_compiler.cs

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo.
echo [SUCCESS] v2m_compiler.exe compiled successfully!
echo.
pause
