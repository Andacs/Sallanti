@echo off
rem Editor KAPALI olmali. Teslim konfigurasyonu: Shipping. Cikti: Build\Package\Windows\Sallanti.exe
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:/Dev/Sallanti/Sallanti.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="C:/Dev/Sallanti/Build/Package" -utf8output -unattended -ubtargs="-NoXGE -MaxParallelActions=2"
