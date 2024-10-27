# Napišite skripto, ki oponaša preprosto delovanje ukaza top. 
# Vsako sekundo naj izpiše 10 procesov, ki imajo največjo izrabo procesorja (najvišjo vrednost %cpu). 
# Pomagajte si z ukazom ps in stikalom -o za nastavitev izpisa.

# Izpisujejo naj se naslednji podatki:
# PID (pid);
# ime procesa (comm);
# poraba pomnilnika (%mem);
# lastnik procesa (user); in
# zasedenost cpu (%cpu).

# Program naj se odziva na naslednje tipke:
# q - izhod iz programa (quit);
# h - izpiše pomoč in čaka na pritisk tipke (help);
# c - menjava vidnosti izpisa ukaza (command toggle display);
# m - menjava vidnosti izpisa porabe pomnilnika (memory toggle display);
# p - menjava vidnosti izpisa zasedenosti cpu (cpu toggle display); in
# u - menjava izpisa uporabnika (user toggle display).

# Menjava vidnosti izpisa pomeni, da če se npr. trenutno izpisuje PID, potem se ob menjavi ne izpisuje več in obratno.

---------------------------------------

#!/bin/bash

###so it looks at least ok dispalyed
show_pid=1
show_comm=1
show_mem=1
show_user=1
show_cpu=1

clear

display_help() {
    echo "Usage:"
    echo "q - Quit the program."
    echo "h - Display this help message."
    echo "c - Toggle the visibility of the command (process name) column."
    echo "m - Toggle the visibility of the memory usage column."
    echo "p - Toggle the visibility of the CPU usage column."
    echo "u - Toggle the visibility of the user column."
    read -n 1 -s 
}

while true; do
    clear
    echo -n "PID "
    [[ $show_user -eq 1 ]] && echo -n "USER "
    [[ $show_cpu -eq 1 ]] && echo -n "%CPU "
    [[ $show_mem -eq 1 ]] && echo -n "%MEM "
    [[ $show_comm -eq 1 ]] && echo -n "COMMAND"
    echo

    ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu | head -n 11 | awk -v pid=$show_pid -v user=$show_user -v cpu=$show_cpu -v mem=$show_mem -v comm=$show_comm 'NR>1 {
        if(pid) printf "%-6s ", $1;
        if(user) printf "%-10s ", $2;
        if(cpu) printf "%-6s ", $3;
        if(mem) printf "%-6s ", $4;
        if(comm) printf "%s", $5;
        printf "\n";
    }'

    read -n 1 -s -t 1 key

    case $key in
        q) break ;;
        h) display_help ;;
        c) show_comm=$((1-show_comm)) ;;
        m) show_mem=$((1-show_mem)) ;;
        p) show_cpu=$((1-show_cpu)) ;;
        u) show_user=$((1-show_user)) ;;
    esac
done

