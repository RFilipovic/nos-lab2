# LAB2C - STRUKTURA PROJEKTA

## 📁 Pregled direktorija

```
lab2c/
├── 🔧 KERNEL MODUL
│   ├── config.h              # Definicije struktura i konstanti
│   ├── shofer.c              # Glavna implementacija cjevovoda
│   ├── Makefile              # Build script za modul
│   ├── load_shofer           # Script za učitavanje modula
│   └── unload_shofer         # Script za uklanjanje modula
│
├── 🧪 TEST PROGRAMI
│   └── test/
│       ├── citac.c           # Program koji čita iz cijevi
│       ├── pisac.c           # Program koji piše u cijev
│       ├── Makefile          # Build script za test programe
│       ├── test_simple.sh    # Test: 1 čitač + 1 pisač
│       ├── test_multiple.sh  # Test: više čitača i pisača
│       └── test_overflow.sh  # Test: preveliki podaci
│
├── 📚 DOKUMENTACIJA
│   ├── QUICK_START.md        # Brzi start - osnovno testiranje
│   ├── UPUTE_TESTIRANJE.md   # Detaljne upute za sve testove
│   ├── KAKO_RADI.md          # Vizualna objašnjenja rada
│   └── Readme.txt            # Osnovne informacije
│
└── 🚀 POMOĆNE SKRIPTE
    └── test_demo.sh          # Automatska demonstracija
```

---

## 📄 Opis datoteka

### Kernel modul (glavni kod)

#### `config.h` (55 linija)
**Svrha:** Definicije struktura, konstanti i makroa

**Ključni elementi:**
- `struct pipe` - Struktura cjevovoda s kfifo, semaforima i mutexom
- `struct shofer_dev` - Device driver struktura
- Konstante: `PIPE_SIZE=64`, `MAX_THREADS=5`
- `klog()` makro za logiranje

**Koristi:**
- Definira arhitekturu cijelog sustava
- Centralizira sve konstante

---

#### `shofer.c` (371 linija)
**Svrha:** Glavna implementacija character device drivera za cjevovod

**Ključne funkcije:**
1. **Module lifecycle:**
   - `shofer_module_init()` - Inicijalizacija modula
   - `shofer_module_exit()` - Čišćenje pri uklanjanju

2. **Device operations:**
   - `shofer_open()` - Otvaranje device-a (provjera prava i limita)
   - `shofer_release()` - Zatvaranje device-a
   - `shofer_read()` - Čitanje iz cijevi (s blokiranjem)
   - `shofer_write()` - Pisanje u cijev (s blokiranjem)

3. **Pipe management:**
   - `pipe_init()` - Inicijalizacija cijevi
   - `pipe_delete()` - Brisanje cijevi

**Mehanizmi sinkronizacije:**
- Semafori: `cs_readers`, `cs_writers`, `empty`, `full`
- Mutex: `lock`
- Blokiranje dretvi kada nema podataka/mjesta

**Provjere sigurnosti:**
- Kontrola pristupa (O_RDONLY/O_WRONLY/O_RDWR)
- Ograničenje broja dretvi
- Provjera veličine podataka

---

#### `Makefile` (26 linija)
**Svrha:** Automatizacija kompajliranja kernel modula

**Komande:**
```bash
make          # Kompajlira modul (stvara shofer.ko)
make clean    # Briše sve generirane datoteke
```

**Značajke:**
- Podrška za DEBUG mode (definira SHOFER_DEBUG)
- Integracija s kernel build sistemom

---

#### `load_shofer` (14 linija)
**Svrha:** Script za učitavanje modula i kreiranje device node-a

**Što radi:**
1. Učitava modul: `insmod shofer.ko`
2. Dohvaća major broj iz `/proc/devices`
3. Kreira `/dev/shofer` s `mknod`
4. Postavlja dozvole na `666` (svi mogu čitati/pisati)

**Korištenje:**
```bash
sudo ./load_shofer [pipe_size=X] [max_threads=Y]
```

---

#### `unload_shofer` (7 linija)
**Svrha:** Script za uklanjanje modula i brisanje device node-a

**Što radi:**
1. Uklanja modul: `rmmod shofer`
2. Briše `/dev/shofer`

**Korištenje:**
```bash
sudo ./unload_shofer
```

---

### Test programi

#### `test/citac.c` (39 linija)
**Svrha:** Demonstracijski program koji čita iz cjevovoda

**Algoritam:**
1. Otvori `/dev/shofer` s `O_RDONLY`
2. U beskonačnoj petlji:
   - Pozovi `read()` za do 16 bajtova
   - Ispiši pročitane podatke
   - Pričekaj 1 sekundu
3. Ako je cijev prazna, **blokira se** i čeka

**Ispis:**
```
Citac 1234 poziva read
Citac 1234 procitao (11): HELLO WORLD
```

---

#### `test/pisac.c` (52 linija)
**Svrha:** Demonstracijski program koji piše u cjevovod

**Algoritam:**
1. Otvori `/dev/shofer` s `O_WRONLY`
2. U beskonačnoj petlji:
   - Generiraj nasumičan tekst (21-42 bajta)
   - Pozovi `write()` za slanje podataka
   - Ispiši poslani tekst
   - Pričekaj 2 sekunde
3. Ako je cijev puna, **blokira se** i čeka

**Ispis:**
```
Pisac 5678 poziva write s tekstom (35): ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHI
Pisac 5678 poslao (35): ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHI
```

---

#### `test/Makefile` (13 linija)
**Svrha:** Build script za test programe

**Komande:**
```bash
make          # Kompajlira citac i pisac
make clean    # Briše izvršne datoteke
```

---

#### `test/test_simple.sh` (24 linija)
**Svrha:** Automatski test s 1 čitačem i 1 pisačem

**Korištenje:**
```bash
./test_simple.sh
```

---

#### `test/test_multiple.sh` (29 linija)
**Svrha:** Test s više čitača i pisača (2+2)

**Demonstrira:**
- Konkurentni pristup cijevi
- Sinkronizaciju više dretvi

---

#### `test/test_overflow.sh` (45 linija)
**Svrha:** Test pisanja prevelikih podataka

**Testira:**
- Pokušaj pisanja 200 bajtova u 64-bajtnu cijev
- Očekivana greška: EFBIG

---

### Dokumentacija

#### `QUICK_START.md` (370 linija)
**Svrha:** Brze upute za početnike

**Sadržaj:**
- TL;DR - najbrži test
- Demonstracija 7 različitih scenarija
- Brza dijagnostika
- Tablica čestih problema

**Za koga:** Početnici koji žele brzo pokrenuti i testirati

---

#### `UPUTE_TESTIRANJE.md` (350 linija)
**Svrha:** Detaljne upute za sva testiranja

**Sadržaj:**
- 8 različitih testova (TEST 1-8)
- Svaki test sa:
  - Uputama korak-po-korak
  - Očekivanim rezultatima
  - Objašnjenjima što se događa

**Testovi:**
1. Učitavanje modula
2. Jedan čitač + jedan pisač
3. Više čitača i pisača
4. Provjera limita dretvi
5. Kontrola pristupa
6. Preveliki podaci
7. Blokiranje pisača
8. Parametri modula

**Za koga:** Svi koji žele dublje razumijevanje

---

#### `KAKO_RADI.md` (600 linija)
**Svrha:** Vizualna objašnjenja unutarnjih mehanizama

**Sadržaj:**
- ASCII dijagrami arhitekture
- 5 detaljnih scenarija korak-po-korak:
  1. Čitač čeka (cijev prazna)
  2. Pisač čeka (cijev puna)
  3. Višestruki čitači i pisači
  4. Kontrola pristupa
  5. Preveliki podatak
- Objašnjenja svih semafora i mutexa
- Vremenski dijagrami
- Praktični primjer s brojevima

**Za koga:** Oni koji žele razumjeti implementaciju

---

#### `Readme.txt` (70 linija)
**Svrha:** Osnovne informacije o projektu

**Sadržaj:**
- Opis projekta
- Karakteristike
- Parametri modula
- Brze upute za kompajliranje i testiranje
- Reference na detaljnu dokumentaciju

**Za koga:** Prva točka kontakta

---

### Pomoćne skripte

#### `test_demo.sh` (65 linija)
**Svrha:** Automatska demonstracija

**Što radi:**
1. Provjerava sudo pristup
2. Učitava modul
3. Verificira kreiranje device-a
4. Prikazuje kernel logove
5. Daje upute za daljnje testiranje

**Korištenje:**
```bash
sudo ./test_demo.sh
```

---

## 🎯 Kako koristiti projekt

### Brzi start (3 koraka)
```bash
cd lab2c
sudo ./load_shofer           # 1. Učitaj modul
./test/citac &               # 2. Pokreni testove
./test/pisac &
sudo ./unload_shofer         # 3. Ukloni modul
```

### Za detaljno testiranje
1. Čitaj `QUICK_START.md`
2. Slijedi upute iz `UPUTE_TESTIRANJE.md`
3. Ako želiš razumjeti kako radi, čitaj `KAKO_RADI.md`

### Za razumijevanje koda
1. Otvori `config.h` - vidi strukture
2. Čitaj `shofer.c` funkciju po funkciju
3. Koristi `KAKO_RADI.md` za vizualizaciju

---

## 📊 Statistika projekta

```
Kernel kod:         ~400 linija (config.h + shofer.c)
Test programi:      ~90 linija (citac.c + pisac.c)
Test skripte:       ~100 linija (4 bash skripte)
Dokumentacija:      ~1400 linija (4 dokumenta)
─────────────────────────────────────────────────
UKUPNO:            ~2000 linija
```

---

## ✅ Kompletnost implementacije

### Zahtjevi zadatka
- ✅ Character device driver
- ✅ Podrška za open/close/read/write
- ✅ Kružni međuspremnik (kfifo)
- ✅ Više dretvi istovremeno (čitači + pisači)
- ✅ Blokiranje na prazan međuspremnik (read)
- ✅ Blokiranje na pun međuspremnik (write)
- ✅ Kontrola pristupa (O_RDONLY/O_WRONLY/O_RDWR)
- ✅ Provjera veličine podataka
- ✅ Parametri modula (pipe_size, max_threads)
- ✅ Sinkronizacija (semafori, mutex)
- ✅ Red čekanja (FIFO) za čitače i pisače
- ✅ Test programi (citac.c, pisac.c)

### Dodatno implementirano
- ✅ Debugging logovi (SHOFER_DEBUG)
- ✅ Rukovanje signalima (interruptible)
- ✅ Automatske test skripte
- ✅ Opsežna dokumentacija
- ✅ Vizualna objašnjenja

---

## 🔍 Ključne značajke implementacije

### Sinkronizacija
- **4 semafora** - koordinacija čitača i pisača
- **1 mutex** - zaštita kfifo strukture
- Spriječen **deadlock**
- Spriječene **race conditions**

### Sigurnost
- Provjera prava pristupa
- Ograničenje broja dretvi
- Validacija veličine podataka
- Rukovanje greškama

### Performanse
- Paralelno čitanje/pisanje (različiti semafori)
- Minimalano blokiranje
- Efikasno korištenje kfifo

---

**Projekt je kompletan i spreman za testiranje!** 🎉
