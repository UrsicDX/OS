/*
 * Napišite program v C, ki izpiše vsebino podane datoteke (prvi argument, privzeta vrednost /etc/shadow).
 *
 * Pri tem naj se vsebina izpiše ne glede dovoljenja trenutnega uporabnika za branje te datoteke.
 * Preden se izpiše vsebina datoteke, naj program še izpiše realni uid in aktualni uid (real and effective uid).
 */

// to make it run we use this 2 commands:
// sudo chown root:root program
// sudo chmod u+s program

----------------------------------------------------------------

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    const char *filename = "/etc/shadow";
    if (argc > 1)
    {
        filename = argv[1];
    }

    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Error open");
        return EXIT_FAILURE;
    }

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    if (bytes_read == -1)
    {
        perror("Error read");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);
    return EXIT_SUCCESS;
}
