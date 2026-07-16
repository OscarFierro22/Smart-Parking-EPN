@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

where flutter >nul 2>nul
if errorlevel 1 (
    echo ERROR: Flutter no esta instalado o no esta agregado al PATH.
    echo Instala Flutter y ejecuta nuevamente este archivo.
    pause
    exit /b 1
)

set "BACKUP=%TEMP%\smart_parking_flutter_%RANDOM%"
mkdir "%BACKUP%"
xcopy /E /I /Y lib "%BACKUP%\lib" >nul
copy /Y pubspec.yaml "%BACKUP%\pubspec.yaml" >nul

flutter create . --project-name smart_parking_app --org ec.edu.epn.smartparking --platforms=android,windows,web
if errorlevel 1 (
    echo ERROR: flutter create no pudo completar el proyecto.
    pause
    exit /b 1
)

rmdir /S /Q lib
xcopy /E /I /Y "%BACKUP%\lib" lib >nul
copy /Y "%BACKUP%\pubspec.yaml" pubspec.yaml >nul
copy /Y android_overlay\app\src\main\AndroidManifest.xml android\app\src\main\AndroidManifest.xml >nul
rmdir /S /Q "%BACKUP%"

flutter pub get

echo.
echo Proyecto Flutter preparado correctamente.
echo Para ejecutarlo conecta el celular y usa: flutter run
pause
