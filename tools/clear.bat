@echo off
cd %~dp0..
setlocal enabledelayedexpansion

set "TARGET=.\build"

rem Delete all folders inside build except deps
for /d %%D in ("%TARGET%\*") do (
    if /I not "%%~nxD"=="deps" (
        rmdir /s /q "%%D"
    )
)

rem Delete all files inside build
for %%F in ("%TARGET%\*") do (
    if not "%%~nxF"=="deps" (
        del /q "%%F"
    )
)