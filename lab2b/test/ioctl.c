/* jednostavan program koji koristi ioctl za slanje naredbe zadanoj datoteci */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <asm/ioctl.h>

#include "../config.h" /* format trećeg argumenta za ioctl */

int main(int argc, char *argv[])
{
	int fd, count;
	unsigned long request, num;
	struct shofer_ioctl cmd;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s file-name ioctl-command\n", argv[0]);
		return -1;
	}

	num = atol(argv[2]);
	if (num < 1 || num > 100) {
		fprintf(stderr, "Usage: %s file-name ioctl-command\n", argv[0]);
		fprintf(stderr, "ioctl-command must be a number from {1,100}\n");
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open failed");
		return -1;
	}

	/* kreiraj zahtjev */
	request = _IOC(_IOC_WRITE, SHOFER_IOCTL_TYPE, SHOFER_IOCTL_NR, sizeof(struct shofer_ioctl));

	/* naredba (COPY) i vrijednost count prosljeđuju se u strukturi shofer_ioctl kao treći argument ioctl-a */
	cmd.command = SHOFER_IOCTL_COPY;
	cmd.count = num;
	
	count = ioctl(fd, request, (unsigned long) &cmd);
	if (count == -1) {
		perror("ioctl error");
		return -1;
	}

	printf("ioctl returned %d\n", count);

	return 0;
}
