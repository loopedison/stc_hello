@echo off
cd /d "%~dp0"
if exist "Listings" del /f /q /s "Listings\*.*" >nul 2>nul
if exist "Objects" (
    for %%f in ("Objects\*.*") do if /i not "%%~xf"==".hex" del /f /q "%%f" >nul 2>nul
    for /d %%d in ("Objects\*") do rd /s /q "%%d" >nul 2>nul
)
del /f /q "*.uvgui.*" >nul 2>nul
exit