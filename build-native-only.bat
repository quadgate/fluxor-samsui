@echo off
REM Build script for native C++ library only (faster build)
REM This script builds only the native library using Gradle's native tasks

echo ========================================
echo Building Native C++ Library Only
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

:parse_args
if "%~1"=="" goto :build
if /i "%~1"=="clean" set CLEAN=true
if /i "%~1"=="release" set BUILD_TYPE=release
if /i "%~1"=="debug" set BUILD_TYPE=debug
shift
goto :parse_args

:build
echo Build Type: %BUILD_TYPE%
echo.

REM Clean if requested
if "%CLEAN%"=="true" (
    echo Cleaning CMake build files...
    call gradlew.bat clean
    echo.
)

REM Build only native library (faster than full build)
echo Building native library only (%BUILD_TYPE%)...
echo.

if "%BUILD_TYPE%"=="release" (
    call gradlew.bat :app:externalNativeBuildRelease
) else (
    call gradlew.bat :app:externalNativeBuildDebug
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
