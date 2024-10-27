# Pošiljanje signalov
# Napišite bash skripto sigsend.sh, ki pošilja signale. Proces, ki mu pošiljamo signale, podamo kot prvi argument skripte:
#
# - če je prvi argument število (lahko preverite le prvi znak), potem gre za PID procesa;
# - sicer gre za ime procesa (poiščite proces).
# Skripta naj se v zanki odziva na naslednje tipke:
#
# x - izhod iz programa;
# h - izpis pomoči;
# w - pošlje signal SIGHUP (končanje pripadajočega terminala);
# i - SIGINT (končanje kot Ctrl+C);
# q - SIGQUIT (končanje procesa, Ctrl+/);
# t - SIGTERM (končanje procesa);
# k - SIGKILL (brezpogojno končanje procesa);
# c - SIGCONT (nadaljevanje izvajanja procesa);
# s - SIGSTOP (brezpogojna ustavitev izvajanja procesa);
# z - SIGTSTP (ustavitev izvajanja procesa kot Ctrl-Z);
# 1 - SIGUSR1 (1. uporabniški signal);
# 2 - SIGUSR2 (2. uporabniški signal); in
# y - SIGCHLD (končanje otroka).
#
# Pred pošiljanjem signala preverite, če prejemnik še obstaja in če ne, potem naj se skripta konča. 
# Izpišite tudi obvestilo o uspehu pošiljanja signala.
#
# Lovec signalov - za oddajo
# Napišite bash skripto lovec.sh (lahko pa tudi program lovec.c v Cju), ki se odziva na signale iz predhodne naloge. 
# Skripta naj najprej izpiše svoj PID. Nato naj se izvaja, dokler je njena energija večja od nič, začetna energija naj bo podana kot prvi argument (privzeta vrednost 42). 
# Vsako sekundo naj izpiše znak "." oz."*" in zmanjša energijo za ena. Ko se konča, naj izpiše obvestilo.
#
# - Če prejme signal SIGTERM, naj se energija poveča za 10 in izpiše obvestilo. Skripta pa naj se NE konča.
#
# - Če prejme signal SIGUSR1, naj se zamenja način izpisa. Če se izpisujejo ".", potem naj se sedaj izpisujejo "*" in obratno.
#
# - Če prejme signal SIGUSR2, naj se ustvari nov proces. Starš naj izpiše PID novonastalega otroka. Otrok naj zaspi za (energija mod 7) + 1 sekund 
# in se konča z izhodnim statusom enakim (42 * energija) mod 128. Otrok naj ob končanju izpiše svoj izhodni status.
#
# Poskrbite za morebitne zombije. Za vsakega pokončanega zombija starš izpiše njegov izhodni status. 
# Tisti, ki boste pisali skripto v bash-u, boste opazili, da od otroka ne morete dobiti izhodnega statusa, ker ukaz wait tega ne omogoča.

------------------------------------------------------------------------------

#!/bin/bash

# Default energy level
energy=${1:-42}
output_char='.'

# Output the PID of this script
echo "PID of this script: $$"
echo "Starting with energy: $energy"

# Function to handle SIGTERM
handle_sigterm() {
    echo "SIGTERM, bonus enery jihaa; current energy is $energy!"
    ((energy += 10))
}

# Function to toggle output between '.' and '*'
toggle_output() {
    if [[ "$output_char" == '.' ]]; then
        output_char='*'
    else
        output_char='.'
    fi
    echo "Toggled output character"
}

# Function to handle SIGUSR2
spawn_child() {
    local child_pid
    ( 
        sleep $((energy % 7 + 1))
        local exit_status=$((42 * energy % 128))
        echo "Child ($$) finishing with exit status: $exit_status"
        exit $exit_status
    ) &
    child_pid=$!
    echo "Spawned child with PID: $child_pid"
}

# Trap signals
trap 'handle_sigterm' SIGTERM
trap 'toggle_output' SIGUSR1
trap 'spawn_child' SIGUSR2

# Main loop
while (( energy > 0 )); do
    echo -n "$output_char"
    sleep 1
    ((energy--))
done

echo "No more energy :("

# Clean up any zombie processes
while IFS= read -r pid; do
    if wait "$pid" 2>/dev/null; then
        echo "Child $pid exited with status $?"
    fi
done < <(jobs -p)

