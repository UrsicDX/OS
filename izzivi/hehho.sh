# Napiši skripto, ki Izpiše vsi argumenti skripte, vsak argument v svojo vrstico. 
# Vrstice naj bodo oštevilčene, nad znaki pa naredite preslikavo: a → ha, e → he, i → hi, o → ho in u → hu. 
# Primer izvajanja:

# ➔ ./hehho.sh Operacijski sistemi so megaaaa!
# 0: Opherhachijskhi
# 1: shisthemhi
# 2: sho
# 3: mheghahahaha!

# Posebno pozornost posvetite presledkom v argumentih. 
# Transformacijo nad znaki lahko izvedete z ukazom sed.

----------------------------------------------------------

#!/bin/bash

# Function to replace characters according to the specified mapping
map_characters() {
    local input="$1"
    echo "${input}" | sed -e 's/a/ha/g' -e 's/e/he/g' -e 's/i/hi/g' -e 's/o/ho/g' -e 's/u/hu/g'
}

for (( i=1; i<=$#; i++ )); do
    mapped_arg=$(map_characters "${!i}")
    echo "$((i-1)): $mapped_arg"
done

