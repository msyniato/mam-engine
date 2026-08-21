@echo off
cd %~dp0..
setlocal enabledelayedexpansion

tools\premake5.exe --file=premake5_root.lua vs2022