@echo off
setlocal EnableDelayedExpansion

pushd "%~dp0.."
set "ROOT=%cd%"
popd

echo ========================================
echo MAMEngine SDK - Generacion completa
echo ========================================
echo.

call "%ROOT%\tools\gen_deps.bat"
if errorlevel 1 (
    echo [ERROR] Fallo gen_deps.bat
    pause
    exit /b 1
)

call "%ROOT%\tools\gen_solution.bat"
if errorlevel 1 (
    echo [ERROR] Fallo gen_solution.bat
    pause
    exit /b 1
)

echo.
echo ========================================
echo LISTO - Abre build\MAMEngineSDK.sln
echo ========================================
pause
