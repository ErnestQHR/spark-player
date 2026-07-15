@echo off
chcp 65001 >nul
echo ============================================
echo Spark Player Windows Build Script (MinGW)
echo ============================================
echo.

set QT_DIR=
set FOUND_QT=0

echo Detecting Qt installation...

if exist "C:\Qt\6.5.3\mingw_64\bin\qmake.exe" (
    set QT_DIR=C:\Qt\6.5.3\mingw_64
    set FOUND_QT=1
) else if exist "C:\Qt\6.6.0\mingw_64\bin\qmake.exe" (
    set QT_DIR=C:\Qt\6.6.0\mingw_64
    set FOUND_QT=1
) else if exist "D:\Qt\6.5.3\mingw_64\bin\qmake.exe" (
    set QT_DIR=D:\Qt\6.5.3\mingw_64
    set FOUND_QT=1
) else if exist "D:\Qt\6.6.0\mingw_64\bin\qmake.exe" (
    set QT_DIR=D:\Qt\6.6.0\mingw_64
    set FOUND_QT=1
)

if %FOUND_QT% equ 1 (
    echo Found Qt at: %QT_DIR%
) else (
    echo WARNING: Qt not found automatically.
    echo Please enter your Qt path (e.g., C:\Qt\6.5.3\mingw_64):
    set /p QT_DIR=
    if not exist "%QT_DIR%\bin\qmake.exe" (
        echo ERROR: qmake not found at %QT_DIR%\bin\qmake.exe
        pause
        exit /b 1
    )
)

set PATH=%QT_DIR%\bin;%PATH%

echo Checking Qt environment...
where qmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: qmake not found.
    pause
    exit /b 1
)

echo Qt environment OK
qmake --version
echo.

echo Creating build directory...
if not exist build_win_mingw mkdir build_win_mingw
cd build_win_mingw

echo Generating Makefile...
qmake ..\spark_player.pro CONFIG+=release
if %errorlevel% neq 0 (
    echo Error: qmake failed
    pause
    exit /b 1
)

echo Building project...
mingw32-make -j4
if %errorlevel% neq 0 (
    echo Error: build failed
    pause
    exit /b 1
)

echo Deploying Qt dependencies...
windeployqt spark_player.exe
if %errorlevel% neq 0 (
    echo Warning: windeployqt may not complete successfully, but exe is generated.
)

echo.
echo ============================================
echo Build complete!
echo Executable: build_win_mingw\spark_player.exe
echo ============================================
pause