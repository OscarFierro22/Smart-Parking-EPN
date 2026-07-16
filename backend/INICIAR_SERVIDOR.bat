@echo off
setlocal
cd /d "%~dp0"

where python >nul 2>nul
if errorlevel 1 (
    echo ERROR: Python no esta instalado o no esta agregado al PATH.
    pause
    exit /b 1
)

if not exist .venv\Scripts\python.exe (
    echo Creando entorno virtual...
    python -m venv .venv
)

call .venv\Scripts\activate.bat
python -m pip install --upgrade pip
python -m pip install -r requirements.txt

echo.
echo ============================================================
echo SMART PARKING API: http://0.0.0.0:8000
echo Prueba en esta PC: http://127.0.0.1:8000/api/health
echo La app puede encontrar esta computadora automaticamente por Wi-Fi.
echo Descubrimiento UDP: puerto 40404
echo Ejecuta CONFIGURAR_WIFI_ADMIN.bat una sola vez como administrador.
echo ============================================================
echo.
python -m uvicorn server:app --host 0.0.0.0 --port 8000
pause
