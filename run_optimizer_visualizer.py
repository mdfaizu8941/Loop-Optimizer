import argparse
import json
import re
import subprocess
import sys
import time
import webbrowser
from datetime import datetime
from pathlib import Path


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(
		description="Run loop optimizer and open Streamlit comparison dashboard"
	)
	parser.add_argument("--input", default="input.c", help="Path to input source file")
	parser.add_argument("--output", default="ouput.c", help="Path to optimized output file")
	parser.add_argument("--optimizer", default="", help="Path to optimizer executable")
	parser.add_argument("--port", type=int, default=8501, help="Port for Streamlit app")
	parser.add_argument(
		"--keep-open",
		action="store_true",
		help="Keep process alive after launching Streamlit",
	)
	parser.add_argument(
		"--optimizer-arg",
		action="append",
		default=[],
		help="Additional argument passed to optimizer",
	)
	return parser.parse_args()


def detect_optimizer(explicit: str, workspace: Path) -> Path:
	if explicit:
		candidate = Path(explicit)
		if candidate.exists():
			return candidate
		raise FileNotFoundError(f"Optimizer not found: {explicit}")

	for name in ("loop_optimizer_split.exe", "loop_optimizer.exe"):
		candidate = workspace / name
		if candidate.exists():
			return candidate

	raise FileNotFoundError(
		"Could not find loop_optimizer_split.exe or loop_optimizer.exe in workspace."
	)


def parse_timing(stdout: str, input_name: str, output_name: str) -> dict:
	input_pat = re.escape(input_name) + r":\s*([0-9]*\.?[0-9]+)s"
	output_pat = re.escape(output_name) + r":\s*([0-9]*\.?[0-9]+)s"

	original = re.search(input_pat, stdout, flags=re.IGNORECASE)
	optimized = re.search(output_pat, stdout, flags=re.IGNORECASE)
	improvement = re.search(
		r"Improvement\s*\([^)]*\):\s*([-+]?[0-9]*\.?[0-9]+)%",
		stdout,
		flags=re.IGNORECASE,
	)

	data = {}
	if original:
		data["original_seconds"] = float(original.group(1))
	if optimized:
		data["optimized_seconds"] = float(optimized.group(1))
	if improvement:
		data["improvement_percent"] = float(improvement.group(1))
	return data


def launch_streamlit(
	workspace: Path,
	input_path: Path,
	output_path: Path,
	metrics_path: Path,
	port: int,
) -> subprocess.Popen:
	cmd = [
		sys.executable,
		"-m",
		"streamlit",
		"run",
		"optimizer_dashboard.py",
		"--server.port",
		str(port),
		"--server.headless",
		"false",
		"--",
		"--input",
		str(input_path),
		"--output",
		str(output_path),
		"--metrics",
		str(metrics_path),
	]
	return subprocess.Popen(cmd, cwd=workspace)


def main() -> int:
	args = parse_args()
	workspace = Path(__file__).resolve().parent

	input_path = (
		(workspace / args.input).resolve()
		if not Path(args.input).is_absolute()
		else Path(args.input)
	)
	output_path = (
		(workspace / args.output).resolve()
		if not Path(args.output).is_absolute()
		else Path(args.output)
	)

	try:
		optimizer = detect_optimizer(args.optimizer, workspace)
	except FileNotFoundError as exc:
		print(exc)
		return 1

	if not input_path.exists():
		print(f"Input file not found: {input_path}")
		return 1

	optimizer_cmd = [str(optimizer), str(input_path)] + args.optimizer_arg
	print("Running optimizer:", " ".join(optimizer_cmd))
	run = subprocess.run(optimizer_cmd, cwd=workspace, capture_output=True, text=True)

	print(run.stdout)
	if run.stderr.strip():
		print(run.stderr, file=sys.stderr)

	metrics = {
		"started_at": datetime.now().isoformat(timespec="seconds"),
		"optimizer": str(optimizer.name),
		"input": str(input_path),
		"output": str(output_path),
		"return_code": run.returncode,
		"stdout": run.stdout,
		"stderr": run.stderr,
		"timing": parse_timing(run.stdout, input_path.name, output_path.name),
	}

	metrics_path = workspace / "last_optimizer_run.json"
	metrics_path.write_text(json.dumps(metrics, indent=2), encoding="utf-8")

	if run.returncode != 0:
		print("Optimizer failed, dashboard not started.")
		return run.returncode

	try:
		process = launch_streamlit(
			workspace, input_path, output_path, metrics_path, args.port
		)
	except Exception as exc:  # pylint: disable=broad-except
		print("Failed to launch Streamlit. Install it using: pip install streamlit")
		print(f"Reason: {exc}")
		return 1

	dashboard_url = f"http://localhost:{args.port}"
	time.sleep(1)
	webbrowser.open_new(dashboard_url)
	print(f"Dashboard started at {dashboard_url}")

	if args.keep_open:
		try:
			process.wait()
		except KeyboardInterrupt:
			process.terminate()
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
