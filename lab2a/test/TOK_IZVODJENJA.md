# TOK IZVOĐENJA - Lab2a Poll Simulacija

## Pregled sistema

```
┌─────────────┐         ┌─────────────┐
│   WRITER    │         │   READER    │
│  (userspace)│         │ (userspace) │
└──────┬──────┘         └──────┬──────┘
       │                       │
       │ open(), write()       │ open(), read()
       │ poll()                │ poll()
       ├───────────────────────┤
       │    SYSCALL            │
       ▼                       ▼
┌──────────────────────────────────────┐
│         KERNEL MODUL (shofer.c)      │
│  ┌─────────────────────────────┐    │
│  │  shofer_dev[0..5]           │    │
│  │  - /dev/shofer0             │    │
│  │  - /dev/shofer1             │    │
│  │  - ...                      │    │
│  └─────────────────────────────┘    │
│  ┌─────────────────────────────┐    │
│  │  buffer[0..5]               │    │
│  │  - kfifo (64 bytes)         │    │
│  │  - wait_queue (rq, wq)      │    │
│  └─────────────────────────────┘    │
└──────────────────────────────────────┘
```

---

## FAZA 1: INICIJALIZACIJA MODULA

### Kod: `shofer_module_init()` (shofer.c:70)

```c
// 1. Alociraj device numbers
alloc_chrdev_region(&dev_no, 0, 6, "shofer");
// Dobivamo major=240, minor=0-5

// 2. Kreiraj 6 bufffera
for (i = 0; i < 6; i++) {
    buffer = buffer_create(64, &retval);
    // Svaki buffer ima kfifo od 64 bajta
    list_add_tail(&buffer->list, &buffers_list);
}

// 3. Kreiraj 6 shofer_dev naprava
for (i = 0; i < 6; i++) {
    shofer = shofer_create(dev_no, &shofer_fops, NULL, &retval);
    // Inicijalizira wait_queue (rq i wq)
    list_add_tail(&shofer->list, &shofers_list);
}

// 4. Poveži naprave s bufferima (round-robin)
// shofer0 → buffer0
// shofer1 → buffer1
// ...
// shofer5 → buffer5
```

**Rezultat:**
```
/dev/shofer0 (240,0) → buffer0 [kfifo 64B, wait_queue rq/wq]
/dev/shofer1 (240,1) → buffer1 [kfifo 64B, wait_queue rq/wq]
/dev/shofer2 (240,2) → buffer2 [kfifo 64B, wait_queue rq/wq]
/dev/shofer3 (240,3) → buffer3 [kfifo 64B, wait_queue rq/wq]
/dev/shofer4 (240,4) → buffer4 [kfifo 64B, wait_queue rq/wq]
/dev/shofer5 (240,5) → buffer5 [kfifo 64B, wait_queue rq/wq]
```

---

## FAZA 2: READER PROGRAM STARTUJE

### Kod: `reader.c:main()`

```c
// 1. OTVORI SVE NAPRAVE
for (i = 0; i < 4; i++) {
    fds[i].fd = open("/dev/shofer0", O_RDONLY);
    // ↓ SYSCALL u kernel
}
```

### U kernelu: `shofer_open()` (shofer.c:221)

```c
static int shofer_open(struct inode *inode, struct file *filp)
{
    shofer = container_of(inode->i_cdev, struct shofer_dev, cdev);
    filp->private_data = shofer;  // Sačuvaj pokazivač na shofer_dev
    return 0;
}
```

**Nakon open() poziva:**
```
Reader proces:
  fd=3 → /dev/shofer0 → kernel shofer_dev[0] → buffer[0]
  fd=4 → /dev/shofer1 → kernel shofer_dev[1] → buffer[1]
  fd=5 → /dev/shofer2 → kernel shofer_dev[2] → buffer[2]
  fd=6 → /dev/shofer3 → kernel shofer_dev[3] → buffer[3]
```

```c
// 2. KONFIGURIRAJ POLLFD
for (i = 0; i < 4; i++) {
    fds[i].events = POLLIN;  // Želimo čitanje
    fds[i].revents = 0;
}

// 3. ČEKAJ NA PODATKE
while (1) {
    ret = poll(fds, 4, -1);  // Timeout = -1 (beskonačno)
    // ↓ SYSCALL u kernel
}
```

---

## FAZA 3: POLL() POZIV OD READER-a

### U kernelu: `shofer_poll()` (shofer.c:293)

```c
static unsigned int shofer_poll(struct file *filp, poll_table *wait)
{
    struct shofer_dev *shofer = filp->private_data;
    struct buffer *buffer = shofer->buffer;
    struct kfifo *fifo = &buffer->fifo;

    // 1. REGISTRIRAJ PROCES U WAIT QUEUE
    poll_wait(filp, &shofer->rq, wait);  // za čitanje
    poll_wait(filp, &shofer->wq, wait);  // za pisanje

    // 2. PROVJERI TRENUTNO STANJE
    unsigned int len = kfifo_len(fifo);    // Koliko podataka ima?
    unsigned int avail = kfifo_avail(fifo); // Koliko mjesta ima?
    unsigned int mask = 0;

    if (len)        // Ima podataka?
        mask |= POLLIN | POLLRDNORM;   // DA - čitljivo!
    if (avail)      // Ima mjesta?
        mask |= POLLOUT | POLLWRNORM;  // DA - zapisljivo!

    return mask;
}
```

### Što se događa u poll():

```
Za svaki fd (0-3):
  1. poll() poziva shofer_poll() za /dev/shofer0
     - kfifo_len(buffer0) = 0  ← PRAZAN
     - Vraća mask = POLLOUT (zapisljivo, ali ne čitljivo)
     - Proces se DODAJE u wait_queue buffer0->rq

  2. poll() poziva shofer_poll() za /dev/shofer1
     - kfifo_len(buffer1) = 0  ← PRAZAN
     - Vraća mask = POLLOUT
     - Proces se DODAJE u wait_queue buffer1->rq

  3. poll() poziva shofer_poll() za /dev/shofer2
     - kfifo_len(buffer2) = 0  ← PRAZAN
     - Vraća mask = POLLOUT
     - Proces se DODAJE u wait_queue buffer2->rq

  4. poll() poziva shofer_poll() za /dev/shofer3
     - kfifo_len(buffer3) = 0  ← PRAZAN
     - Vraća mask = POLLOUT
     - Proces se DODAJE u wait_queue buffer3->rq
```

**Rezultat:**
```
Niti jedan buffer nema podataka (POLLIN nije setiran)
→ poll() BLOKIRA proces
→ Reader proces ide u SLEEP stanje
→ Čeka na wake_up() signal
```

```
KERNEL WAIT QUEUE:
buffer0->rq: [reader_proces]
buffer1->rq: [reader_proces]
buffer2->rq: [reader_proces]
buffer3->rq: [reader_proces]
```

**Reader proces sada SPAVA i čeka podatke!**

---

## FAZA 4: WRITER PROGRAM STARTUJE

### Kod: `writer.c:main()`

```c
// 1. OTVORI NAPRAVE ZA PISANJE
for (i = 0; i < 4; i++) {
    fds[i].fd = open("/dev/shofer0", O_WRONLY);
    fds[i].events = POLLOUT;  // Želimo pisanje
}

// 2. GLAVNA PETLJA
while (1) {
    // Čekaj 1000ms ili dok naprava ne postane zapisljiva
    ret = poll(fds, 4, 1000);  // Timeout = 1000ms

    if (ret > 0) {
        // Neka naprava je spremna!
        // Pronađi sve spremne naprave
        for (i = 0; i < 4; i++) {
            if (fds[i].revents & POLLOUT)
                ready_devices[num_ready++] = i;
        }

        // Nasumično odaberi jednu
        selected = ready_devices[rand() % num_ready];

        // PIŠI ZNAK
        write(fds[selected].fd, &current_char, 1);
        // ↓ SYSCALL u kernel
    }
}
```

---

## FAZA 5: WRITER PIŠE PODATAK

### U kernelu: `shofer_write()` (shofer.c:262)

```c
static ssize_t shofer_write(struct file *filp, const char __user *ubuf,
    size_t count, loff_t *f_pos)
{
    struct shofer_dev *shofer = filp->private_data;
    struct buffer *buffer = shofer->buffer;
    struct kfifo *fifo = &buffer->fifo;

    // 1. ZAKLJUČAJ BUFFER
    mutex_lock(&buffer->lock);

    // 2. KOPIRAJ PODATKE IZ USERSPACE U KFIFO
    retval = kfifo_from_user(fifo, ubuf, count, &copied);
    // Recimo da writer piše 'A' u /dev/shofer2
    // → buffer2->fifo ← 'A'

    // 3. SIMULIRAJ DELAY (1000ms)
    simulate_delay(1000);

    // 4. OTKLJUČAJ
    mutex_unlock(&buffer->lock);

    // 5. ★★★ PROBUDI ČEKAJUĆE PROCESE ★★★
    wake_up_all(&shofer->wq);  // Probudi one koji čekaju na pisanje

    return copied;
}
```

### ŠTO SE DOGAĐA:

```
PRIJE write():
buffer2->fifo: []  (prazan)
buffer2->rq: [reader_proces]  (čeka na čitanje)

WRITE('A'):
1. mutex_lock(&buffer2->lock)
2. kfifo_from_user() → buffer2->fifo: ['A']
3. simulate_delay(1000ms)  ← Spava 1 sekundu
4. mutex_unlock(&buffer2->lock)
5. wake_up_all(&buffer2->wq)  ← PROBUDI ČEKAJUĆE!

NAKON write():
buffer2->fifo: ['A']  (ima podataka!)
buffer2->rq: [reader_proces]  (još uvijek čeka)
```

**VAŽNO:** `wake_up_all(&shofer->wq)` - ovo budi procese koji čekaju na **PISANJE** (wq = write queue)

Ali mi trebamo probuditi **ČITAČE**! Gdje se to događa?

---

## FAZA 6: READER SE BUDI!

### Poll mehanizam u kernelu:

Kada `write()` završi, kernel **automatski provjerava** sve poll wait queues:

1. Kernel zna da je reader registriran u `buffer2->rq` (read queue)
2. Kernel ponovno poziva `shofer_poll()` za sve fds koje reader čeka
3. Za `/dev/shofer2`:
   ```c
   len = kfifo_len(&buffer2->fifo);  // len = 1 (ima 'A')
   if (len)
       mask |= POLLIN;  // ★ SADA JE SETIRAN POLLIN!
   ```
4. Poll vidi da je `fds[2].revents & POLLIN` setiran
5. **Poll vraća control reader procesu!**

### U readeru:

```c
ret = poll(fds, 4, -1);  // Vraća 1 (jedna naprava spremna)

// Provjeri koja naprava je spremna
for (i = 0; i < 4; i++) {
    if (fds[i].revents & POLLIN) {  // fds[2] JE SETIRAN!
        bytes_read = read(fds[i].fd, buffer, 64);
        // ↓ SYSCALL u kernel
    }
}
```

---

## FAZA 7: READ() OPERACIJA

### U kernelu: `shofer_read()` (shofer.c:231)

```c
static ssize_t shofer_read(struct file *filp, char __user *ubuf,
    size_t count, loff_t *f_pos)
{
    struct shofer_dev *shofer = filp->private_data;
    struct buffer *buffer = shofer->buffer;
    struct kfifo *fifo = &buffer->fifo;

    // 1. ZAKLJUČAJ
    mutex_lock(&buffer->lock);

    // 2. KOPIRAJ IZ KFIFO U USERSPACE
    retval = kfifo_to_user(fifo, ubuf, count, &copied);
    // buffer2->fifo: ['A'] → ubuf (userspace)
    // buffer2->fifo: []     (sada prazan)

    // 3. SIMULIRAJ DELAY
    simulate_delay(1000);

    // 4. OTKLJUČAJ
    mutex_unlock(&buffer->lock);

    // 5. ★★★ PROBUDI ČEKAJUĆE WRITERE ★★★
    wake_up_all(&shofer->rq);  // Probudi one koji čekaju na čitanje

    return copied;
}
```

### REZULTAT:

```
Reader proces ispisuje:
[shofer2] Pročitano 1 byte(s): 'A'
```

---

## FAZA 8: CIKLUS SE NASTAVLJA

Reader se vraća u `poll()` i ponovno čeka:

```
poll(fds, 4, -1);  → SPAVA dok writer opet ne napiše
```

Writer nakon 1000ms (1 sekunda) ponovno piše:

```
poll(fds, 4, 1000);  → Timeout nakon 1s
→ Piše 'B' na nasumičnu napravu (npr. shofer0)
→ Reader se budi
→ Reader čita 'B' iz shofer0
→ Ispisuje: [shofer0] Pročitano 1 byte(s): 'B'
```

---

## DIJAGRAM VREMENSKE LINIJE

```
TIME  WRITER                        BUFFER           READER
────────────────────────────────────────────────────────────────
0s    Pokreće se                                     Pokreće se
      open(/dev/shofer0-3)                           open(/dev/shofer0-3)
      poll(timeout=1000ms)                           poll(timeout=-1)
                                                      ↓ SPAVA (čeka podatke)

1s    poll() → timeout
      Piše 'A' u /dev/shofer2
      write(fd[2], 'A', 1)
      ↓
      shofer_write()               buffer2 ← ['A']
      kfifo_from_user()
      simulate_delay(1000ms) ────→ DELAY 1s

2s    write() završava             buffer2: ['A']
      wake_up_all(&wq)                               ↓ PROBUDI SE!
                                                      poll() vraća 1
                                                      revents[2] = POLLIN
                                                      read(fd[2])
                                                      ↓
                                                      shofer_read()
                                   buffer2: [] ←──── kfifo_to_user()
                                                      simulate_delay(1s)

3s    poll(timeout=1000ms)                           read() završava
      ↓ ČEKA                                          Ispis: [shofer2] 'A'
                                                      poll(timeout=-1)
                                                      ↓ SPAVA

4s    poll() → timeout
      Piše 'B' u /dev/shofer0      buffer0 ← ['B']
      ...                                            ↓ PROBUDI SE!
                                                     Ispis: [shofer0] 'B'
                                                     ↓ SPAVA

5s    Piše 'C' u /dev/shofer3      buffer3 ← ['C']
      ...                                            ↓ PROBUDI SE!
                                                     Ispis: [shofer3] 'C'
```

---

## KLJUČNI MEHANIZMI

### 1. WAIT QUEUE (čekanje)

```c
// U shofer_dev strukturi:
struct wait_queue_head rq;  // read queue
struct wait_queue_head wq;  // write queue

// Poll dodaje proces u queue:
poll_wait(filp, &shofer->rq, wait);

// write() budi procese:
wake_up_all(&shofer->wq);
```

### 2. POLL MASK (stanje)

```c
unsigned int mask = 0;

if (kfifo_len(fifo))     // Ima podataka?
    mask |= POLLIN;      // → Spremno za READ

if (kfifo_avail(fifo))   // Ima mjesta?
    mask |= POLLOUT;     // → Spremno za WRITE
```

### 3. KFIFO BUFFER

```c
struct kfifo fifo;  // Kernel FIFO buffer

kfifo_from_user();  // Write: user → kernel
kfifo_to_user();    // Read: kernel → user
kfifo_len();        // Koliko podataka ima
kfifo_avail();      // Koliko mjesta ima
```

### 4. MUTEX ZAŠTITA

```c
mutex_lock(&buffer->lock);    // Zaključaj buffer
// ... read/write operacije
mutex_unlock(&buffer->lock);  // Otključaj
```

---

## ZAŠTO JE POLL() EFIKASAN?

**BEZ poll() (aktivno čekanje):**
```c
while (1) {
    ret = read(fd, buf, size);  // Blokira na jednom fd
    if (ret > 0) break;
}
// Problem: Blokira se na PRVOM fd, ne vidi ostale!
```

**SA poll() (event-driven):**
```c
poll(fds, 4, -1);  // Čeka na SVA 4 fd-a istovremeno!
// Kernel USPAVA proces
// Kada BILO KOJI fd dobije podatke → PROBUDI proces
// CPU se NE troši!
```

---

## SAŽETAK TOKA

1. **Modul se učita** → Kreira 6 naprava, 6 bufffera
2. **Reader startuje** → Otvara naprave, poziva `poll()`, SPAVA
3. **Poll() u kernelu** → Registrira reader u wait queues
4. **Writer startuje** → Otvara naprave, čeka 1s
5. **Writer piše** → `write()` stavlja 'A' u buffer2
6. **Kernel provjerava** → buffer2 ima podataka!
7. **Poll() se budi** → Vraća control readeru, `revents[2] = POLLIN`
8. **Reader čita** → `read()` uzima 'A' iz buffer2
9. **Ispis** → `[shofer2] Pročitano 1 byte(s): 'A'`
10. **Ponovno čekanje** → Reader opet `poll()`, writer čeka 1s
11. **GOTO 5** → Ciklus se ponavlja

---

## DODATNO: ŠTO AKO BUFFER POSTANE PUN?

```
Writer piše brže nego reader čita:
buffer0->fifo: [64/64 bytes]  ← PUN!

Writer poziva poll():
→ kfifo_avail(&buffer0->fifo) = 0
→ mask NE setira POLLOUT
→ Writer NE piše u buffer0
→ Odabire drugu napravu koja ima mjesta

Kada reader pročita iz buffer0:
→ kfifo_avail() > 0
→ Poll() vidi POLLOUT za buffer0
→ Writer može opet pisati u buffer0
```

---

To je kompletan tok izvođenja! 🚀
