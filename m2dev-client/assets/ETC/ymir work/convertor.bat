@echo off
title MSENV TO JSON

if not exist *.msenv (
    echo NU EXISTA fisiere .msenv in folder.
    pause
    exit
)

python "%~dp0msev_to_json.py"

echo.
echo GATA.
pause