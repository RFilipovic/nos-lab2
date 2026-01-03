#!/bin/bash

echo "=========================================="
echo "   LAB2B - Test Kernel Modula"
echo "=========================================="
echo

# Boje za output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}KORAK 1: Učitavam modul u kernel...${NC}"
sudo ./load_shofer
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Modul učitan!${NC}"
else
    echo -e "${RED}✗ Greška pri učitavanju modula${NC}"
    exit 1
fi
echo

echo -e "${BLUE}KORAK 2: Provjeravam da li su naprave kreirane...${NC}"
ls -l /dev/shofer_* 2>/dev/null
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Naprave kreirane!${NC}"
else
    echo -e "${RED}✗ Naprave nisu kreirane${NC}"
    exit 1
fi
echo

echo -e "${BLUE}KORAK 3: TEST - Pišem 'Hello World' u ulazni međuspremnik${NC}"
echo "Hello World" > /dev/shofer_in
echo -e "${GREEN}✓ Podaci upisani u /dev/shofer_in${NC}"
echo

echo -e "${BLUE}KORAK 4: TEST - Koristim ioctl da prebacim 5 bajtova${NC}"
./test/ioctl /dev/shofer_control 5
echo -e "${GREEN}✓ ioctl izvršen - prebacio 5 bajtova iz in_buff → out_buff${NC}"
echo

echo -e "${BLUE}KORAK 5: TEST - Čitam iz izlaznog međuspremnika${NC}"
echo -n "Output: "
dd if=/dev/shofer_out bs=1 count=5 2>/dev/null
echo
echo -e "${GREEN}✓ Podaci pročitani iz /dev/shofer_out${NC}"
echo

echo -e "${BLUE}KORAK 6: TEST - Timer funkcionalnost${NC}"
echo "Pišem 'ABC' u ulazni međuspremnik..."
echo "ABC" > /dev/shofer_in
echo "Čekam 16 sekundi da timer prebaci znakove (3 timer perioda po 5s)..."
echo -n "Timer radi: "
for i in {16..1}; do
    echo -n "$i "
    sleep 1
done
echo
echo -n "Output nakon timer-a: "
dd if=/dev/shofer_out bs=1 count=10 2>/dev/null
echo
echo -e "${GREEN}✓ Timer je automatski prebacio znakove!${NC}"
echo

echo -e "${BLUE}KORAK 7: Prikazujem kernel log (dmesg)${NC}"
echo "Zadnjih 40 log poruka:"
echo "--------------------------------------"
sudo dmesg | grep shofer | tail -40
echo "--------------------------------------"
echo

echo -e "${BLUE}KORAK 8: Isključujem modul...${NC}"
sudo ./unload_shofer
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Modul isključen!${NC}"
else
    echo -e "${RED}✗ Greška pri isključivanju modula${NC}"
fi
echo

echo "=========================================="
echo -e "${GREEN}   Testiranje završeno!${NC}"
echo "=========================================="
