@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

echo === Building RegExJit (MSVC) ===
call .\build_msvc.cmd
if %errorlevel% neq 0 (
    echo Build failed. Aborting benchmarks.
    exit /b %errorlevel%
)

echo.
echo === Running Benchmarks (MSVC) ===
.\build\benchmarks\Release\regexjit_benchmark.exe %*
if %errorlevel% neq 0 (
    echo Benchmarks failed.
    exit /b %errorlevel%
)

echo.
echo Benchmarks completed successfully!
