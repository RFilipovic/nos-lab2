#!/bin/bash
# Test script for lab2c - Pipe implementation

echo "=========================================="
echo "LAB2C - CJEVOVOD (PIPE) TEST DEMONSTRACIJA"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "GREŠKA: Ovaj script mora biti pokrenut sa sudo!"
    echo "Koristi: sudo ./test_demo.sh"
    exit 1
fi

echo "1. Učitavanje kernel modula..."
echo "=========================================="
./load_shofer pipe_size=128 max_threads=5
if [ $? -ne 0 ]; then
    echo "GREŠKA: Nije uspjelo učitavanje modula!"
    exit 1
fi
echo ""

echo "2. Provjera da li je device kreiran..."
echo "=========================================="
ls -l /dev/shofer
if [ ! -c /dev/shofer ]; then
    echo "GREŠKA: Device /dev/shofer nije kreiran!"
    ./unload_shofer
    exit 1
fi
echo ""

echo "3. Provjera kernel log poruka..."
echo "=========================================="
dmesg | tail -5
echo ""

echo "4. Informacije o modulu..."
echo "=========================================="
lsmod | grep shofer
echo ""
modinfo shofer.ko | grep -E "filename|description|parm"
echo ""

echo "=========================================="
echo "TESTIRANJE POČINJE"
echo "=========================================="
echo ""
echo "Sada možeš pokrenuti test programe:"
echo ""
echo "U TERMINALU 1:"
echo "  ./test/citac"
echo ""
echo "U TERMINALU 2:"
echo "  ./test/pisac"
echo ""
echo "U TERMINALU 3 (više čitača):"
echo "  ./test/citac &"
echo "  ./test/citac &"
echo ""
echo "U TERMINALU 4 (više pisača):"
echo "  ./test/pisac &"
echo "  ./test/pisac &"
echo ""
echo "Za praćenje kernel logova:"
echo "  sudo dmesg -w | grep shofer"
echo ""
echo "Za zaustavljanje svih procesa:"
echo "  killall citac pisac"
echo ""
echo "Za uklanjanje modula kada završiš:"
echo "  sudo ./unload_shofer"
echo ""
echo "=========================================="
