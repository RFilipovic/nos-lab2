# LAB2C - UPUTE ZA TESTIRANJE CJEVOVODA

## Priprema

Prije testiranja, provjerite da sve je kompajlirano:

```bash
cd /home/rene/Documents/nos-lab2/lab2c

# Kompajliranje modula
make

# Kompajliranje test programa
cd test
make
cd ..
```

## TEST 1: Osnovno testiranje - Učitavanje modula

```bash
# Učitaj modul
sudo ./load_shofer

# Provjeri da je modul učitan
lsmod | grep shofer

# Provjeri da je device kreiran
ls -l /dev/shofer

# Provjeri kernel logove
dmesg | tail -10
```

**Očekivani rezultat:**
- Poruka "Created device /dev/shofer"
- Device `/dev/shofer` postoji
- U dmesg: "Module initialized with major=..."

---

## TEST 2: Jedan čitač + jedan pisač

### Terminalom 1 - Pokreni čitača:
```bash
cd /home/rene/Documents/nos-lab2/lab2c
./test/citac
```

### Terminalom 2 - Pokreni pisača:
```bash
cd /home/rene/Documents/nos-lab2/lab2c
./test/pisac
```

**Što se događa:**
1. **Čitač** prvo poziva `read` i blokira se (cijev je prazna)
2. **Pisač** generira nasumičan tekst i šalje ga u cijev
3. **Čitač** se odblokirava i ispisuje primljene podatke
4. Proces se ponavlja

**Očekivani ispis čitača:**
```
Citac 12345 poziva read
Citac 12345 procitao (45): XYZABCDEF...
Citac 12345 poziva read
...
```

**Očekivani ispis pisača:**
```
Pisac 12346 poziva write s tekstom (52): ABCDEFGH...
Pisac 12346 poslao (52): ABCDEFGH...
...
```

Zaustavi sa Ctrl+C u oba terminala.

---

## TEST 3: Više čitača i pisača

### Terminal 1 - Praćenje kernel logova:
```bash
sudo dmesg -w | grep shofer
```

### Terminal 2 - Pokreni test:
```bash
cd /home/rene/Documents/nos-lab2/lab2c/test
./test_multiple.sh
```

**Što se događa:**
- Pokreće 2 čitača i 2 pisača istovremeno
- Svi koriste istu cijev
- Čitači se redom odblokavaju (semafor cs_readers)
- Pisači se redom odblokavaju (semafor cs_writers)

**U kernel logu ćeš vidjeti:**
```
Device opened, thread_cnt=1
Device opened, thread_cnt=2
Device opened, thread_cnt=3
Device opened, thread_cnt=4
Written X bytes
Read Y bytes
...
```

Zaustavi sa Ctrl+C.

---

## TEST 4: Provjera ograničenja broja dretvi

Modul ima parametar `max_threads` (default: 5). Testirajmo:

```bash
# Prvo ukloni postojeći modul
sudo ./unload_shofer

# Učitaj modul s max_threads=2
sudo ./load_shofer max_threads=2

# Pokušaj otvoriti više od 2 puta
cd test
./citac &  # Thread 1
./citac &  # Thread 2
./citac    # Thread 3 - trebao bi dobiti grešku EBUSY
```

**Očekivano:**
- Prvi 2 procesa uspješno otvaraju device
- Treći proces dobiva grešku: "Device or resource busy"

```bash
# Očisti
killall citac
```

---

## TEST 5: Provjera kontrole pristupa (O_RDONLY, O_WRONLY)

### Test 5a: Pokušaj pisanja u read-only device

Kreiraj test program:

```bash
cat > test_writeonly.c << 'EOF'
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/dev/shofer", O_RDONLY);
    char buf[10] = "test";

    printf("Pokušavam pisati u read-only device...\n");
    ssize_t ret = write(fd, buf, 4);
    if (ret == -1) {
        perror("write");
        printf("✓ Ispravno: write je odbijen\n");
    }

    close(fd);
    return 0;
}
EOF

gcc -o test_writeonly test_writeonly.c
./test_writeonly
```

**Očekivano:** Greška "Operation not permitted"

### Test 5b: Pokušaj čitanja iz write-only device

```bash
cat > test_readonly.c << 'EOF'
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/dev/shofer", O_WRONLY);
    char buf[10];

    printf("Pokušavam čitati iz write-only device...\n");
    ssize_t ret = read(fd, buf, 10);
    if (ret == -1) {
        perror("read");
        printf("✓ Ispravno: read je odbijen\n");
    }

    close(fd);
    return 0;
}
EOF

gcc -o test_readonly test_readonly.c
./test_readonly
```

**Očekivano:** Greška "Operation not permitted"

---

## TEST 6: Provjera prevelikih podataka

```bash
cd /home/rene/Documents/nos-lab2/lab2c/test
./test_overflow.sh
```

**Što testira:**
- Pokušava upisati 200 bajtova u cijev veličine 64
- Driver bi trebao vratiti grešku EFBIG

**Očekivani ispis:**
```
Pokušavam upisati 200 bajtova u cijev (veličina cijevi = 64)...
✓ Ispravno: write je vratio grešku: File too large
✓ Greška je EFBIG - podatak prevelik!
```

---

## TEST 7: Testiranje blokiranja pisača (cijev puna)

### Terminal 1:
```bash
# Pokreni pisača (pisat će puno podataka)
cd /home/rene/Documents/nos-lab2/lab2c
./test/pisac
```

Pisač će nakon nekog vremena napuniti cijev (64 bajta) i **blokirati se** jer čeka da netko pročita.

### Terminal 2 - Provjeri stanje:
```bash
# Vidi da pisač čeka
ps aux | grep pisac

# Pokreni čitača da odblokirate pisača
./test/citac
```

Čim pokreneš čitača, pisač će se odblokirati i nastaviti slati podatke!

---

## TEST 8: Parametri modula

```bash
# Ukloni modul
sudo ./unload_shofer

# Učitaj s prilagođenim parametrima
sudo ./load_shofer pipe_size=256 max_threads=10

# Provjeri parametre
cat /sys/module/shofer/parameters/pipe_size
cat /sys/module/shofer/parameters/max_threads
```

**Očekivano:**
```
256
10
```

Sada možeš testirati s većom cijevi i više istovremenih procesa!

---

## Čišćenje nakon testiranja

```bash
# Zaustavi sve procese
killall citac pisac

# Ukloni modul
sudo ./unload_shofer

# Provjeri da je uklonjen
lsmod | grep shofer

# Provjeri da device više ne postoji
ls -l /dev/shofer
```

---

## Automatski test

Za brzu demonstraciju:

```bash
sudo ./test_demo.sh
```

Ova skripta će:
1. Učitati modul
2. Provjeriti da je device kreiran
3. Prikazati kernel logove
4. Dati upute za ručno testiranje

---

## Praćenje rada u realnom vremenu

Otvori 3 terminala:

**Terminal 1 - Kernel logovi:**
```bash
sudo dmesg -w | grep shofer
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

Gledaj kako poruke teku između terminala i kako se logiraju u kernel!

---

## Očekivani kernel log poruke

```
[shofer] Module started initialization
[shofer] Module initialized with major=XXX, minor=0
[shofer] Device opened, thread_cnt=1
[shofer] Device opened, thread_cnt=2
[shofer] Written 52 bytes
[shofer] Read 16 bytes
[shofer] Read 36 bytes
[shofer] Written 48 bytes
[shofer] Device closed, thread_cnt=1
[shofer] Device closed, thread_cnt=0
```

---

## Rješavanje problema

### Problem: "Can't get major device number"
- Modul je možda već učitan
- Rješenje: `sudo ./unload_shofer` pa ponovo `sudo ./load_shofer`

### Problem: "Device or resource busy"
- Dostignut max_threads limit
- Rješenje: Zatvori neke procese ili učitaj modul s većim max_threads

### Problem: "No such device"
- Device nije kreiran
- Rješenje: Provjeri dmesg za greške, pokušaj ponovo učitati modul

### Problem: Pisač se blokira
- Cijev je puna i nema čitača
- Očekivano ponašanje! Pokreni čitača da se odblokirate.

### Problem: Čitač se blokira
- Cijev je prazna i nema pisača
- Očekivano ponašanje! Pokreni pisača da se odblokirate.
