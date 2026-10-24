#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=========================================================="
echo "  CS F214 Logic Assignment 1: Task 6 Analysis Suite       "
echo "=========================================================="

cd "$WORKSPACE_ROOT"

# Determine Python interpreter with matplotlib & pandas
if [ -x "/home/agnibh/anaconda3/bin/python3" ]; then
    PYTHON_EXEC="/home/agnibh/anaconda3/bin/python3"
elif command -v python3 &>/dev/null; then
    PYTHON_EXEC="python3"
else
    echo "Error: python3 not found."
    exit 1
fi

# 0. Purge all previous plots, cached data, and generated artifacts
echo "[0/5] Purging all old plots and cached benchmark datasets..."
rm -f Part-A/analysis/plots/*.png Part-A/docs/html/plots/*.png
rm -f Part-A/analysis/data/*.csv Part-A/analysis/data/*.json

# 1. Compile C Programs for Tasks 1-5
echo "[1/5] Compiling Tasks 1 through 5 in Part-A..."
gcc -Wall -O2 Part-A/task1.c -o Part-A/task1
gcc -Wall -O2 Part-A/task2.c -o Part-A/task2
gcc -Wall -O2 Part-A/task3.c -o Part-A/task3
gcc -Wall -O2 Part-A/task4.c -o Part-A/task4
gcc -Wall -O2 Part-A/task5.c -o Part-A/task5

# 2. Extract and index SAT 2002 benchmark files
echo "[2/5] Indexing SAT 2002 benchmark suite..."
python3 Part-A/analysis/cnf_extractor.py

# 3. Compile and run high-precision C benchmark engine
echo "[3/5] Running High-Precision Microsecond Benchmark Engine..."
gcc -O3 -Wall Part-A/analysis/benchmark_engine.c -o Part-A/analysis/benchmark_engine -lm
./Part-A/analysis/benchmark_engine

# 4. Generate all 9 visualization figures
echo "[4/5] Generating 9 publication-grade visualization plots..."
"$PYTHON_EXEC" Part-A/analysis/generate_visualizations.py

# 5. Generate interactive HTML reports
echo "[5/5] Generating aesthetic HTML report dashboard..."
"$PYTHON_EXEC" Part-A/analysis/generate_html_report.py

echo "=========================================================="
echo "  Analysis pipeline complete!                             "
echo "  Report available at:                                    "
echo "    1. Part-A/analysis/index.html                         "
echo "    2. Part-A/docs/html/analysis.html                     "
echo "=========================================================="

