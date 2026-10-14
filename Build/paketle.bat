@echo off
rem Editor KAPALI olmali. Teslim konfigurasyonu: Shipping. Cikti: Build\Package\Windows\Sallanti.exe
set "ROOT=%~dp0.."
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%ROOT%\Sallanti.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="%ROOT%\Build\Package" -utf8output -unattended -ubtargs="-NoXGE -MaxParallelActions=2"
