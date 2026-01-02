# Kako testirati Lab2a - Poll operaciju

## Brzi pregled

Imaš dva programa:
- **reader** - čita podatke s naprava
- **writer** - piše podatke na naprave

## Korak po korak testiranje

### 1. Provjeri da li je modul učitan

```bash
lsmod | grep shofer
ls -l /dev/shofer*
```

Ako modul NIJE učitan:
```bash
cd /home/rene/Documents/nos-lab2/lab2a
sudo ./load_shofer
```

### 2. Otvori DVA TERMINALA

#### Terminal 1: Pokreni READER

```bash
cd /home/rene/Documents/nos-lab2/lab2a/test
./reader
```

Vidjet ćeš:
```
Otvaram 6 naprava za čitanje...
Otvorio /dev/shofer0 (fd=3)
Otvorio /dev/shofer1 (fd=4)
...
Čekam na podatke (Ctrl+C za izlaz)...
```

Reader sada ČEKA na podatke (koristi poll).

#### Terminal 2: Pokreni WRITER

```bash
cd /home/rene/Documents/nos-lab2/lab2a/test
./writer
```

Vidjet ćeš:
```
Otvaram 6 naprava za pisanje...
Otvorio /dev/shofer0 (fd=3)
Otvorio /dev/shofer1 (fd=4)
...
Periodički šaljem znakove svakih 5000 ms (Ctrl+C za izlaz)...

[shofer2] Napisao znak 'A' (6 spremnih naprava)
```

### 3. Što očekivati

**U Terminal 2 (writer):**
```
[shofer3] Napisao znak 'A' (6 spremnih naprava)
[shofer1] Napisao znak 'B' (6 spremnih naprava)
[shofer0] Napisao znak 'C' (6 spremnih naprava)
...
```
Writer svakih 5 sekundi nasumično odabire napravu i piše znak (A-Z)

**U Terminal 1 (reader):**
```
[shofer3] Pročitano 1 byte(s): 'A'
[shofer1] Pročitano 1 byte(s): 'B'
[shofer0] Pročitano 1 byte(s): 'C'
...
```
Reader ODMAH čita znakove čim ih writer napiše (poll reagira!)

### 4. Testiranje različitih opcija

**Koristi samo 3 naprave umjesto 6:**
```bash
# Terminal 1
./reader 3

# Terminal 2
./writer 3
```

**Writer šalje brže (svakih 2 sekunde):**
```bash
./writer 4 2000
```

### 5. Zaustavi programe

U oba terminala pritisni: **Ctrl+C**

### 6. Ukloni modul (nakon testiranja)

```bash
cd /home/rene/Documents/nos-lab2/lab2a
sudo ./unload_shofer
```

## Brzi test (jedan terminal)

Ako želiš brzo testirati bez dva terminala:

```bash
cd /home/rene/Documents/nos-lab2/lab2a/test

# Pokreni writer u pozadini
./writer 4 2000 &

# Pokreni reader (Ctrl+C nakon nekoliko čitanja)
./reader 4

# Zaustavi writer
killall writer
```

## Što demonstrira ovaj test?

1. **poll() na višestrukim napravama** - reader čeka na sve naprave odjednom
2. **Non-blocking I/O** - reader ne blokira na jednoj napravi
3. **Event-driven čitanje** - reader reagira čim su podaci spremni
4. **poll() za provjeru spremnosti** - writer provjerava prije pisanja
5. **Nasumični odabir** - writer slučajno bira napravu

## Debugging

Pogledaj kernel poruke:
```bash
dmesg | tail -20
```

Omogući debug (zahtijeva rekompilaciju modula):
```bash
# U config.h odkomentiraj:
# #define SHOFER_DEBUG

cd /home/rene/Documents/nos-lab2/lab2a
make clean && make
sudo ./unload_shofer
sudo ./load_shofer

# Gledaj poruke
dmesg -w
```

## Troubleshooting

**"Permission denied" pri otvaranju naprave:**
```bash
ls -l /dev/shofer*  # Provjeri permisije
# Trebale bi biti 666 (rw-rw-rw-)
```

**"No such file or directory" za /dev/shofer*:**
```bash
# Učitaj modul
sudo ./load_shofer
```

**Writer kaže "Nijedna naprava nije spremna":**
- To je normalno ako su bufferi puni
- Reader će isprazniti buffere i omogućiti pisanje
