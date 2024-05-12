#!/bin/bash

# Funcția pentru a verifica conținutul fișierului și a determina dacă este periculos
check_file_content() {
    local FILE="$1"

    # Ensure file exists and is a file
    if [ ! -f "$FILE" ]; then
        exit 1
    fi

    # Ensure file is not empty (if it is, it's safe)
    if [ ! -s "$FILE" ]; then
        echo "SAFE"
        return
    fi

    local IS_SUSPECT=0

    # Count amount of lines, words and characters in file
    local LINES=$(wc -l < "$FILE")
    local WORDS=$(wc -w < "$FILE")
    local CHARS=$(wc -m < "$FILE")

    # If there's less than 3 lines, more than 1000 words and more than 2000 characters, file is suspect
    if [ "$LINES" -lt 3 ] && [ "$WORDS" -gt 1000 ] && [ "$CHARS" -gt 2000 ]; then
        IS_SUSPECT=1
    fi

    if [ "$IS_SUSPECT" -eq 1 ]; then
        local IS_MALICIOUS=0

        # Check if the file has non-ascii words
        local TR_RESULT=$(tr -d -c '[:print:]\n' < "$FILE")

        if [ "$TR_RESULT" != "$(cat "$FILE")" ]; then
            IS_MALICIOUS=1
        fi

        # Check if the file has any of the following words: corrupted, dangerous, risk, attack, malware, malicious
        if egrep -q "corrupted|dangerous|risk|attack|malware|malicious" "$FILE"; then
            IS_MALICIOUS=1
        fi

        if [ "$IS_MALICIOUS" -eq 1 ]; then
            echo "$FILE"
            return
        fi
    fi

    echo "SAFE"
}

# Variabila pentru directorul de ieșire
output_dir=""

# Variabila pentru directorul izolat
isolated_script_dir=""

# Lista pentru directorii specificați
directories=()

# Parcurgem toate argumentele date în linia de comandă
for arg in "$@"; do
    case "$arg" in
        -o)
            # Dacă argumentul este "-o", următorul argument este directorul de ieșire
            shift
            output_dir="$1"
            ;;
        -s)
            # Dacă argumentul este "-s", următorul argument este directorul izolat
            shift
            isolated_script_dir="$1"
            ;;
        *)
            # Dacă argumentul nu este una dintre opțiunile specifice, este un director
            # Îl adăugăm la lista de directoare
            directories+=("$arg")
            ;;
    esac
done

# Verificăm dacă directorul de ieșire și directorul izolat sunt specificate
if [ -z "$output_dir" ] || [ -z "$isolated_script_dir" ]; then
    echo "Usage: $0 [directories...] -o output -s isolatedScript"
    exit 1
fi

# Parcurgem toate directoarele date ca argument
for directory in "${directories[@]}"; do
    # Verificăm dacă directorul există și conține fișiere
    if [ -d "$directory" ] && [ "$(ls -A "$directory")" ]; then
        # Parcurgem fișierele din director
        for file in "$directory"/*; do
            # Verificăm dacă fișierul este un fișier regulat
            if [ -f "$file" ]; then
                # Apelăm funcția pentru verificarea conținutului fișierului
                check_file_content "$file"
            fi
        done
    fi
done

echo "Done"
