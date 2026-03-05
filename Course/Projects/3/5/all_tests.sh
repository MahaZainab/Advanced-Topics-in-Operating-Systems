#!/usr/bin/env bash
set -euo pipefail

LOG="aubatch_test_results.txt"
TEST_INPUT="all_tests.in"

# 1) Build fresh
make clean >/dev/null 2>&1 || true
make

# 2) Create one big input file for aubatch (all tests)
cat > "$TEST_INPUT" <<'EOF'
help
list
fcfs
list
sjf
list
priority
list
foo

# ---------- run short jobs FCFS (2–3 times) ----------
fcfs
run ./batch_job 2 1
list
run ./batch_job 2 1
list
run ./batch_job 2 1
list

# ---------- run short jobs SJF ----------
sjf
run ./batch_job 3 1
list
run ./batch_job 1 1
list
run ./batch_job 2 1
list

# ---------- run short jobs Priority ----------
priority
run ./batch_job 2 3
list
run ./batch_job 2 1
list
run ./batch_job 2 2
list

# ---------- long jobs Priority ----------
priority
run ./batch_job 6 3
run ./batch_job 6 1
run ./batch_job 6 2
list

# ---------- long jobs FCFS ----------
fcfs
run ./batch_job 6 1
run ./batch_job 6 2
run ./batch_job 6 3
list

# ---------- long jobs SJF ----------
sjf
run ./batch_job 6 1
run ./batch_job 2 1
run ./batch_job 4 1
run ./batch_job 3 1
list

# ---------- policy switch while waiting: to Priority ----------
fcfs
run ./batch_job 8 3
run ./batch_job 6 2
run ./batch_job 4 1
run ./batch_job 5 3
run ./batch_job 3 2
list
priority
list

# ---------- policy switch while waiting: to FCFS ----------
priority
run ./batch_job 8 3
run ./batch_job 6 2
run ./batch_job 4 1
run ./batch_job 5 3
run ./batch_job 3 2
list
fcfs
list

# ---------- policy switch while waiting: to SJF ----------
fcfs
run ./batch_job 8 3
run ./batch_job 6 2
run ./batch_job 4 1
run ./batch_job 5 3
run ./batch_job 3 2
list
sjf
list

# ---------- test command FCFS ----------
test ./batch_job fcfs 10 3 1 3
list
list

# ---------- test command SJF ----------
test ./batch_job sjf 10 3 1 3
list

# ---------- test command Priority ----------
test ./batch_job priority 10 3 1 3
list

# ---------- invalid run parameters ----------
run ./batch_job -5 1
run ./batch_job 0 1
run ./batch_job 3 -1

# ---------- invalid test ----------
test ./batch_job abc 10 3 1 3

# ---------- stress (10 jobs) ----------
fcfs
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
run ./batch_job 1 1
list

quit
EOF

# 3) Record like "script" would: capture stdout+stderr with timestamps
{
  echo "===== AUbatch Automated Test Run ====="
  echo "Date: $(date)"
  echo "Working dir: $(pwd)"
  echo "======================================"
  echo
  echo ">>> Running ./aubatch with input from $TEST_INPUT"
  echo
  # -u makes output unbuffered so it appears immediately in the log
  stdbuf -o0 -e0 ./aubatch < "$TEST_INPUT"
  echo
  echo "===== END TEST RUN ====="
} | tee "$LOG" >/dev/null

echo "Saved full output to: $LOG"