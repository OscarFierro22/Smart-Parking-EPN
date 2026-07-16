@echo off
setlocal
cd /d "%~dp0"

echo Dispositivos disponibles:
flutter devices
echo.
set /p DEVICE=Escribe el ID del telefono que muestra Flutter: 
if "%DEVICE%"=="" (
    flutter run
) else (
    flutter run -d %DEVICE%
)
pause
