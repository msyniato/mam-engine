@echo off
setlocal EnableDelayedExpansion

pushd "%~dp0.."
set "ROOT=%cd%"
popd

set "SOURCE=%ROOT%\assets"
set "TARGET_ROOT=%ROOT%\build"

for /d %%D in ("%TARGET_ROOT%\*") do (
    if exist "%SOURCE%" (
        xcopy /e /i /y "%SOURCE%" "%%D\assets\" >nul
    )
)
