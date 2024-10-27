#Napiši skripto, ki ji podamo enega ali več argumentov. Skripta naj za vse argumente od drugega naprej izpiše, ali so enaki ali ne prvemu.
---------------------------------------

prvi_arg="$1"

for arg in "${@:2}"; do
    if [ "$arg" == "$prvi_arg" ]; then
        echo "$arg je enak $prvi_arg"
    else
        echo "$arg ni enak $prvi_arg"
    fi
done
