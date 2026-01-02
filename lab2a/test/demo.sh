#!/bin/bash
# Demo skripta za testiranje reader i writer programa

cd "$(dirname "$0")"

echo "=========================================="
echo "DEMO: Writer i Reader s poll() operacijom"
echo "=========================================="
echo ""

# Provjeri da li naprave postoje
if [ ! -c /dev/shofer0 ]; then
    echo "ERROR: Naprave /dev/shofer* ne postoje!"
    echo "Prvo učitaj modul:"
    echo "  cd .. && sudo ./load_shofer"
    exit 1
fi

NUM_DEVICES=$(ls /dev/shofer* 2>/dev/null | wc -l)
echo "Pronađeno $NUM_DEVICES naprava:"
ls -l /dev/shofer*
echo ""

# Test 1: Writer
echo "=========================================="
echo "TEST 1: Writer (piše znakove svakih 1s)"
echo "=========================================="
echo "Pokrećem writer za 5 sekundi..."
echo ""

timeout 5 ./writer $NUM_DEVICES 1000

echo ""
echo "Writer završio."
echo ""

# Test 2: Reader čeka podatke
echo "=========================================="
echo "TEST 2: Reader + Writer zajedno"
echo "=========================================="
echo "Pokrećem writer u pozadini..."

./writer $NUM_DEVICES 1000 &
WRITER_PID=$!
echo "Writer PID: $WRITER_PID"

sleep 2

echo ""
echo "Pokrećem reader (čita 10 sekundi)..."
echo ""

timeout 10 ./reader $NUM_DEVICES

echo ""
echo "Reader završio. Zaustavljam writer..."
kill $WRITER_PID 2>/dev/null
wait $WRITER_PID 2>/dev/null

echo ""
echo "=========================================="
echo "DEMO ZAVRŠEN"
echo "=========================================="
