@echo off
setlocal

net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: Ejecuta este archivo con clic derecho ^> Ejecutar como administrador.
    pause
    exit /b 1
)

echo Configurando reglas del Firewall de Windows...
netsh advfirewall firewall delete rule name="Smart Parking API TCP 8000" >nul 2>&1
netsh advfirewall firewall delete rule name="Smart Parking Discovery UDP 40404" >nul 2>&1
netsh advfirewall firewall add rule name="Smart Parking API TCP 8000" dir=in action=allow protocol=TCP localport=8000 profile=private
netsh advfirewall firewall add rule name="Smart Parking Discovery UDP 40404" dir=in action=allow protocol=UDP localport=40404 profile=private

echo.
echo Configuracion terminada.
echo TCP 8000: datos HTTP y WebSocket.
echo UDP 40404: deteccion automatica de la computadora.
pause
