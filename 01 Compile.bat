@echo off
setlocal
cd /d "%~dp0"

set "LOGFILE=build_log.txt"

echo Building Modern Edirol SD-80... > "%LOGFILE%"
echo %date% %time% >> "%LOGFILE%"
echo ---------------------------------------- >> "%LOGFILE%"
echo AU is macOS-only ^(Xcode^). This Windows build: VST3 + Standalone, then CLAP. >> "%LOGFILE%"
echo JUCE 9.0.1. If you previously configured JUCE 8, delete the build folder first so CMake re-fetches. >> "%LOGFILE%"
echo ---------------------------------------- >> "%LOGFILE%"

cmake -B build -G "Visual Studio 17 2022" -A x64 >> "%LOGFILE%" 2>&1
if errorlevel 1 goto :fail

echo Building VST3 + Standalone... >> "%LOGFILE%"
cmake --build build --config Release --target ModernEdirolSD80_VST3 ModernEdirolSD80_Standalone >> "%LOGFILE%" 2>&1
if errorlevel 1 goto :fail

echo Building CLAP... >> "%LOGFILE%"
cmake --build build --config Release --target ModernEdirolSD80_CLAP >> "%LOGFILE%" 2>&1
if errorlevel 1 (
  echo CLAP was not built. VST3 and Standalone succeeded. >> "%LOGFILE%"
  echo If you do not need CLAP: cmake -B build -G "Visual Studio 17 2022" -A x64 -DMESD80_CLAP=OFF >> "%LOGFILE%"
) else (
  echo CLAP succeeded. >> "%LOGFILE%"
)

echo ---------------------------------------- >> "%LOGFILE%"
echo Build Succeeded! >> "%LOGFILE%"
echo Artefacts: build\ModernEdirolSD80_artefacts\Release\ >> "%LOGFILE%"
exit /b 0

:fail
echo ---------------------------------------- >> "%LOGFILE%"
echo Build Failed with error code %errorlevel%. >> "%LOGFILE%"
exit /b %errorlevel%
