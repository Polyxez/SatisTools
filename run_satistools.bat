@echo off
chcp 65001 > nul
title SatisTools - Интерактивная карта Satisfactory
echo =======================================================
echo   Запуск SatisTools: Интерактивная карта и аналитика
echo =======================================================
cd /d "%~dp0"
python serve.py
pause
