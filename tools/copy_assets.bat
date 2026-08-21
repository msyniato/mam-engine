@echo off
cd %~dp0..
setlocal enabledelayedexpansion

set "SOURCE=.\assets"
set "TARGET_ROOT=.\build"

for /d %%D in ("%TARGET_ROOT%\*") do (
    xcopy /e /i /y "%SOURCE%" "%%D\assets\"
)