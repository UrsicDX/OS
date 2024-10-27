/*
 * Napišite skripto, ki:
 *
 * - ustvari podano število (privzeta vrednost naj bo 100) procesov;
 * - počaka na uporabnikov pritisk poljubne tipke;
 * - vse ustvarjene procese ukine; in
 * - nato počaka, da se vsi procesi končajo.
 *
 * Posamezen proces ustvarite tako, da zaženete npr. program xeyes ali xclock (po potrebi ju namestite), 
 * z verjetnostjo zagona prvega približno 2/3 in drugega 1/3 (uporabite $RANDOM). Izpišite tudi PIDe zagnanih procesov.
 * Primer izpisa:
 *
 * Ustvarjam 5 procesov...
 * PIDs: 20046 20047 20048 20049 20050
 * Ukinjam procese...
 * KILLs: 20050 20049 20048 20047 20046
 * Procesi končani.
 *
 * a) Procese ukinjajte kakor želite.
 *
 * b) Procese ukinjajte v nasprotnem vrstnem redu, kot so bili ustvarjeni. 
 *    Izziv: ne uporabite polja, ampak navaden seznam (PIDi kot niz besed).
 *
 * c) Ob ukinjanju procesov se izpisujejo obvestila o njihovi ukinitvi, npr.
 *
 * ./skripta.sh: line 31: 16730 Killed xclock
 *
 * Utišajte ta obvestila. 
 * Namig: obvestila izpisuje (ob prejemu signala SIGCHLD) lupina - starš, ki je te procese ustvarila. 
 * Torej bo potrebno nekaj spremeniti pri staršu (ali celo ustvariti vmesnega starša).
 *
 * d) Nadgradite c), da sami izpisujete ustrezna obvestila o ukinitvi.
 */
------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Struct to pass multiple arguments to the thread function
typedef struct
{
    int threadNumber;
    int numIterations;
} ThreadArgs;

// Function for threads to print their names 'numIterations' times
void *printThread(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int threadNumber = args->threadNumber;
    int numIterations = args->numIterations;

    for (int i = 0; i < numIterations; i++)
    {
        printf("Thread %d iteration %d\n", threadNumber, i);
        usleep(1000000); // Sleep for 1 second
    }

    pthread_exit(NULL);
}

// Function for threads with infinite loops
void *infiniteThread(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int threadNumber = args->threadNumber;
    int numIterations = args->numIterations;

    volatile int running = 1;
    while (running)
    {
        printf("Thread %d is running\n", threadNumber);
        usleep(1000000); // Sleep for 1 second
        // You could use some logic to stop the thread, e.g., set running to 0 under certain conditions.
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int numThreads = atoi(argv[1]);
    int numIterations = argc > 2 ? atoi(argv[2]) : 5;

    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];

    // a) Creating and starting threads to print their names 'numIterations' times
    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].threadNumber = i;
        threadArgs[i].numIterations = numIterations;
        pthread_create(&threads[i], NULL, printThread, (void *)&threadArgs[i]);
    }

    // Wait for all print threads to finish
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // b) Creating threads with infinite loops
    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].threadNumber = i;
        threadArgs[i].numIterations = numIterations;
        pthread_create(&threads[i], NULL, infiniteThread, (void *)&threadArgs[i]);
    }

    // Sleep for 'numIterations' seconds and then stop the infinite threads
    sleep(numIterations);
    for (int i = 0; i < numThreads; i++)
    {
        pthread_cancel(threads[i]);
    }

    // Wait for all infinite threads to finish
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
