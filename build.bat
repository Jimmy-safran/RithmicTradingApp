@echo off
echo Building RithmicTradingApp project...

REM Check if g++ is available
g++ --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: g++ not found. Please install MinGW or use Visual Studio.
    echo.
    echo For Visual Studio, run from Developer Command Prompt:
         echo cl src\RithmicTradingApp.cpp /I./include /I../rapi-sdk/13.5.0.0/include /link /LIBPATH:../rapi-sdk/13.5.0.0/win10/lib RApiPlus_md64.lib OmneStreamEngine_md64.lib OmneChannel_md64.lib OmneEngine_md64.lib api_md64.lib apistb_md64.lib kit_md64.lib libssl_md64.lib libcrypto_md64.lib zlib_md64.lib ws2_32.lib iphlpapi.lib crypt32.lib /OUT:bin\RithmicTradingApp.exe
    pause
    exit /b 1
)

REM Build with g++
g++ -O3 -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing -std=c++11 -DWIN32 -D_WINDOWS -I./include -I../rapi-sdk/13.5.0.0/include -o bin/RithmicTradingApp.exe src/RithmicTradingApp.cpp -L../rapi-sdk/13.5.0.0/win10/lib -lRApiPlus_md64 -lOmneStreamEngine_md64 -lOmneChannel_md64 -lOmneEngine_md64 -lapi_md64 -lapistb_md64 -lkit_md64 -llibssl_md64 -llibcrypto_md64 -lzlib_md64 -lws2_32 -liphlpapi -lcrypt32

if %errorlevel% equ 0 (
    echo.
    echo ✓ Build successful!
    echo Executable created: bin\RithmicTradingApp.exe
    echo.
    echo To run the program:
    echo bin\RithmicTradingApp.exe [username] [password] [exchange] [ticker] [side] [order_type] [price] [quantity]
    echo.
    echo Example:
    echo bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B open_long 3366.7 1
) else (
    echo.
    echo ✗ Build failed!
)

pause
