@echo off
setlocal EnableDelayedExpansion

:: Resolver ruta raiz correctamente (este .bat esta en tools\)
pushd "%~dp0.."
set "ROOT=%cd%"
popd
set "STAGE_DIR=%ROOT%\MAMEngine"

echo ========================================
echo Raiz detectada: %ROOT%
echo Stage: %STAGE_DIR%
echo ========================================

:: -------------------------------------------------------
:: [1/5] Visual Studio
:: -------------------------------------------------------
echo [1/5] Cargando entorno de Visual Studio...
where msbuild >nul 2>nul
if %errorlevel% neq 0 (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
)

:: -------------------------------------------------------
:: [2/5] Compilar mam_engine en las 3 configs
:: -------------------------------------------------------
echo [2/5] Compilando mam_engine...
set "ENGINE_PROJECT=%ROOT%\build\mam_engine.vcxproj"

if not exist "%ENGINE_PROJECT%" (
    echo ERROR: No se encontro "%ENGINE_PROJECT%"
    echo Ejecuta primero tools\gen_all.bat para generar la solucion.
    pause
    exit /b 1
)

msbuild "%ENGINE_PROJECT%" /p:Configuration=Debug         /p:Platform=x64 /p:UseMultiToolTask=false /p:CL_MPCount=1
if errorlevel 1 goto :build_error

msbuild "%ENGINE_PROJECT%" /p:Configuration=Release        /p:Platform=x64 /p:UseMultiToolTask=false /p:CL_MPCount=1
if errorlevel 1 goto :build_error

msbuild "%ENGINE_PROJECT%" /p:Configuration=RelWithDebInfo /p:Platform=x64 /p:UseMultiToolTask=false /p:CL_MPCount=1
if errorlevel 1 goto :build_error

:: -------------------------------------------------------
:: [3/5] Recrear stage limpio
:: -------------------------------------------------------
echo [3/5] Recreando stage...
if exist "%STAGE_DIR%" rd /s /q "%STAGE_DIR%"

mkdir "%STAGE_DIR%"
mkdir "%STAGE_DIR%\assets"
mkdir "%STAGE_DIR%\doc"
mkdir "%STAGE_DIR%\examples"
mkdir "%STAGE_DIR%\game"
mkdir "%STAGE_DIR%\mam"
mkdir "%STAGE_DIR%\mam\include"
mkdir "%STAGE_DIR%\mam\include\mam"
mkdir "%STAGE_DIR%\mam\include\mam\audiosys"
mkdir "%STAGE_DIR%\mam\include\mam\common"
mkdir "%STAGE_DIR%\mam\include\mam\core"
mkdir "%STAGE_DIR%\mam\include\mam\ecs"
mkdir "%STAGE_DIR%\mam\include\mam\jobsys"
mkdir "%STAGE_DIR%\mam\include\mam\matsys"
mkdir "%STAGE_DIR%\mam\include\mam\render"
mkdir "%STAGE_DIR%\mam\include\mam\render\api"
mkdir "%STAGE_DIR%\mam\include\mam\scriptsys"
mkdir "%STAGE_DIR%\mam\include\mam\ui"
mkdir "%STAGE_DIR%\mam\vendors"
mkdir "%STAGE_DIR%\lib"
mkdir "%STAGE_DIR%\lib\Debug"
mkdir "%STAGE_DIR%\lib\Release"
mkdir "%STAGE_DIR%\lib\RelWithDebInfo"
mkdir "%STAGE_DIR%\lib\vendors"
mkdir "%STAGE_DIR%\lib\vendors\openal_soft"
mkdir "%STAGE_DIR%\lib\vendors\openal_soft\libs"
mkdir "%STAGE_DIR%\tools"

:: -------------------------------------------------------
:: [4/5] Copiar archivos
:: -------------------------------------------------------
echo [4/5] Copiando archivos...

:: Assets y doc
if exist "%ROOT%\assets" xcopy "%ROOT%\assets" "%STAGE_DIR%\assets\" /s /e /i /y >nul
if exist "%ROOT%\doc"    xcopy "%ROOT%\doc"    "%STAGE_DIR%\doc\"    /s /e /i /y >nul
echo        assets y doc OK

:: Solo premake5.exe de tools (NO los .bat del motor)
if exist "%ROOT%\tools\premake5.exe" (
    copy "%ROOT%\tools\premake5.exe" "%STAGE_DIR%\tools\premake5.exe" /y >nul
    echo        premake5.exe OK
) else (
    echo [AVISO] No se encontro tools\premake5.exe
)

:: .bat adaptados al stage (desde tools\dist\ - NO los del motor)
if not exist "%ROOT%\tools\dist\" (
    echo [ERROR] No se encontro la carpeta tools\dist\
    pause
    exit /b 1
)
if exist "%ROOT%\tools\dist\setup.bat"        ( copy "%ROOT%\tools\dist\setup.bat"        "%STAGE_DIR%\setup.bat"               /y >nul ) else ( echo [AVISO] No se encontro tools\dist\setup.bat )
if exist "%ROOT%\tools\dist\gen_all.bat"      ( copy "%ROOT%\tools\dist\gen_all.bat"      "%STAGE_DIR%\tools\gen_all.bat"       /y >nul ) else ( echo [AVISO] No se encontro tools\dist\gen_all.bat )
if exist "%ROOT%\tools\dist\gen_deps.bat"     ( copy "%ROOT%\tools\dist\gen_deps.bat"     "%STAGE_DIR%\tools\gen_deps.bat"      /y >nul ) else ( echo [AVISO] No se encontro tools\dist\gen_deps.bat )
if exist "%ROOT%\tools\dist\gen_solution.bat" ( copy "%ROOT%\tools\dist\gen_solution.bat" "%STAGE_DIR%\tools\gen_solution.bat"  /y >nul ) else ( echo [AVISO] No se encontro tools\dist\gen_solution.bat )
if exist "%ROOT%\tools\dist\copy_assets.bat"  ( copy "%ROOT%\tools\dist\copy_assets.bat"  "%STAGE_DIR%\tools\copy_assets.bat"   /y >nul ) else ( echo [AVISO] No se encontro tools\dist\copy_assets.bat )
echo        .bat del stage OK

:: Cabeceras publicas del motor
:: NOTA: se copian bajo mam\include\mam\ para que #include "mam/subsystem/header.hpp" resuelva
:: correctamente apuntando al directorio mam\include\ como include path.
if exist "%ROOT%\mam\include\mam\audiosys"   xcopy "%ROOT%\mam\include\mam\audiosys"   "%STAGE_DIR%\mam\include\mam\audiosys\"   /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\common"     xcopy "%ROOT%\mam\include\mam\common"     "%STAGE_DIR%\mam\include\mam\common\"     /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\core"       xcopy "%ROOT%\mam\include\mam\core"       "%STAGE_DIR%\mam\include\mam\core\"       /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\ecs"        xcopy "%ROOT%\mam\include\mam\ecs"        "%STAGE_DIR%\mam\include\mam\ecs\"        /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\jobsys"     xcopy "%ROOT%\mam\include\mam\jobsys"     "%STAGE_DIR%\mam\include\mam\jobsys\"     /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\matsys"     xcopy "%ROOT%\mam\include\mam\matsys"     "%STAGE_DIR%\mam\include\mam\matsys\"     /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\render\api" xcopy "%ROOT%\mam\include\mam\render\api" "%STAGE_DIR%\mam\include\mam\render\api\" /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\scriptsys"  xcopy "%ROOT%\mam\include\mam\scriptsys"  "%STAGE_DIR%\mam\include\mam\scriptsys\"  /s /e /i /y >nul
if exist "%ROOT%\mam\include\mam\ui"         xcopy "%ROOT%\mam\include\mam\ui"         "%STAGE_DIR%\mam\include\mam\ui\"         /s /e /i /y >nul
echo        cabeceras del motor OK

:: Cabeceras de vendors
if exist "%ROOT%\mam\vendors\stb_image"           xcopy "%ROOT%\mam\vendors\stb_image"           "%STAGE_DIR%\mam\vendors\stb_image\"           /s /e /i /y >nul
if exist "%ROOT%\mam\vendors\stb_vorbis"          xcopy "%ROOT%\mam\vendors\stb_vorbis"          "%STAGE_DIR%\mam\vendors\stb_vorbis\"          /s /e /i /y >nul
if exist "%ROOT%\mam\vendors\openal_soft\include" xcopy "%ROOT%\mam\vendors\openal_soft\include" "%STAGE_DIR%\mam\vendors\openal_soft\include\" /s /e /i /y >nul
if exist "%ROOT%\mam\vendors\imgui"               xcopy "%ROOT%\mam\vendors\imgui"               "%STAGE_DIR%\mam\vendors\imgui\"               /s /e /i /y >nul
if exist "%ROOT%\mam\vendors\dr"                  xcopy "%ROOT%\mam\vendors\dr"                  "%STAGE_DIR%\mam\vendors\dr\"                  /s /e /i /y >nul
if exist "%ROOT%\mam\vendors\lua\src"             xcopy "%ROOT%\mam\vendors\lua\src"             "%STAGE_DIR%\mam\vendors\lua\src\"             /s /e /i /y >nul
if exist "%ROOT%\mam\vendors\sol2\include"        xcopy "%ROOT%\mam\vendors\sol2\include"        "%STAGE_DIR%\mam\vendors\sol2\include\"        /s /e /i /y >nul
if exist "%ROOT%\mam\vendors\Perlin"              xcopy "%ROOT%\mam\vendors\Perlin"              "%STAGE_DIR%\mam\vendors\Perlin\"              /s /e /i /y >nul
echo        cabeceras de vendors OK

:: Libs de vendors precompiladas
if exist "%ROOT%\mam\vendors\openal_soft\libs" (
    xcopy "%ROOT%\mam\vendors\openal_soft\libs" "%STAGE_DIR%\lib\vendors\openal_soft\libs\" /s /e /i /y >nul
    echo        openal libs OK
) else (
    echo [AVISO] No se encontraron libs de openal_soft
)

:: Libs compiladas del motor
for %%C in (Debug Release RelWithDebInfo) do (
    if exist "%ROOT%\build\mam_engine\%%C\mam_engine.lib" (
        copy "%ROOT%\build\mam_engine\%%C\mam_engine.lib" "%STAGE_DIR%\lib\%%C\" /y >nul
        echo        lib\%%C\mam_engine.lib OK
    ) else (
        echo [AVISO] No encontrada: build\mam_engine\%%C\mam_engine.lib
    )
)

:: Carpeta conan source (engine/src/build/ con conanfile.txt)
if exist "%ROOT%\mam\src\build" (
    xcopy "%ROOT%\mam\src\build" "%STAGE_DIR%\conan\" /s /e /i /y >nul
    echo        conan\ OK  ^(desde mam\src\build\^)
) else (
    echo [AVISO] No se encontro mam\src\build\
)

:: Workspaces de ejemplo
if exist "%ROOT%\workspaces" (
    xcopy "%ROOT%\workspaces" "%STAGE_DIR%\examples\" /s /e /i /y >nul
    echo        examples OK
)

:: main.cpp de plantilla para el usuario (desde game\ en la raiz del proyecto)
if exist "%ROOT%\game\main.cpp" (
    copy "%ROOT%\game\main.cpp" "%STAGE_DIR%\game\main.cpp" /y >nul
    echo        game\main.cpp OK
) else (
    echo [AVISO] No se encontro game\main.cpp en la raiz del proyecto
)

:: premake5.lua del SDK
if exist "%ROOT%\premake5_sdk.lua" (
    copy "%ROOT%\premake5_sdk.lua" "%STAGE_DIR%\premake5.lua" /y >nul
    echo        premake5.lua OK
) else (
    echo [ERROR] No se encontro premake5_sdk.lua en la raiz
    pause
    exit /b 1
)

:: -------------------------------------------------------
:: [5/5] Listo
:: -------------------------------------------------------
echo [5/5] Stage listo. El usuario debe ejecutar setup.bat
echo.
echo ========================================
echo STAGE SDK GENERADO CORRECTAMENTE
echo %STAGE_DIR%
echo ========================================
pause
exit /b 0

:build_error
echo.
echo ========================================
echo ERROR: Fallo la compilacion de mam_engine
echo ========================================
pause
exit /b 1