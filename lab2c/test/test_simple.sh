#!/bin/bash
# Simple test - one reader, one writer

echo "Pokrećem jednostavan test: 1 čitač + 1 pisač"
echo "Pritisni Ctrl+C za zaustavljanje"
echo ""

# Start reader in background
./citac &
READER_PID=$!

# Wait a bit
sleep 1

# Start writer in background
./pisac &
WRITER_PID=$!

# Wait for user to stop
echo "Test u tijeku... Pritisni Ctrl+C za zaustavljanje"
wait

# Cleanup
kill $READER_PID $WRITER_PID 2>/dev/null
