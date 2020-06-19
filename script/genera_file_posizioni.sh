# genera il file posizioni in formato txt in modo random, passando il numero di righe che si vuole sia lungo
#!/bin/bash

args=("$@")

SCRIPT_NAME=$(basename ${BASH_SOURCE[0]})

if [ "$#" != 1 ]; then
    echo -e "\e[31mERROR: $SCRIPT_NAME prende 1 paramentro: numero_righe \e[0m"
    return
fi

NUMERO_RIGHE=${args[0]}

echo "<$SCRIPT_NAME> Creazione file_posizioni.txt..."

rm -f input/file_posizioni.txt

for (( row = 0; row < NUMERO_RIGHE; row++ ))
do
    pos=""
    for (( d = 0; d < 5; d++ ))
    do
        rand_row=$(((RANDOM % 10)))
        rand_col=$(((RANDOM % 10)))

        pos+="${rand_row},${rand_col}|"
    
    done
    pos=${pos:0:${#pos}-1}
    echo "${pos}" >> input/file_posizioni.txt
done
echo "<$SCRIPT_NAME> OK."