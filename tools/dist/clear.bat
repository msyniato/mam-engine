@echo off
pushd "%~dp0.."
set "ROOT=%cd%"
popd
setlocal enabledelayedexpansion

cls
echo ========================================
echo MAMEngine SDK - Limpiando build...
echo ========================================
echo.

:: Borrar .vs
IF EXIST "%ROOT%\build\.vs" rmdir /s /q "%ROOT%\build\.vs"

:: Borrar subcarpetas de build EXCEPTO deps (conanbuildinfo generados)
FOR /D %%i IN ("%ROOT%\build\*") DO (
    IF /I NOT "%%~nxi"=="deps" rmdir /s /q "%%i"
)

:: Borrar archivos sueltos de build
FOR %%i IN ("%ROOT%\build\*") DO (
    del /q "%%i"
)

:: NO tocar build\deps\ ni conan\ (archivos de conan)
echo Listo - build\deps\ y conan\ conservados
echo.
