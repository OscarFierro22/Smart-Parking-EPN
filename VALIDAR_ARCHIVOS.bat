@echo off
setlocal
cd /d "%~dp0"

echo Verificando archivos principales...
set "ERRORS=0"

for %%F in (
    "OpenGL.sln"
    "OpenGL\Proyecto_Final.cpp"
    "OpenGL\Vehicle\Vehicle.cpp"
    "OpenGL\model\Vehicle\Body.obj"
    "OpenGL\model\night_sky\night_sky.obj"
    "backend\server.py"
    "smart_parking_app\pubspec.yaml"
    "smart_parking_app\lib\main.dart"
) do (
    if not exist %%F (
        echo FALTA: %%F
        set /a ERRORS+=1
    )
)

if "%ERRORS%"=="0" (
    echo Proyecto completo: archivos principales encontrados.
) else (
    echo Se encontraron %ERRORS% archivos faltantes.
)
pause
