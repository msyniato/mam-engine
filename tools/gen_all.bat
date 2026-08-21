@echo off
cd %~dp0..
setlocal enabledelayedexpansion

call tools\gen_deps.bat
call tools\gen_solution.bat
