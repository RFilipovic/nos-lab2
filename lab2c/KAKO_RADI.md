# KAKO CJEVOVOD RADI - Vizualna demonstracija

## Arhitektura

```
┌─────────────────────────────────────────────────────────┐
│                    User Space                            │
│                                                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ Čitač 1  │  │ Pisač 1  │  │ Čitač 2  │ ...          │
│  │  (PID)   │  │  (PID)   │  │  (PID)   │              │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘              │
│       │             │             │                      │
│    read()        write()       read()                    │
└───────┼─────────────┼─────────────┼───────────────────┘
        │             │             │
═══════════════════════════════════════════════════════════
        │             │             │
┌───────┼─────────────┼─────────────┼───────────────────┐
│       ▼             ▼             ▼     Kernel Space   │
│  ┌──────────────────────────────────────────────────┐  │
│  │         /dev/shofer (Character Device)           │  │
│  │                                                  │  │
│  │  ┌────────────────────────────────────────────┐ │  │
│  │  │        PIPE STRUCTURE                      │ │  │
│  │  │                                            │ │  │
│  │  │  Semafori:                                 │ │  │
│  │  │  • cs_readers (critical section readers)  │ │  │
│  │  │  • cs_writers (critical section writers)  │ │  │
│  │  │  • empty (čeka čitač - cijev prazna)      │ │  │
│  │  │  • full (čeka pisač - cijev puna)         │ │  │
│  │  │                                            │ │  │
│  │  │  Mutex: lock (zaštita kfifo pristupa)     │ │  │
│  │  │                                            │ │  │
│  │  │  ┌──────────────────────────────────────┐ │ │  │
│  │  │  │   KFIFO (Circular Buffer)            │ │ │  │
│  │  │  │   Size: 64 bytes (default)           │ │  │  │
│  │  │  │                                      │ │ │  │
│  │  │  │  [data][data][    free space    ]   │ │ │  │
│  │  │  │                                      │ │ │  │
│  │  │  └──────────────────────────────────────┘ │ │  │
│  │  │                                            │ │  │
│  │  │  Counters:                                 │ │  │
│  │  │  • thread_cnt: 2 (trenutno otvoreno)      │ │  │
│  │  │  • max_threads: 5 (maksimum)              │ │  │
│  │  └────────────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## Scenarij 1: ČITAČ ČEKA (Cijev prazna)

### Korak 1: Čitač poziva read()
```
Čitač (PID 1234):
  read(fd, buf, 16)
      ↓
  down(cs_readers)  ✓ Uđe u kritični odsječak za čitače
      ↓
  mutex_lock(lock)  ✓ Zaključa pristup cijevi
      ↓
  kfifo_is_empty?   ✓ DA - cijev je prazna!
      ↓
  reader_waiting = 1
      ↓
  mutex_unlock(lock) → Otključa
      ↓
  down(empty)  ⏸️  BLOKIRA SE - čeka na semafor 'empty'
```

**Stanje:** Čitač spava, čeka da pisač stavi podatke.

### Korak 2: Pisač poziva write() i budi čitača
```
Pisač (PID 5678):
  write(fd, "HELLO", 5)
      ↓
  down(cs_writers)  ✓ Uđe u kritični odsječak za pisače
      ↓
  mutex_lock(lock)  ✓ Zaključa pristup cijevi
      ↓
  kfifo_avail > 5?  ✓ DA - ima mjesta
      ↓
  kfifo_from_user()  → Stavi "HELLO" u cijev
      ↓
  reader_waiting?   ✓ DA - čitač čeka!
      ↓
  up(empty)  🔔 BUDI ČITAČA!
      ↓
  mutex_unlock(lock)
      ↓
  up(cs_writers)
```

### Korak 3: Čitač se budi i čita
```
Čitač (PID 1234):
  [nastavi od down(empty)]
      ↓
  down(empty)  ✓ Semafor je oslobođen, nastavi!
      ↓
  mutex_lock(lock)  ✓ Zaključa pristup cijevi
      ↓
  kfifo_to_user()   → Čita "HELLO" (5 bajtova)
      ↓
  if (writer_waiting) up(full)  → Budi pisača ako čeka
      ↓
  mutex_unlock(lock)
      ↓
  up(cs_readers)
      ↓
  return 5  ✅ Vraća broj pročitanih bajtova
```

---

## Scenarij 2: PISAČ ČEKA (Cijev puna)

### Stanje cijevi:
```
KFIFO (64 bajta):
[XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX]
 ← Cijev potpuno puna! (64/64 bajtova)
```

### Korak 1: Pisač pokušava pisati 50 bajtova
```
Pisač (PID 5678):
  write(fd, data, 50)
      ↓
  count > pipe_size?  ✗ NE (50 < 64)
      ↓
  down(cs_writers)  ✓ Uđe u kritični odsječak za pisače
      ↓
  mutex_lock(lock)  ✓ Zaključa pristup cijevi
      ↓
  kfifo_avail < 50? ✓ DA - nema dovoljno mjesta!
      ↓
  writer_waiting = 1
      ↓
  mutex_unlock(lock)
      ↓
  down(full)  ⏸️  BLOKIRA SE - čeka na semafor 'full'
```

**Stanje:** Pisač spava, čeka da čitač oslobodi mjesto.

### Korak 2: Čitač čita 16 bajtova
```
Čitač (PID 1234):
  read(fd, buf, 16)
      ↓
  ... [standardan tok čitanja]
      ↓
  kfifo_to_user(16)  → Čita 16 bajtova
      ↓
  writer_waiting?   ✓ DA - pisač čeka!
      ↓
  up(full)  🔔 BUDI PISAČA!
```

### Stanje cijevi nakon čitanja:
```
KFIFO (64 bajta):
[XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX................]
 ← Sada ima 16 bajtova slobodno! (48/64 bajtova)
```

### Korak 3: Pisač se budi i piše
```
Pisač (PID 5678):
  [nastavi od down(full)]
      ↓
  down(full)  ✗ ZAUSTAVI - još nema dovoljno mjesta!
              (treba 50, ima samo 16 slobodnih)
      ↓
  ... čeka dalje ...
```

Pisač će **nastaviti čekati** dok čitač ne pročita još barem 34 bajta!

---

## Scenarij 3: VIŠESTRUKI ČITAČI I PISAČI

```
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│  Čitač 1    │  │  Čitač 2    │  │  Pisač 1    │
└──────┬──────┘  └──────┬──────┘  └──────┬──────┘
       │                │                │
       │ read()         │ read()         │ write("ABC",3)
       ▼                ▼                ▼
   down(cs_readers) down(cs_readers) down(cs_writers)
       │                │                │
       ✓ UĐE            ⏸️ ČEKA          ✓ UĐE
       │                                 │
   mutex_lock()                      mutex_lock()
       │                                 │
   kfifo prazna?                     kfifo_from_user()
       │                                 │
   reader_waiting=1                  up(empty) 🔔
       │                                 │
   mutex_unlock()                    mutex_unlock()
       │                                 │
   down(empty) ⏸️                       up(cs_writers)
       │
   [ČITAČ 1 se budi kad pisač stavi podatke]
       │
   kfifo_to_user() → Čita "ABC"
       │
   up(cs_readers) 🔔 Pusti ČITAČA 2!
       │
   [ČITAČ 2 se budi i pokušava čitati...]
```

**Ključna točka:**
- Samo JEDAN čitač može biti u kritičnom odsječku istovremeno (cs_readers)
- Samo JEDAN pisač može biti u kritičnom odsječku istovremeno (cs_writers)
- Ali čitač i pisač mogu raditi paralelno (različiti semafori!)

---

## Scenarij 4: KONTROLA PRISTUPA

### Test: Read iz WRITE-ONLY device-a
```
Process:
  fd = open("/dev/shofer", O_WRONLY)
      ↓
  shofer_open():
    filp->f_flags = O_WRONLY  ✓
    thread_cnt++
      ↓
  read(fd, buf, 10)
      ↓
  shofer_read():
    if ((f_flags & O_ACCMODE) == O_WRONLY)  ✓ DA!
        return -EPERM  ❌ ZABRANJEN PRISTUP!
```

**Rezultat:** `perror("read")` → "Operation not permitted"

---

## Scenarij 5: PREVELIKI PODATAK

### Test: Write 200 bajtova u 64-bajtnu cijev
```
Process:
  fd = open("/dev/shofer", O_WRONLY)
      ↓
  write(fd, data, 200)
      ↓
  shofer_write():
    if (count > pipe_size)  ✓ DA! (200 > 64)
        return -EFBIG  ❌ PODATAK PREVELIK!
```

**Rezultat:** `perror("write")` → "File too large"

---

## Sinkronizacijski mehanizmi

### 1. **Semaphore cs_readers** (inicijaliziran na 1)
- Omogućava samo JEDNOM čitaču da uđe u kritični odsječak
- Osigurava da čitači rade jedan po jedan

### 2. **Semaphore cs_writers** (inicijaliziran na 1)
- Omogućava samo JEDNOM pisaču da uđe u kritični odsječak
- Osigurava da pisači rade jedan po jedan

### 3. **Semaphore empty** (inicijaliziran na 0)
- Čitač poziva `down(empty)` kada je cijev prazna → blokira se
- Pisač poziva `up(empty)` kada stavi podatke → budi čitača

### 4. **Semaphore full** (inicijaliziran na 1)
- Pisač poziva `down(full)` kada je cijev puna → blokira se
- Čitač poziva `up(full)` kada uzme podatke → budi pisača

### 5. **Mutex lock**
- Štiti pristup kfifo strukturi
- Osigurava da samo jedna dretva mijenja kfifo u bilo kojem trenutku

---

## Tok podataka - Vremenski dijagram

```
Vrijeme →

Čitač 1:  |--open--|----READ (čeka)----|========čita=====|--close--|
                              ↑                    ↑
                              |                    |
                           down(empty)          up(empty)
                              ⏸️                     🔔
                              |                    |
Pisač 1:       |--open--|----WRITE (piše)---------|--close--|


Čitač 2:            |--open--|----READ (čeka)--|=čita=|
                                     ↓
                                  down(empty)
                                     ⏸️

KFIFO:      [prazna]  → [ABC....] → [......] → [XYZ] → [..]
            0 bajtova   3 bajta     0 bajtova  3 bajta  0 b
```

---

## Praktični primjer s brojevima

```
Inicijalno stanje:
  pipe_size = 64
  max_threads = 5
  thread_cnt = 0
  kfifo = [ ] (prazno)

T=0: Čitač 1 otvara device
  thread_cnt = 1 ✓

T=1: Pisač 1 otvara device
  thread_cnt = 2 ✓

T=2: Čitač 1 poziva read(16)
  → Cijev prazna, blokira se na down(empty)

T=3: Pisač 1 poziva write("HELLO WORLD", 11)
  → kfifo_avail = 64 ✓ (ima mjesta)
  → Upiše 11 bajtova
  → Primijeti da čitač čeka
  → up(empty) - budi čitača!
  → kfifo = [HELLO WORLD...] (11/64)

T=4: Čitač 1 se budi
  → kfifo_to_user(16)
  → Pročita 11 bajtova (toliko ima)
  → Vrati 11
  → kfifo = [ ] (0/64)

T=5: Pisač piše 60 bajtova
  → kfifo = [XXXX...60 bajtova...XXXX] (60/64)

T=6: Pisač piše 50 bajtova
  → kfifo_avail = 4 (nedovoljno!)
  → Blokira se na down(full)

T=7: Čitač čita 30 bajtova
  → Pročita 30, ostane 30 u kfifo
  → Primijeti da pisač čeka
  → up(full) - budi pisača!
  → kfifo = [XXX...30 bajtova...XXX] (30/64)

T=8: Pisač se budi
  → kfifo_avail = 34 < 50 (još uvijek nedovoljno!)
  → Ponovo se blokira

T=9: Čitač čita 30 bajtova
  → Pročita 30, kfifo potpuno prazna
  → up(full) - budi pisača!
  → kfifo = [ ] (0/64)

T=10: Pisač se budi
  → kfifo_avail = 64 >= 50 ✓
  → Upiše 50 bajtova
  → kfifo = [YYY...50 bajtova...YYY] (50/64)
```

---

## Sigurnosne provjere

### 1. Ograničenje broja dretvi
```c
if (shofer->pipe.thread_cnt >= shofer->pipe.max_threads)
    return -EBUSY;
```

### 2. Provjera veličine podataka
```c
if (count > pipe->pipe_size)
    return -EFBIG;
```

### 3. Provjera prava pristupa
```c
if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
    return -EPERM; // u shofer_read

if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
    return -EPERM; // u shofer_write
```

### 4. Rukovanje signalima
```c
if (mutex_lock_interruptible(&pipe->lock))
    return -ERESTARTSYS; // proces prekinut signalom
```

---

Ova arhitektura omogućava **siguran, konkurentan pristup cjevovodu** od strane više procesa/dretvi uz potpunu zaštitu od *race conditions* i deadlock-a!
