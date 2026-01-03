#!/bin/bash
echo "=========================================="
echo "  TEST: Prepunjenje izlaznog buffera"
echo "=========================================="

# Reload modul
sudo ./unload_shofer
sudo ./load_shofer

echo -e "\n[KORAK 1] Napunimo izlazni buffer sa 64 bajta"
# Napiši 64 karaktera u ulazni
python3 -c "print('A' * 64, end='')" > /dev/shofer_in

# Prebaci sve u izlazni (pun buffer!)
./test/ioctl /dev/shofer_control 64
echo "✓ Izlazni buffer sada ima 64 bajta (PUN!)"

echo -e "\n[KORAK 2] Pokušaj prebaciti još podataka"
# Napiši još 20 karaktera u ulazni
python3 -c "print('B' * 20, end='')" > /dev/shofer_in

# Pokušaj prebaciti - NE BI TREBALO RADITI jer je izlazni PUN!
echo "Pokušavam prebaciti 20 'B' znakova sa ioctl..."
./test/ioctl /dev/shofer_control 20
echo "✓ ioctl izvršen (ali možda nije ništa prebačeno)"

echo -e "\n[KORAK 3] Provjeri izlazni buffer"
# Čitaj - trebao bi vidjeti samo 'A' znakove (64), bez 'B'
echo "Sadržaj izlaznog buffera (prvih 70 bajtova):"
dd if=/dev/shofer_out bs=1 count=70 2>/dev/null | od -c | head -5

echo -e "\n[KORAK 4] Provjerimo da li su 'B' ostali u ulaznom bufferu"
# Sada kada smo ispraznili izlazni, probaj ponovo
echo "Pokušavam ponovo prebaciti 'B' znakove..."
./test/ioctl /dev/shofer_control 20

echo "Sadržaj izlaznog buffera (sad bi trebali biti 'B' znakovi):"
dd if=/dev/shofer_out bs=1 count=20 2>/dev/null
echo

echo -e "\n[KORAK 5] Kernel log - provjeri greške"
sudo dmesg | grep shofer | tail -20

echo "=========================================="
