# Run from the repository root: gdb -q -x scripts/debug-demo.gdb
set pagination off
directory src/internal/engine
file build/logq-debug
set args --text hello --number 21

break main.main
break 'logq/src/internal/engine.Demo'
break lq_demo_inspect

# Start in Go main. Use next to step, continue to reach Demo and then C++.
run
list
