@echo off
echo Building RithmicTradingApp with Visual Studio...

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

REM Build the project
msbuild RithmicTradingApp.sln /p:Configuration=Release /p:Platform=x64

if %errorlevel% equ 0 (
    echo.
    echo ✓ Build successful!
    echo Executable created: bin\RithmicTradingApp.exe
) else (
    echo.
    echo ✗ Build failed!
)

pause
