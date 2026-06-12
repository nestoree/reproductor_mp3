@echo off
title Reproductor App - Instalador
echo ============================================
echo  Reproductor App - Instalador para Windows
echo ============================================
echo.
echo Este instalador configura MSYS2, compila
echo la aplicacion y la ejecuta.
echo Si aparece UAC, haga clic en SI.
echo.
pause
powershell -ExecutionPolicy Bypass -NoProfile -File "%~dp0setup.ps1"
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Revise el mensaje en la ventana anterior.
    echo Codigo: %errorlevel%
    pause
    exit /b %errorlevel%
)
echo.
echo Listo. El reproductor deberia estar ejecutandose.
pause
