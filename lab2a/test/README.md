# Lab2a - Korištenje operacije poll

## Pregled

Implementacija dva programa koji demonstriraju korištenje `poll()` sistemskog poziva za sinkronu I/O operaciju na višestrukim character device napravama.

## Datoteke

- **reader.c** - Program koji čita podatke s naprava koristeći poll
- **writer.c** - Program koji piše podatke na naprave koristeći poll
- **shofer_user.h** - Zajedničke definicije za userspace programe
- **Makefile** - Build skripta za kompilaciju programa
- **test_upute.txt** - Detaljne upute za testiranje

## Implementacija

### Reader Program (`reader.c`)

Program otvara sve naprave za **čitanje** i koristi `poll()` za čekanje na podatke.

**Funkcionalnost:**
1. Otvara sve naprave (`/dev/shofer0` do `/dev/shofer5`) u read-only modu
2. Konfigurira `pollfd` strukturu za svaku napravu s `POLLIN` flagom
3. U beskonačnoj petlji:
   - Poziva `poll()` s timeout-om `-1` (beskonačno čekanje)
   - Kada poll vrati da je naprava spremna za čitanje
   - Čita jedan znak iz te naprave
   - Ispisuje znak i broj naprave

**Primjer korištenja:**
```bash
./reader          # Koristi svih 6 naprava
./reader 3        # Koristi samo 3 naprave
```

### Writer Program (`writer.c`)

Program otvara sve naprave za **pisanje** i periodički šalje znakove.

**Funkcionalnost:**
1. Otvara sve naprave u write-only modu
2. Konfigurira `pollfd` strukturu za svaku napravu s `POLLOUT` flagom
3. U beskonačnoj petlji:
   - Poziva `poll()` s timeout-om od 5000ms (5 sekundi)
   - Pronalazi sve naprave spremne za pisanje
   - **Nasumično** odabire jednu od spremnih naprava
   - Piše jedan znak na tu napravu (A-Z ciklički)

**Primjer korištenja:**
```bash
./writer              # 6 naprava, interval 5000ms
./writer 3            # 3 naprave, interval 5000ms
./writer 3 2000       # 3 naprave, interval 2000ms
```

## Kernel modul shofer.c

Modul već ima implementiranu `poll` operaciju:

```c
static unsigned int shofer_poll(struct file *filp, poll_table *wait)
{
    struct shofer_dev *shofer = filp->private_data;
    struct buffer *buffer = shofer->buffer;
    struct kfifo *fifo = &buffer->fifo;
    unsigned int len = kfifo_len(fifo);
    unsigned int avail = kfifo_avail(fifo);
    unsigned int mask = 0;

    poll_wait(filp, &shofer->rq, wait);
    poll_wait(filp, &shofer->wq, wait);

    if (len)
        mask |= POLLIN | POLLRDNORM;  /* readable */
    if (avail)
        mask |= POLLOUT | POLLWRNORM; /* writable */

    return mask;
}
```

**Kako radi:**
- `poll_wait()` dodaje proces u wait queue (`rq` za read, `wq` za write)
- Vraća masku koja indicira da li je naprava spremna za čitanje/pisanje
- `POLLIN` - postoje podaci za čitanje (fifo nije prazan)
- `POLLOUT` - ima mjesta za pisanje (fifo nije pun)

## Kompilacija

```bash
# Kompilacija testnih programa
cd /home/rene/Documents/nos-lab2/lab2a/test
make

# Kompilacija kernel modula
cd /home/rene/Documents/nos-lab2/lab2a
make
```

## Testiranje

### 1. Učitaj modul
```bash
cd /home/rene/Documents/nos-lab2/lab2a
sudo ./load_shofer
```

### 2. Terminal 1 - Pokreni reader
```bash
cd /home/rene/Documents/nos-lab2/lab2a/test
./reader
```

### 3. Terminal 2 - Pokreni writer
```bash
cd /home/rene/Documents/nos-lab2/lab2a/test
./writer
```

### 4. Očekivani rezultat

**Writer terminal:**
```
Otvaram 6 naprava za pisanje...
Otvorio /dev/shofer0 (fd=3)
Otvorio /dev/shofer1 (fd=4)
...
[shofer3] Napisao znak 'A' (6 spremnih naprava)
[shofer1] Napisao znak 'B' (6 spremnih naprava)
[shofer5] Napisao znak 'C' (6 spremnih naprava)
```

**Reader terminal:**
```
Otvaram 6 naprava za čitanje...
Otvorio /dev/shofer0 (fd=3)
Otvorio /dev/shofer1 (fd=4)
...
Čekam na podatke (Ctrl+C za izlaz)...

[shofer3] Pročitano 1 byte(s): 'A'
[shofer1] Pročitano 1 byte(s): 'B'
[shofer5] Pročitano 1 byte(s): 'C'
```

### 5. Ukloni modul
```bash
sudo ./unload_shofer
```

## Ključni koncepti

### poll() sistemski poziv

`poll()` omogućava programu da čeka na više file descriptora istovremeno:

```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

- **fds** - niz `pollfd` struktura
- **nfds** - broj elemenata u nizu
- **timeout** - vrijeme čekanja u milisekundama (-1 = beskonačno)

### pollfd struktura

```c
struct pollfd {
    int   fd;       /* file descriptor */
    short events;   /* zahtijevani eventi (POLLIN, POLLOUT) */
    short revents;  /* vraćeni eventi */
};
```

### Eventi

- **POLLIN** - Podaci dostupni za čitanje
- **POLLOUT** - Spremno za pisanje
- **POLLERR** - Greška
- **POLLHUP** - Hangup
- **POLLNVAL** - Nevažeći file descriptor

## Arhitektura sustava

```
┌─────────┐         poll()          ┌──────────────┐
│ reader  ├─────────────────────────>│ /dev/shofer0 │
│         │                          ├──────────────┤
│         │                          │ /dev/shofer1 │
│         │                          ├──────────────┤
│         │                          │     ...      │
└─────────┘                          ├──────────────┤
                                     │ /dev/shofer5 │
                                     └──────────────┘
                                            ^
                                            │
┌─────────┐         poll()                 │
│ writer  ├─────────────────────────────────┘
│         │
│         │
└─────────┘
```

Modul:
- **6 shofer_dev struktura** (naprava)
- **6 buffer struktura** (kfifo bufferi)
- Naprave povezane s bufferima u round-robin modu
- Svaki buffer ima wait queue za read i write operacije

## Reference

- [poll(2) man page](https://man7.org/linux/man-pages/man2/poll.2.html)
- Linux Device Drivers, Third Edition (Chapter 6: Advanced Char Driver Operations)
