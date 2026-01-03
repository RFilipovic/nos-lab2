# 🚀 QUICK START - LAB2C Cjevovod

## TL;DR - Najbrži test

```bash
cd /home/rene/Documents/nos-lab2/lab2c

# 1. Učitaj modul
sudo ./load_shofer

# 2. Terminal 1 - Pokreni čitača
./test/citac

# 3. Terminal 2 - Pokreni pisača
./test/pisac

# 4. Kada završiš, zaustavi programe (Ctrl+C) i ukloni modul
sudo ./unload_shofer
```

---

## Što ćeš vidjeti

### Terminal 1 (čitač):
```
Citac 12345 poziva read
Citac 12345 procitao (45): ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQ
Citac 12345 poziva read
Citac 12345 procitao (52): XYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUV
Citac 12345 poziva read
...
```

### Terminal 2 (pisač):
```
Pisac 67890 poziva write s tekstom (45): ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQ
Pisac 67890 poslao (45): ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQ
Pisac 67890 poziva write s tekstom (52): XYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUV
Pisac 67890 poslao (52): XYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUV
...
```

### Kernel log (dmesg | tail):
```
[shofer] Module started initialization
[shofer] Module initialized with major=237, minor=0
[shofer] Device opened, thread_cnt=1
[shofer] Device opened, thread_cnt=2
[shofer] Written 45 bytes
[shofer] Read 16 bytes
[shofer] Read 29 bytes
[shofer] Written 52 bytes
...
```

---

## Demonstracija različitih scenarija

### 1️⃣ Čitač čeka (cijev prazna)

```bash
# Terminal 1 - Prvo pokreni SAMO čitača
./test/citac
```

**Što se događa:**
- Čitač ispisuje: `Citac XXXX poziva read`
- Program se **zamrzne** - čitač čeka podatke!
- U kernel logu: čitač je blokiran na semaforu `empty`

```bash
# Terminal 2 - Sada pokreni pisača
./test/pisac
```

**Što se događa:**
- ✨ Čitač se odmah **odblokirava**!
- Čitač ispisuje pročitane podatke
- Komunikacija teče normalno

---

### 2️⃣ Više čitača natječu se za podatke

```bash
# Pokreni 3 čitača istovremeno
./test/citac &
./test/citac &
./test/citac &

# Pokreni 1 pisača
./test/pisac
```

**Što se događa:**
- Pisač šalje jedan podatak
- Samo JEDAN čitač ga prima (prvi koji uđe u kritični odsječak)
- Ostali čitači čekaju sljedeći podatak
- Podaci se pravilno distribuiraju među čitačima

**Očisti:**
```bash
killall citac pisac
```

---

### 3️⃣ Više pisača šalju podatke

```bash
# Pokreni 1 čitača
./test/citac &

# Pokreni 3 pisača
./test/pisac &
./test/pisac &
./test/pisac &
```

**Što se događa:**
- Pisači se redom stavljaju u kritični odsječak
- Čitač prima podatke od različitih pisača
- Svi podaci se pravilno prenose

**Očisti:**
```bash
killall citac pisac
```

---

### 4️⃣ Testiranje maksimalnog broja dretvi

```bash
# Učitaj modul s ograničenjem od 3 dretve
sudo ./unload_shofer
sudo ./load_shofer max_threads=3

# Pokušaj otvoriti 4 puta
./test/citac &    # Dretva 1 ✓
./test/citac &    # Dretva 2 ✓
./test/citac &    # Dretva 3 ✓
./test/citac      # Dretva 4 ✗ GREŠKA!
```

**Očekivani ispis za 4. proces:**
```
Nisam otvorio cjevovod! Greska: : Device or resource busy
```

**Očisti:**
```bash
killall citac
sudo ./unload_shofer
sudo ./load_shofer  # Vrati default (max_threads=5)
```

---

### 5️⃣ Testiranje kontrole pristupa

**Test A - Pokušaj pisanja u read-only:**
```bash
cat > /tmp/test_ro.c << 'EOF'
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/dev/shofer", O_RDONLY);
    char buf[10] = "test";

    printf("Pokušavam write() u O_RDONLY device...\n");
    if (write(fd, buf, 4) == -1) {
        perror("write");
        printf("✓ Write je ispravno odbijen!\n");
    }
    close(fd);
    return 0;
}
EOF

gcc /tmp/test_ro.c -o /tmp/test_ro
/tmp/test_ro
```

**Očekivani ispis:**
```
Pokušavam write() u O_RDONLY device...
write: Operation not permitted
✓ Write je ispravno odbijen!
```

**Test B - Pokušaj čitanja iz write-only:**
```bash
cat > /tmp/test_wo.c << 'EOF'
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/dev/shofer", O_WRONLY);
    char buf[10];

    printf("Pokušavam read() iz O_WRONLY device...\n");
    if (read(fd, buf, 10) == -1) {
        perror("read");
        printf("✓ Read je ispravno odbijen!\n");
    }
    close(fd);
    return 0;
}
EOF

gcc /tmp/test_wo.c -o /tmp/test_wo
/tmp/test_wo
```

**Očekivani ispis:**
```
Pokušavam read() iz O_WRONLY device...
read: Operation not permitted
✓ Read je ispravno odbijen!
```

---

### 6️⃣ Praćenje u realnom vremenu

Otvori 3 terminala:

**Terminal 1 - Kernel logovi:**
```bash
sudo dmesg -wH | grep shofer
```

**Terminal 2 - Čitač:**
```bash
cd /home/rene/Documents/nos-lab2/lab2c
./test/citac
```

**Terminal 3 - Pisač:**
```bash
cd /home/rene/Documents/nos-lab2/lab2c
./test/pisac
```

Gledaj kako se poruke sinkroniziraju između svih terminala!

---

### 7️⃣ Testiranje s prilagođenom veličinom cijevi

```bash
# Učitaj modul s većom cijevi
sudo ./unload_shofer
sudo ./load_shofer pipe_size=256

# Provjeri parametar
cat /sys/module/shofer/parameters/pipe_size
# Output: 256

# Testiraj s većim podacima
./test/pisac  # Pisač sada može slati veće pakete bez blokiranja
```

---

## Brza dijagnostika

### Provjeri je li modul učitan:
```bash
lsmod | grep shofer
```

### Provjeri je li device kreiran:
```bash
ls -l /dev/shofer
```

### Vidi zadnjih 20 kernel poruka:
```bash
dmesg | tail -20
```

### Vidi aktivne procese:
```bash
ps aux | grep -E "citac|pisac"
```

### Zaustavi sve testove:
```bash
killall citac pisac
```

---

## Uobičajeni problemi

| Problem | Uzrok | Rješenje |
|---------|-------|----------|
| `Can't get major device number` | Modul već učitan | `sudo ./unload_shofer` pa ponovo učitaj |
| `Device or resource busy` | Previše otvorenih procesa | Zatvori neke ili poveći `max_threads` |
| `No such device` | Device nije kreiran | Provjeri `dmesg` za greške |
| Čitač/pisač se zamrzne | Očekivano ponašanje! | Pokreni nedostajući program (pisač/čitač) |

---

## Prilagođeni parametri

```bash
# Velika cijev, malo dretvi
sudo ./load_shofer pipe_size=512 max_threads=2

# Mala cijev, puno dretvi
sudo ./load_shofer pipe_size=32 max_threads=20

# Provjeri parametre
cat /sys/module/shofer/parameters/pipe_size
cat /sys/module/shofer/parameters/max_threads
```

---

## Kompletna test sekvenca

```bash
# 1. Kompajliranje
cd /home/rene/Documents/nos-lab2/lab2c
make clean && make
cd test && make clean && make && cd ..

# 2. Učitavanje
sudo ./load_shofer

# 3. Verifikacija
ls -l /dev/shofer
lsmod | grep shofer

# 4. Testiranje
./test/citac &
sleep 1
./test/pisac &

# 5. Praćenje (u drugom terminalu)
sudo dmesg -w | grep shofer

# 6. Čekaj 10 sekundi
sleep 10

# 7. Zaustavi
killall citac pisac

# 8. Ukloni modul
sudo ./unload_shofer

# 9. Provjeri čišćenje
lsmod | grep shofer  # Ne bi trebalo biti rezultata
ls -l /dev/shofer    # Ne bi trebalo postojati
```

---

## Više informacija

- Detaljne upute: `UPUTE_TESTIRANJE.md`
- Kako radi: `KAKO_RADI.md`
- Osnovne info: `Readme.txt`

---

**Uživaj u testiranju! 🎉**
