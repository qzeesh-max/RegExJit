@echo off
setlocal enabledelayedexpansion

:: Remove MSYS64 from PATH to prevent it from interfering with MSVC builds
set PATH=%PATH:C:\msys64\ucrt64\bin=%
set PATH=%PATH:C:\msys64\usr\bin=%
set PATH=%PATH:C:\msys64\mingw64\bin=%
set PATH=%PATH:C:\msys64\mingw32\bin=%

cd /d "%~dp0"

echo ========================================
echo   Building RegExJit (MSVC)
echo ========================================

where cl.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo Initializing MSVC environment...
    set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!vswhere!" (
        for /f "usebackq tokens=*" %%i in (`"!vswhere!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set "InstallDir=%%i"
        )
        if exist "!InstallDir!\VC\Auxiliary\Build\vcvars64.bat" (
            call "!InstallDir!\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
    where cl.exe >nul 2>nul
    if !errorlevel! neq 0 (
        echo Error: Could not find MSVC compiler. Please run this script from a Visual Studio Developer Command Prompt.
        exit /b 1
    )
)

set BUILD_DIR=build
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

if not exist vcpkg (
    echo Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg.git
    call .\vcpkg\bootstrap-vcpkg.bat
)
echo Installing boost-test via vcpkg...
.\vcpkg\vcpkg install boost-test:x64-windows

cd %BUILD_DIR%

echo Configuring CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_IGNORE_PATH=C:/msys64
if %errorlevel% equ 0 goto build_step

echo CMake configuration failed! Cleaning cache and retrying...
if exist CMakeCache.txt del CMakeCache.txt
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_IGNORE_PATH=C:/msys64
if %errorlevel% neq 0 (
    echo CMake configuration failed again!
    cd ..
    exit /b %errorlevel%
)

:build_step

echo.
echo Building Project...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    exit /b %errorlevel%
)

echo.
echo Build completed successfully!
cd ..
