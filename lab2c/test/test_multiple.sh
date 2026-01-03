#!/bin/bash
# Test with multiple readers and writers

echo "Pokrećem test s više čitača i pisača:"
echo "- 2 čitača"
echo "- 2 pisača"
echo ""
echo "Pritisni Ctrl+C za zaustavljanje"
echo ""

# Start readers
./citac &
PID1=$!
./citac &
PID2=$!

sleep 1

# Start writers
./pisac &
PID3=$!
./pisac &
PID4=$!

echo "Test u tijeku... Pritisni Ctrl+C za zaustavljanje"
echo "Prati kernel logove sa: sudo dmesg -w | grep shofer"
wait

# Cleanup
kill $PID1 $PID2 $PID3 $PID4 2>/dev/null
