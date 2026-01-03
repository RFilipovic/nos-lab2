LAB2C - Cjevovod (Pipe) Device Driver
======================================

Opis:
-----
Implementacija cjevovoda kao character device drivera koji omogućava
komunikaciju između više dretvi/procesa putem sučelja za rad s datotekama
(open, close, read, write).

Karakteristike:
---------------
- Podrška za O_RDONLY, O_WRONLY i O_RDWR zastavice pri otvaranju
- FIFO (kružni međuspremnik) za pohranu podataka
- Sinkronizacija čitača i pisača pomoću semafora i mutexa
- Blokiranje dretvi kada nema podataka (read) ili nema mjesta (write)
- Ograničenje veličine podataka i broja istovremenih pristupa

Parametri modula:
-----------------
- pipe_size: Veličina cjevovoda u bajtovima (default: 64)
- max_threads: Maksimalan broj procesa/dretvi koji mogu istovremeno koristiti cjevovod (default: 5)

Primjer pokretanja s argumentima:
sudo ./load_shofer pipe_size=128 max_threads=10

Kompajliranje:
--------------
1. Kompajliranje modula:
   make

2. Kompajliranje test programa:
   cd test
   make

Učitavanje modula:
------------------
sudo ./load_shofer [pipe_size=X] [max_threads=Y]

Uklanjanje modula:
------------------
sudo ./unload_shofer

Testiranje:
-----------
BRZI START:
1. sudo ./test_demo.sh
   (automatski učitava modul i daje upute)

RUČNO TESTIRANJE:
1. U jednom terminalu pokrenite čitača:
   ./test/citac

2. U drugom terminalu pokrenite pisača:
   ./test/pisac

3. Možete pokrenuti više čitača i više pisača istovremeno
   (do max_threads procesa ukupno)

DETALJNE UPUTE:
Pogledaj UPUTE_TESTIRANJE.md za sve testove uključujući:
- Testiranje s više dretvi
- Provjeru kontrole pristupa
- Testiranje blokiranja
- Testiranje prevelikih podataka
- Praćenje kernel logova

Čišćenje:
---------
make clean
cd test && make clean
