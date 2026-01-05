@echo off
setlocal
cd /d "%~dp0"

REM 让 Windows loader 优先从 bin 找 DLL（可选但建议）
set "PATH=%~dp0\..\..;%PATH%"

..\..\QuoteGenerator.exe %*
