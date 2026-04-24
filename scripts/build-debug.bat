@echo off
cd /d "%~dp0.."
cmake -B _build -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b %errorlevel%
cmake --build _build --config Debug --parallel
