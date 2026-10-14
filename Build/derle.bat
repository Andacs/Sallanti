@echo off
rem Editor KAPALI olmali (Live Coding kilidi). -NoXGE -MaxParallelActions=2 bu makinede sart.
rem Proje yolu bu dosyanin konumundan bulunur; repo nereye klonlanirsa calisir.
set "PROJ=%~dp0..\Sallanti.uproject"
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" SallantiEditor Win64 Development -project="%PROJ%" -waitmutex -NoXGE -MaxParallelActions=2
