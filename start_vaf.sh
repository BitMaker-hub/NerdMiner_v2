#!/bin/bash

SESSION="vaf_miner"

# Start a new session, detached
tmux new-session -d -s $SESSION

# Window 1: Compilation
tmux rename-window -t $SESSION:0 'Build'
tmux send-keys -t $SESSION:0 "echo '--- VAF Miner Build Console ---' && pio run -e esp32-s3-devkitc-1" C-m

# Window 2: Monitor
tmux rename-window -t $SESSION:1 'Monitor'
tmux send-keys -t $SESSION:1 "echo '--- Serial Monitor (Awaiting Device) ---' && pio device monitor -e esp32-s3-devkitc-1" C-m

# Window 3: Host Performance
tmux rename-window -t $SESSION:2 'Host-Bench'
tmux send-keys -t $SESSION:2 "echo '--- Phone CPU Benchmark ---' && ./miner_test" C-m

# Attach to the session
tmux attach-session -t $SESSION
