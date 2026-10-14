@echo off
rem L_Test'i editorsuz oyun penceresinde acar (derleme gerekmez, derle.bat yeterli).
rem Kontroller: 1-4 slot, fare hayalet, sol tik yerlestir, R dondur, sag tik/Esc iptal.
rem Konsol (~): SLSelfTest = otomatik 1-6 olcumu, SLReport = egim/CoM loga.
set "ROOT=%~dp0.."
start "" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%ROOT%\Sallanti.uproject" /Game/Sallanti/Maps/L_Test -game -windowed -ResX=1600 -ResY=900 -log
