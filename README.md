# Loop Optimizer

A C/C++ source-to-source optimization tool focused on loop-heavy code.

This project reads an input C/C++ file, applies loop optimizations, writes an optimized source file (`ouput.c`), and can compare runtime performance between original and optimized code.

## What It Does

The optimizer currently applies pattern-based transformations in `loop_optimizer_engine.cpp`:

- Loop detection (`for`, `while`)
- Loop unrolling (for simple countable loops)
- Loop Invariant Code Motion (LICM)
- Common Subexpression Elimination (CSE) inside loops
- Algebraic simplification (example: `x + x -> 2 * x`)
- Strength reduction (example: `i * 2 -> i << 1`)
- Conservative dead-store/dead-code cleanup in loop bodies
- Nested-loop optimization support

## Example Transformations

### 1) Duplicate Expression Elimination

Input:

```c
y = a * b;
x = a * b;
```

Optimized pattern:

```c
y = a * b;
x = y;
```

If invariant, it may be hoisted:

```c
t = a * b;
for (...) {
	y = t;
	x = t;
}
```

### 2) Loop Invariant Code Motion

Input:

```c
for (...) {
	y = a * b;
}
```

Optimized:

```c
t = a * b;
for (...) {
	y = t;
}
```

### 3) Algebraic Simplification

```c
x = x + x;        // -> x = 2 * x;
z = i * 0;        // -> z = 0;
```

### 4) Strength Reduction

```c
arr[i] += i * 2;  // -> arr[i] += i << 1;
```

## Project Structure

- `loop_optimizer_engine.cpp`: Core optimization engine
- `loop_optimizer.h`: Optimization report structure and API
- `loop_optimizer.cpp`: CLI main entry and report output
- `cli.cpp`, `cli.h`: Command-line argument parsing
- `performance.cpp`, `performance.h`: Runtime comparison support
- `optimizer_dashboard.py`: Streamlit dashboard for visual comparison
- `run_optimizer_visualizer.py`: End-to-end runner for optimizer + dashboard
- `input.c`: Sample test workload
- `ouput.c`: Generated optimized output (name kept for compatibility)

## Build

From project root (Windows PowerShell):

```powershell
g++ -std=c++17 -O2 loop_optimizer.cpp loop_optimizer_engine.cpp file_utils.cpp cli.cpp performance.cpp -o loop_optimizer.exe
```

Optional binaries used in this project:

```powershell
g++ -std=c++17 -O2 loop_optimizer.cpp loop_optimizer_engine.cpp file_utils.cpp cli.cpp performance.cpp -o loop_optimizer_split.exe
g++ -std=c++17 -O2 loop_optimizer.cpp loop_optimizer_engine.cpp file_utils.cpp cli.cpp performance.cpp -o loop_optimizer_test.exe
```

## Run Optimizer

Basic run:

```powershell
.\loop_optimizer.exe input.c
```

Choose output file and emit machine-readable JSON report:

```powershell
.\loop_optimizer.exe input.c --output optimized.c --report-json optimizer_report.json
```

Skip runtime comparison:

```powershell
.\loop_optimizer.exe input.c --no-compare
```

Generated optimized file is written to:

```text
ouput.c
```

## Visualizer Dashboard

Install Python dependencies:

```powershell
pip install -r requirements.txt
```

Run visualizer:

```powershell
python run_optimizer_visualizer.py --port 8513
```

Then open:

```text
http://localhost:8513
```

Workflow:

- Open dashboard in browser.
- Upload/select the source file in the UI.
- Click **Optimize in backend**.
- View diff, counters, and download optimized output.

Optional preloaded compare mode:

```powershell
python run_optimizer_visualizer.py --input input.c --output optimized.c --metrics optimizer_report.json --port 8513
```

## CLI Arguments

```text
loop_optimizer <input-file> [--compiler <gcc|g++>] [--output <file>] [--report-json <file>] [--no-compare] [--keep-binaries] [--fail-on-error]
```

## Deployment-Friendly Validation

Example command for CI/CD style checks:

```powershell
.\loop_optimizer.exe input.c --output optimized.c --report-json optimizer_report.json --no-compare --fail-on-error
```

## Optimization Report

The tool prints a report such as:

- Loops detected
- Loops unrolled
- LICM hoists
- CSE eliminations
- Algebraic simplifications
- Strength reductions
- Dead code removed

## Correctness Notes

Safety checks are conservative:

- Expressions are hoisted only when they do not depend on loop variables.
- Expressions are not hoisted if they depend on values modified inside the loop.
- Function-call-like expressions are treated conservatively to avoid side-effect issues.

## Current Limitations

- Pattern-based optimizer (not a full parser/AST for all C syntax forms)
- Some complex language constructs may be skipped
- Output formatting is functional but may not always be stylistically perfect
- Output filename is currently `ouput.c` (intentional to match existing toolchain)

## Future Improvements

- Full AST-based transformations for stronger semantic guarantees
- Better alias analysis and side-effect tracking
- More aggressive dead-code elimination across wider scopes
- Optional pretty-printer/formatter for cleaner generated source

## License

Add your preferred license (MIT/Apache-2.0/etc.) in a `LICENSE` file.