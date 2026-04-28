import argparse
import subprocess
import sys
import time
import webbrowser
from pathlib import Path


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(
		description="Open Streamlit dashboard for backend loop optimization and comparison"
	)
	parser.add_argument("--input", default="", help="Optional original source path for preloaded compare mode")
	parser.add_argument("--output", default="", help="Optional optimized source path for preloaded compare mode")
	parser.add_argument("--metrics", default="", help="Optional metrics JSON path for preloaded compare mode")
	parser.add_argument("--optimizer", default="", help="Path to optimizer executable")
	parser.add_argument("--port", type=int, default=8501, help="Port for Streamlit app")
	parser.add_argument(
		"--keep-open",
		action="store_true",
		help="Keep process alive after launching Streamlit",
	)
	return parser.parse_args()


def detect_optimizer(explicit: str, workspace: Path) -> Path:
	if explicit:
		candidate = Path(explicit)
		if candidate.exists():
			return candidate
		raise FileNotFoundError(f"Optimizer not found: {explicit}")

	for name in ("loop_optimizer.exe", "loop_optimizer_split.exe"):
		candidate = workspace / name
		if candidate.exists():
			return candidate

	raise FileNotFoundError(
		"Could not find loop_optimizer_split.exe or loop_optimizer.exe in workspace."
	)


def launch_streamlit(
	workspace: Path,
	args: argparse.Namespace,
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
	]

	if args.input:
		cmd.extend(["--input", str(Path(args.input).resolve())])
	if args.output:
		cmd.extend(["--output", str(Path(args.output).resolve())])
	if args.metrics:
		cmd.extend(["--metrics", str(Path(args.metrics).resolve())])

	try:
		optimizer = detect_optimizer(args.optimizer, workspace)
		cmd.extend(["--optimizer", str(optimizer.resolve())])
	except FileNotFoundError:
		# Dashboard can still start and show a clear error in UI.
		if args.optimizer:
			cmd.extend(["--optimizer", args.optimizer])

	return subprocess.Popen(cmd, cwd=workspace)


def main() -> int:
	args = parse_args()
	workspace = Path(__file__).resolve().parent

	try:
		process = launch_streamlit(workspace, args, args.port)
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
