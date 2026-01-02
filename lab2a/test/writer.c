/*
 * writer.c - Program koji koristi poll za pisanje podataka u više naprava
 *
 * Otvara sve naprave za pisanje, periodički (svakih 5 sekundi) s poll provjerava
 * je li barem jedna od njih spremna za prihvat novih znakova i ako je nasumice
 * odabire jednu takvu i šalje joj jedan znak.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "shofer_user.h"

#define DEVICE_PATH "/dev/" DRIVER_NAME
#define POLL_INTERVAL_MS 5000  /* 5 sekundi */

int main(int argc, char *argv[])
{
	struct pollfd fds[DRIVER_NUM];
	int num_devices = DRIVER_NUM;
	char device_name[64];
	int i, ret;
	char current_char = 'A';
	int ready_devices[DRIVER_NUM];
	int num_ready;
	int selected;
	int poll_interval = POLL_INTERVAL_MS;

	/* Inicijaliziraj generator slučajnih brojeva */
	srand(time(NULL));

	/* Override broj naprava i interval ako su dani kao argumenti */
	if (argc > 1) {
		num_devices = atoi(argv[1]);
		if (num_devices < 1 || num_devices > DRIVER_NUM) {
			fprintf(stderr, "Broj naprava mora biti između 1 i %d\n", DRIVER_NUM);
			return -1;
		}
	}
	if (argc > 2) {
		poll_interval = atoi(argv[2]);
		if (poll_interval < 100) {
			fprintf(stderr, "Interval mora biti >= 100 ms\n");
			return -1;
		}
	}

	printf("Otvaram %d naprava za pisanje...\n", num_devices);

	/* Otvori sve naprave za pisanje */
	for (i = 0; i < num_devices; i++) {
		snprintf(device_name, sizeof(device_name), "%s%d", DEVICE_PATH, i);
		fds[i].fd = open(device_name, O_WRONLY);
		if (fds[i].fd == -1) {
			perror("open failed");
			fprintf(stderr, "Ne mogu otvoriti %s\n", device_name);
			/* Zatvori već otvorene naprave */
			while (--i >= 0)
				close(fds[i].fd);
			return -1;
		}
		fds[i].events = POLLOUT; /* Čekamo da postane zapisljivo */
		printf("Otvorio %s (fd=%d)\n", device_name, fds[i].fd);
	}

	printf("\nPeriodički šaljem znakove svakih %d ms (Ctrl+C za izlaz)...\n\n",
		poll_interval);

	/* Glavna petlja - periodički provjeri i piši */
	while (1) {
		/* Koristi poll s timeout-om da provjeri spremnost za pisanje */
		ret = poll(fds, num_devices, poll_interval);

		if (ret == -1) {
			if (errno == EINTR)
				continue; /* Prekinuto signalom, nastavi */
			perror("poll failed");
			break;
		}

		if (ret > 0) {
			/* Poll vratio da postoje naprave spremne za pisanje */
			num_ready = 0;

			/* Pronađi sve naprave spremne za pisanje */
			for (i = 0; i < num_devices; i++) {
				if (fds[i].revents & POLLOUT) {
					ready_devices[num_ready++] = i;
				}

				if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
					printf("[%s%d] Greška (ERR=%d HUP=%d NVAL=%d)\n",
						DRIVER_NAME, i,
						!!(fds[i].revents & POLLERR),
						!!(fds[i].revents & POLLHUP),
						!!(fds[i].revents & POLLNVAL));
				}
			}

			if (num_ready > 0) {
				/* Nasumično odaberi jednu od spremnih naprava */
				selected = ready_devices[rand() % num_ready];

				/* Piši jedan znak na odabranu napravu */
				char buffer[2];
				buffer[0] = current_char;
				buffer[1] = '\0';

				ssize_t bytes_written = write(fds[selected].fd, buffer, 1);

				if (bytes_written == 1) {
					printf("[%s%d] Napisao znak '%c' (%d spremnih naprava)\n",
						DRIVER_NAME, selected, current_char, num_ready);

					/* Idi na sljedeći znak (A-Z, onda natrag na A) */
					current_char++;
					if (current_char > 'Z')
						current_char = 'A';
				} else if (bytes_written == 0) {
					printf("[%s%d] Nula bajtova napisano\n", DRIVER_NAME, selected);
				} else {
					perror("write failed");
				}
			} else {
				printf("Poll završio ali nema spremnih naprava za pisanje\n");
			}
		} else {
			/* Timeout - nijedna naprava nije spremna */
			printf("Timeout: Nijedna naprava nije spremna za pisanje\n");
		}
	}

	/* Čišćenje - zatvori sve naprave */
	printf("\nZatvaram naprave...\n");
	for (i = 0; i < num_devices; i++) {
		if (fds[i].fd >= 0)
			close(fds[i].fd);
	}

	return 0;
}
