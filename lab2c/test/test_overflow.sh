#!/bin/bash
# Test writing data larger than pipe size

echo "Test: Pisanje podataka većih od veličine cijevi"
echo ""

# Try to write 200 bytes to a 64-byte pipe
cat > test_large_write.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define CIJEV "/dev/shofer"

int main() {
    int fp;
    char buffer[200];
    ssize_t ret;

    memset(buffer, 'X', sizeof(buffer));

    fp = open(CIJEV, O_WRONLY);
    if (fp == -1) {
        perror("open");
        return 1;
    }

    printf("Pokušavam upisati 200 bajtova u cijev (veličina cijevi = 64)...\n");
    ret = write(fp, buffer, 200);
    if (ret == -1) {
        printf("✓ Ispravno: write je vratio grešku: %s\n", strerror(errno));
        if (errno == EFBIG) {
            printf("✓ Greška je EFBIG - podatak prevelik!\n");
        }
    } else {
        printf("✗ Neočekivano: write je uspio (%zd bajtova)\n", ret);
    }

    close(fp);
    return 0;
}
EOF

gcc -o test_large_write test_large_write.c
./test_large_write
rm -f test_large_write test_large_write.c

echo ""
echo "Test završen."
