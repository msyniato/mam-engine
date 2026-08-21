@echo off
pushd "%~dp0.."
set "ROOT=%cd%"
popd
setlocal enabledelayedexpansion

echo ========================================
echo Instalando dependencias con conan...
echo Raiz: %ROOT%
echo ========================================
echo.

where conan >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] No se encontro conan. Instala con: pip install conan
    pause
    exit /b 1
)

for /f "tokens=3" %%V in ('conan --version 2^>^&1') do set CONAN_VER=%%V
echo Conan: v%CONAN_VER%
echo.

if not exist "%ROOT%\conan" (
    echo [ERROR] No se encontro la carpeta conan\ en %ROOT%
    pause
    exit /b 1
)

echo [1/3] Release...
conan install -if "%ROOT%\build\deps\Release" -s build_type=Release -s compiler="Visual Studio" -s compiler.runtime=MD --build=missing "%ROOT%\conan"
if errorlevel 1 goto :error

echo [2/3] RelWithDebInfo...
conan install -if "%ROOT%\build\deps\RelWithDebInfo" -s build_type=RelWithDebInfo -s compiler="Visual Studio" -s compiler.runtime=MD --build=missing "%ROOT%\conan"
if errorlevel 1 goto :error

echo [3/3] Debug...
conan install -if "%ROOT%\build\deps\Debug" -s build_type=Debug -s compiler="Visual Studio" -s compiler.runtime=MDd --build=missing "%ROOT%\conan"
if errorlevel 1 goto :error

echo.
echo ========================================
echo Dependencias instaladas correctamente
echo ========================================
exit /b 0

:error
echo.
echo ========================================
echo ERROR: Fallo conan install
echo ========================================
pause
exit /b 1
