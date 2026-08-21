@echo off
cd %~dp0..
setlocal enabledelayedexpansion

conan install -if build/deps/Release -s build_type=Release -s compiler="Visual Studio" -s compiler.runtime=MD --build=missing mam/src/build/
conan install -if build/deps/RelWithDebInfo -s build_type=RelWithDebInfo -s compiler="Visual Studio" -s compiler.runtime=MD --build=missing mam/src/build/
conan install -if build/deps/Debug -s build_type=Debug -s compiler="Visual Studio" -s compiler.runtime=MDd --build=missing mam/src/build/



