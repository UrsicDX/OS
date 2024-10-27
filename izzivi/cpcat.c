/*
 * V programskem jeziku C napišite program cpcat.c, ki ga uporabljamo na naslednji način:
 *
 * ./cpcat vhod izhod
 *
 * Prvi argument je vhodna datoteka. Če je argument znak "-" ali če ni podan, se privzame standardni vhod. 
 * Drugi argument je izhodna datoteka, če argument ni podan se privzame standardni izhod. Program naj kopira vhodno datoteko v izhodno.
 *
 * Pri izvedbi uporabljajte le ovojne funkcije sistemskih klicev open(), close(), read(), write() in exit().
 *
 * Program napišite programersko estetsko dovršeno, npr. zanko, ki kopira podatke napišite le enkrat - ne uprabljate copy&paste za vsak način uporabe argumentov.
 *
 * Prav tako preverjajte izhodne statuse sistemskih klicev in izpisujte obvestila o morebitnih napakah s perror(). Program naj v primeru napake tudi vrne izhodni status, 
 * ki je enak kodi napake iz GNU standardne C knjižnice.
 *
 * Primeri izvajanja:
 * ./cpcat a.txt b.txt ... kopiranje iz a.txt v b.txt
 * ./cpcat a.txt ... izpis datoteke a.txt
 * ./cpcat - b.txt ... kopiranje stdin v b.txt
 * ./cpcat ... kopiranje stdin v stdout
 */

-----------------------------------------------------------------------------

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 4096

void copy_data(int fd_in, int fd_out)
{
    ssize_t read_bytes, write_bytes;
    char buffer[BUFFER_SIZE];

    while ((read_bytes = read(fd_in, buffer, BUFFER_SIZE)) > 0)
    {
        write_bytes = write(fd_out, buffer, read_bytes);
        if (write_bytes != read_bytes)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    if (read_bytes == -1)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int fd_in = STDIN_FILENO;
    int fd_out = STDOUT_FILENO;

    if (argc > 1 && strcmp(argv[1], "-") != 0)
    {
        fd_in = open(argv[1], O_RDONLY);
        if (fd_in == -1)
        {
            perror("input err:");
            exit(EXIT_FAILURE);
        }
    }

    if (argc > 2)
    {
        fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1)
        {
            perror("output err:");
            exit(EXIT_FAILURE);
        }
    }

    copy_data(fd_in, fd_out);

    if (fd_in != STDIN_FILENO)
        close(fd_in);
    if (fd_out != STDOUT_FILENO)
        close(fd_out);

    return 0;
}
