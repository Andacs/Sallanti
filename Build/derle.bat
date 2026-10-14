@echo off
rem Editor KAPALI olmali (Live Coding kilidi). -NoXGE -MaxParallelActions=2 bu makinede sart.
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" SallantiEditor Win64 Development -project="C:/Dev/Sallanti/Sallanti.uproject" -waitmutex -NoXGE -MaxParallelActions=2
