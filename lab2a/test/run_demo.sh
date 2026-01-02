#!/bin/bash
# Jednostavna demo skripta

cd "$(dirname "$0")"

echo "===== WRITER TEST (5 sekundi) ====="
timeout 5 stdbuf -oL ./writer 4 1000 &
WRITER_PID=$!

sleep 3

echo ""
echo "===== READER TEST (7 sekundi) ====="
timeout 7 stdbuf -oL ./reader 4 &
READER_PID=$!

wait $READER_PID
wait $WRITER_PID

echo ""
echo "===== DEMO ZAVRŠEN ====="
