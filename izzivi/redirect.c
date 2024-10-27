/*
 * V programskem jeziku C napišite program, ki omogoča preusmeritev standardnega vhoda in izhoda danega programa. 
 * Uporabimo ga na sledeči način:
 *
 * ./redirect vhod izhod prog arg1 arg2 ...
 *
 * Pri tem je vhod vhodna datoteka, ki jo uporabimo za preusmeritev standardnega vhoda, izhod pa izhodna datoteka na katero preusmerimo standardni izhod. 
 * Če vhod ali izhod enak "-", potem naj se ustrezna preusmeritev ne izvede. Po preusmeritvi naj se zažene dani program z danimi argumenti. 
 * Program naj torej deluje podobno, kot če bi v lupini bash napisali
 *
 * prog arg1 arg2 ... <vhod >izhod
 *
 * Namig: program zaženete s funkcijo execvp(), npr. execvp(argv[3], &argv[3]).
 */

-------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s input output prog arg1 arg2 ...\n", argv[0]);
        return 1;
    }

    // Redirect standard input if input file is specified
    if (strcmp(argv[1], "-") != 0)
    {
        int fd_in = open(argv[1], O_RDONLY);
        if (fd_in < 0)
        {
            perror("Error opening input file");
            return 1;
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    // Redirect standard output if output file is specified
    if (strcmp(argv[2], "-") != 0)
    {
        int fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_out < 0)
        {
            perror("Error opening output file");
            return 1;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    // Execute the program with given arguments
    execvp(argv[3], &argv[3]);
    perror("execvp failed");
    return 1;
}
