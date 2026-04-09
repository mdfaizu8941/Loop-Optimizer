@echo off
setlocal

cd /d "%~dp0"
python run_optimizer_visualizer.py --input input.c --output ouput.c

endlocal
