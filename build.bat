@echo off
REM ============================================================================
REM  Modern Edirol SD-80
REM  One-click Windows build. One log: logs\build.log
REM
REM  Double-click this file, or from a terminal:
REM      build.bat
REM      build.bat nopause
REM ============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

chcp 65001 >nul 2>&1

set "ROOT=%cd%"
set "LOGDIR=%ROOT%\logs"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"
set "LOG=%LOGDIR%\build.log"

if exist "%LOG%" del /q "%LOG%" >nul 2>&1

call :banner
call :log "==============================================================="
call :log "  Modern Edirol SD-80"
call :log "  Crimson Redstone  -  freeware"
call :log "  Started: %DATE% %TIME%"
call :log "  Folder : %ROOT%"
call :log "  Log    : %LOG%"
call :log "==============================================================="
call :log ""
call :log "  AU is macOS / Xcode only. This Windows build: VST3 + Standalone, then CLAP."
call :log "  JUCE 9.0.1. If you previously configured JUCE 8, delete the build folder first."
call :log ""

REM ---- tools -----------------------------------------------------------------
call :log "[1/5] Checking tools..."
where cmake >nul 2>&1
if errorlevel 1 (
    call :fail "CMake is not on PATH. Install https://cmake.org/download/ and tick 'Add CMake to the system PATH'."
    goto :end
)
for /f "delims=" %%V in ('cmake --version 2^>^&1') do (
    call :log "  %%V"
    goto :after_cmake_ver
)
:after_cmake_ver
call :log "  git:"
where git >nul 2>&1
if errorlevel 1 (
    call :log "  WARNING: git not found. First configure will fail because JUCE is fetched via git."
    call :log "  Install Git from https://git-scm.com/download/win and re-run."
) else (
    for /f "delims=" %%V in ('git --version 2^>^&1') do call :log "  %%V"
)

call :log ""
call :log "  Compiler / generator probe:"
if defined VSINSTALLDIR call :log "  VSINSTALLDIR=%VSINSTALLDIR%"
where cl >nul 2>&1
if errorlevel 1 (
    call :log "  cl.exe not on PATH (normal if you did not open an x64 Native Tools prompt)."
    call :log "  CMake will try the Visual Studio generator instead."
) else (
    for /f "delims=" %%V in ('cl 2^>^&1') do (
        call :log "  %%V"
        goto :after_cl
    )
)
:after_cl

REM ---- configure -------------------------------------------------------------
call :log ""
call :log "[2/5] Configuring CMake (first run downloads JUCE 9.0.1, a few minutes)..."

cmake -G "Visual Studio 17 2022" -A x64 -B "%ROOT%\build" -S "%ROOT%" >> "%LOG%" 2>&1
if errorlevel 1 (
    call :log "  VS 2022 generator failed — retrying with CMake's default generator..."
    cmake -B "%ROOT%\build" -S "%ROOT%" -DCMAKE_BUILD_TYPE=Release >> "%LOG%" 2>&1
    if errorlevel 1 (
        call :fail "CMake configure failed. Scroll the log for the first error (often: git missing, no compiler, or network blocked fetching JUCE)."
        goto :end
    )
) else (
    call :log "  Configured with Visual Studio 17 2022 x64."
)

REM ---- build VST3 + Standalone ----------------------------------------------
call :log ""
call :log "[3/5] Compiling VST3 + Standalone (first build compiles JUCE too — go make tea)..."
cmake --build "%ROOT%\build" --config Release --parallel --target ModernEdirolSD80_VST3 ModernEdirolSD80_Standalone >> "%LOG%" 2>&1
if errorlevel 1 (
    call :fail "Compile failed. Search the log for 'error C' or 'error:' — that is the line to paste."
    goto :end
)

REM ---- CLAP (best-effort) ----------------------------------------------------
call :log ""
call :log "[4/5] Compiling CLAP (optional — VST3 and Standalone already succeeded)..."
cmake --build "%ROOT%\build" --config Release --parallel --target ModernEdirolSD80_CLAP >> "%LOG%" 2>&1
if errorlevel 1 (
    call :log "  CLAP was not built. VST3 and Standalone are still good."
    call :log "  If you do not need CLAP, reconfigure with -DMESD80_CLAP=OFF"
    set "CLAP_OK=0"
) else (
    call :log "  CLAP succeeded."
    set "CLAP_OK=1"
)

REM ---- locate artefacts ------------------------------------------------------
call :log ""
call :log "[5/5] Locating plugin files..."
set "ART="
for /d %%D in ("%ROOT%\build\ModernEdirolSD80_artefacts*") do set "ART=%%~fD"
if not defined ART (
    for /d %%D in ("%ROOT%\build\*\ModernEdirolSD80_artefacts*") do set "ART=%%~fD"
)

if not defined ART (
    call :log "  Built, but could not find ModernEdirolSD80_artefacts. Dumping build\ :"
    dir /s /b "%ROOT%\build\*.vst3" "%ROOT%\build\*.exe" "%ROOT%\build\*.clap" >> "%LOG%" 2>&1
) else (
    call :log "  Artefacts: %ART%"
    if exist "%ART%\Release" (
        dir /s /b "%ART%\Release" >> "%LOG%" 2>&1
    ) else (
        dir /s /b "%ART%" >> "%LOG%" 2>&1
    )
)

call :log ""
call :log "SUCCESS"
call :log "  Copy the .vst3 folder to:  C:\Program Files\Common Files\VST3"
call :log "  Standalone .exe is next to it under Standalone\"
if "%CLAP_OK%"=="1" call :log "  CLAP is under CLAP\"
call :log "  Log: %LOG%"
call :log "==============================================================="

echo.
echo  ============================================================
echo   BUILD OK   —  Modern Edirol SD-80
echo   Log:      %LOG%
if defined ART echo   Output:   %ART%
echo  ============================================================
echo.
set "ERR=0"
goto :end

REM ---- helpers ---------------------------------------------------------------
:banner
echo.
echo   Modern Edirol SD-80  —  one-click build
echo   Log: logs\build.log
echo.
goto :eof

:log
echo %~1
echo %~1>> "%LOG%"
goto :eof

:fail
echo.
echo  ============================================================
echo   BUILD FAILED
echo   %~1
echo   Paste THIS FILE:  logs\build.log
echo  ============================================================
echo.
call :log ""
call :log "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
call :log "FAILED: %~1"
call :log "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
set "ERR=1"
exit /b 1

:end
if not defined ERR set "ERR=%ERRORLEVEL%"
if /I not "%~1"=="nopause" if /I not "%~1"=="--nopause" (
    echo.
    pause
)
endlocal & exit /b %ERR%
