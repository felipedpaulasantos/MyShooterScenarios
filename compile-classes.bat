@echo off
echo Compilando classes C++ do projeto MY_SHOOTER...

set UNREAL_ENGINE_PATH=C:\Program Files\Epic Games\UE_5.6
set PROJECT_PATH=%~dp0
set PROJECT_FILE=%PROJECT_PATH%MY_SHOOTER.uproject

"%UNREAL_ENGINE_PATH%\Engine\Build\BatchFiles\Build.bat" LyraGameEditor Win64 Development "%PROJECT_FILE%" -waitmutex

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Compilacao concluida com sucesso!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo ERRO na compilacao! Codigo: %ERRORLEVEL%
    echo ========================================
)

pause

