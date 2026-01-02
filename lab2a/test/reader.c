/*
 * reader.c - Program koji koristi poll za čitanje podataka iz više naprava
 *
 * Otvara sve naprave za čitanje, s poll čeka da se na bilo kojoj pojavi znak,
 * čita ga i ispisuje.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#include "shofer_user.h"

#define DEVICE_PATH "/dev/" DRIVER_NAME

int main(int argc, char *argv[])
{
	struct pollfd fds[DRIVER_NUM];
	int num_devices = DRIVER_NUM;
	char device_name[64];
	int i, ret;
	char buffer[64];

	/* Override broj naprava ako je dan kao argument */
	if (argc > 1) {
		num_devices = atoi(argv[1]);
		if (num_devices < 1 || num_devices > DRIVER_NUM) {
			fprintf(stderr, "Broj naprava mora biti između 1 i %d\n", DRIVER_NUM);
			return -1;
		}
	}

	printf("Otvaram %d naprava za čitanje...\n", num_devices);

	/* Otvori sve naprave za čitanje */
	for (i = 0; i < num_devices; i++) {
		snprintf(device_name, sizeof(device_name), "%s%d", DEVICE_PATH, i);
		fds[i].fd = open(device_name, O_RDONLY);
		if (fds[i].fd == -1) {
			perror("open failed");
			fprintf(stderr, "Ne mogu otvoriti %s\n", device_name);
			/* Zatvori već otvorene naprave */
			while (--i >= 0)
				close(fds[i].fd);
			return -1;
		}
		fds[i].events = POLLIN; /* Čekamo da postane čitljivo */
		printf("Otvorio %s (fd=%d)\n", device_name, fds[i].fd);
	}

	printf("\nČekam na podatke (Ctrl+C za izlaz)...\n\n");

	/* Glavna petlja - čekaj i čitaj podatke */
	while (1) {
		/* Koristi poll da čeka na bilo koju napravu */
		ret = poll(fds, num_devices, -1); /* -1 = beskonačno čekanje */

		if (ret == -1) {
			if (errno == EINTR)
				continue; /* Prekinuto signalom, nastavi */
			perror("poll failed");
			break;
		}

		if (ret > 0) {
			/* Provjeri sve naprave */
			for (i = 0; i < num_devices; i++) {
				if (fds[i].revents & POLLIN) {
					/* Naprava je spremna za čitanje */
					ssize_t bytes_read = read(fds[i].fd, buffer, sizeof(buffer) - 1);

					if (bytes_read > 0) {
						buffer[bytes_read] = '\0';
						printf("[%s%d] Pročitano %zd byte(s): '%s'\n",
							DRIVER_NAME, i, bytes_read, buffer);
					} else if (bytes_read == 0) {
						printf("[%s%d] EOF\n", DRIVER_NAME, i);
					} else {
						perror("read failed");
					}
				}

				if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
					printf("[%s%d] Greška na napraviERR=%d HUP=%d NVAL=%d)\n",
						DRIVER_NAME, i,
						!!(fds[i].revents & POLLERR),
						!!(fds[i].revents & POLLHUP),
						!!(fds[i].revents & POLLNVAL));
				}
			}
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
