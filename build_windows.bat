@echo off
chcp 65001 >nul
echo ============================================
echo Spark Player Windows Build Script
echo ============================================
echo.

set QT_DIR=
set FOUND_QT=0

echo Detecting Qt installation...

if exist "C:\Qt\6.5.3\msvc2019_64\bin\qmake.exe" (
    set QT_DIR=C:\Qt\6.5.3\msvc2019_64
    set FOUND_QT=1
) else if exist "C:\Qt\6.6.0\msvc2019_64\bin\qmake.exe" (
    set QT_DIR=C:\Qt\6.6.0\msvc2019_64
    set FOUND_QT=1
) else if exist "D:\Qt\6.5.3\msvc2019_64\bin\qmake.exe" (
    set QT_DIR=D:\Qt\6.5.3\msvc2019_64
    set FOUND_QT=1
) else if exist "D:\Qt\6.6.0\msvc2019_64\bin\qmake.exe" (
    set QT_DIR=D:\Qt\6.6.0\msvc2019_64
    set FOUND_QT=1
) else if exist "%USERPROFILE%\Qt\6.5.3\msvc2019_64\bin\qmake.exe" (
    set QT_DIR=%USERPROFILE%\Qt\6.5.3\msvc2019_64
    set FOUND_QT=1
)

if %FOUND_QT% equ 1 (
    echo Found Qt at: %QT_DIR%
) else (
    echo WARNING: Qt not found automatically.
    echo Please enter your Qt path (e.g., C:\Qt\6.5.3\msvc2019_64):
    set /p QT_DIR=
    if not exist "%QT_DIR%\bin\qmake.exe" (
        echo ERROR: qmake not found at %QT_DIR%\bin\qmake.exe
        echo.
        echo Please install Qt 6.5.3 or later from https://www.qt.io/download
        echo Make sure to select "msvc2019_64" component during installation.
        pause
        exit /b 1
    )
)

set PATH=%QT_DIR%\bin;%PATH%

echo Checking Qt environment...
where qmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: qmake not found.
    echo Please install Qt 6.5.3 with msvc2019_64 component.
    pause
    exit /b 1
)

echo Qt environment OK
qmake --version
echo.

echo Checking for Visual Studio...
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Visual Studio compiler not found.
    echo Please run this script from "Developer Command Prompt for VS 2019".
    echo Or set up MSVC environment first.
    echo.
    pause
)

echo Creating build directory...
if not exist build_win mkdir build_win
cd build_win

echo Generating Makefile...
qmake ..\spark_player.pro CONFIG+=release
if %errorlevel% neq 0 (
    echo Error: qmake failed
    pause
    exit /b 1
)

echo Building project...
nmake
if %errorlevel% neq 0 (
    echo Error: build failed
    pause
    exit /b 1
)

echo Deploying Qt dependencies...
cd release
windeployqt spark_player.exe
if %errorlevel% neq 0 (
    echo Warning: windeployqt may not complete successfully, but exe is generated.
)

echo.
echo ============================================
echo Build complete!
echo Executable: build_win\release\spark_player.exe
echo ============================================
echo.
echo Note: Ensure Visual Studio 2019+ and Qt 6.5.3 (msvc2019_64) are installed.
pause