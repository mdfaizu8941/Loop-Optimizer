@echo off
setlocal

cd /d "%~dp0"

if exist ".venv\Scripts\python.exe" (
	".venv\Scripts\python.exe" run_optimizer_visualizer.py --port 8515
) else (
	python run_optimizer_visualizer.py --port 8515
)

endlocal
