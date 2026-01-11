@echo off
REM Build script for native C++ library
REM This script builds the native library using Gradle

echo ========================================
echo Building Native C++ Library
echo ========================================
echo.

REM Check if gradlew exists
if not exist "gradlew.bat" (
    echo Error: gradlew.bat not found!
    echo Please run this script from the project root directory.
    pause
    exit /b 1
)

REM Parse command line arguments
set BUILD_TYPE=debug
set CLEAN=false
set ABIS=all

:parse_args
if "%~1"=="" goto :build
if /i "%~1"=="clean" set CLEAN=true
if /i "%~1"=="release" set BUILD_TYPE=release
if /i "%~1"=="debug" set BUILD_TYPE=debug
if /i "%~1"=="all" set ABIS=all
shift
goto :parse_args

:build
echo Build Type: %BUILD_TYPE%
echo.

REM Clean if requested
if "%CLEAN%"=="true" (
    echo Cleaning previous build...
    call gradlew.bat clean
    echo.
)

REM Build native library
echo Building native library (%BUILD_TYPE%)...
echo.

if "%BUILD_TYPE%"=="release" (
    call gradlew.bat assembleRelease -x lint
) else (
    call gradlew.bat assembleDebug -x lint
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo Build FAILED!
    echo ========================================
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================
echo Build SUCCESS!
echo ========================================
echo.
echo Native library location:
if "%BUILD_TYPE%"=="release" (
    echo   app\build\intermediates\cmake\release\obj\
) else (
    echo   app\build\intermediates\cmake\debug\obj\
)
echo.
pause
