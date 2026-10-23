@echo off
rem Editor KAPALI olmali. Teslim konfigurasyonu: Shipping. Cikti: Build\Package\Windows\Sallanti.exe
rem Istege bagli 1. arguman: cikti klasoru (Tools/otomatik_build.sh kendi klasorunu verir).
set "ROOT=%~dp0.."
set "OUT=%~1"
if "%OUT%"=="" set "OUT=%ROOT%\Build\Package"
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%ROOT%\Sallanti.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="%OUT%" -utf8output -unattended -ubtargs="-NoXGE -MaxParallelActions=2"
