@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ========================================
echo   Running Tests for RegExJit (MSVC)
echo ========================================

call .\build_msvc.cmd
if %errorlevel% neq 0 (
    echo Build failed. Aborting tests.
    exit /b %errorlevel%
)

echo.
echo ========================================
echo   Executing Tests
echo ========================================
set PATH=C:\msys64\usr\bin;%PATH%
.\build\tests\Release\test_regexjit.exe --log_level=test_suite
if %errorlevel% neq 0 (
    echo Tests failed.
    exit /b %errorlevel%
)

echo.
echo All tests passed successfully!
