/*
 * Napiši program v programskem jeziku C, ki ustvari N otroških procesov, kjer je N podan kot prvi argument, s privzeto vrednostjo 10. 
 * Starš naj 10 sekund izpisuje zvezdice, vsako sekundo eno zvezdico. 
 * Po zaključku izpisa naj počaka na otroke, da se končajo, nato naj izpiše "Samo brez panike" in se konča z izhodnim statusem 42.
 *
 * Otroci naj izpisujejo znake, pri čemer naj i-ti otrok izpisuje znak 'A' + i vsakih 1 + i sekund. 
 * Otroci naj prenehajo z izpisovanjem po 42 sekundah in se končajo z izhodnim statusom i.
 */

------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    int N = 10;
    if (argc > 1)
    {
        N = atoi(argv[1]);
    }

    pid_t pids[N];
    int i;
    for (i = 0; i < N; i++)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            perror("Fork failed");
            exit(1);
        }
        if (pids[i] == 0)
        { // otrok
            int interval = 1 + i;
            char c = 'A' + i;
            for (int j = 0; j < 42 / interval; j++)
            {
                sleep(interval);
                printf("%c\n", c);
            }
            exit(i); // končamo z i otroka
        }
    }

    if (i == N)
    { // Parent process
        for (int sec = 0; sec < 10; sec++)
        {
            sleep(1);
            printf("*\n");
        }

        // čakamo d se končajo otroci
        for (int j = 0; j < N; j++)
        {
            waitpid(pids[j], NULL, 0);
        }

        printf("Samo brez panike\n");
        exit(42);
    }

    return 0;
}