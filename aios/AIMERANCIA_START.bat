@echo off
title AIMERANCIA
color 0A
cd /d C:\Users\ADMIN\aios\aios
echo [1/3] Launching AIMERANCIA kernel...
start "AIMERANCIA" "C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom aios.iso -drive file=disk.img,format=raw,if=ide,index=1 -no-reboot -netdev user,id=net0 -device rtl8139,netdev=net0 -vga std -display sdl,window-close=off
echo [2/3] Waiting for boot...
timeout /t 4 /nobreak >nul
echo [3/3] Starting voice bridge...
python aimerancia_voice.py
pause
