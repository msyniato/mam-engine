@echo off
setlocal EnableDelayedExpansion

pushd "%~dp0.."
set "ROOT=%cd%"
popd

echo ========================================
echo Generando solucion Visual Studio...
echo Raiz: %ROOT%
echo ========================================
echo.

if not exist "%ROOT%\premake5.lua" (
    echo [ERROR] No se encontro premake5.lua en %ROOT%
    pause
    exit /b 1
)

if not exist "%ROOT%\tools\premake5.exe" (
    echo [ERROR] No se encontro tools\premake5.exe
    pause
    exit /b 1
)

"%ROOT%\tools\premake5.exe" --file="%ROOT%\premake5.lua" vs2022
if errorlevel 1 (
    echo [ERROR] Fallo la generacion de la solucion
    pause
    exit /b 1
)

echo.
echo ========================================
echo Solucion generada: build\MAMEngineSDK.sln
echo ========================================
exit /b 0
