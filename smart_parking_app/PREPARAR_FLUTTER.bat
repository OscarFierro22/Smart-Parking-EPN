@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

where flutter >nul 2>nul
if errorlevel 1 (
    echo ERROR: Flutter no esta instalado o no esta agregado al PATH.
    pause
    exit /b 1
)

set "BACKUP=%TEMP%\smart_parking_flutter_%RANDOM%"
mkdir "%BACKUP%"
xcopy /E /I /Y lib "%BACKUP%\lib" >nul
copy /Y pubspec.yaml "%BACKUP%\pubspec.yaml" >nul

if exist android rmdir /S /Q android
flutter create --platforms=android --org ec.edu.epn.smartparking --project-name smart_parking_app .
if errorlevel 1 (
    echo ERROR: Flutter no pudo generar la plataforma Android moderna.
    pause
    exit /b 1
)

rmdir /S /Q lib
xcopy /E /I /Y "%BACKUP%\lib" lib >nul
copy /Y "%BACKUP%\pubspec.yaml" pubspec.yaml >nul
copy /Y android_overlay\app\src\main\AndroidManifest.xml android\app\src\main\AndroidManifest.xml >nul
rmdir /S /Q "%BACKUP%"

flutter clean
flutter pub get
flutter analyze

echo.
echo Proyecto Android preparado con Flutter embedding v2.
echo Conecta el celular y ejecuta EJECUTAR_FLUTTER.bat.
pause
