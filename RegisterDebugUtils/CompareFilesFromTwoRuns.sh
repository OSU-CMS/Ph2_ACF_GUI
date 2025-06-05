#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <RunNumber_1> <RunNumber_2> <FilePattern> <chipType>"
    exit 1
fi

# Extract arguments
run_number_1="$1"
run_number_2="$2"
file_pattern="$3"
chipType="$4"

# Set the directory path
directory_1="${PH2ACF_BASE_DIR}/Results/Run_${run_number_1}/"
directory_2="${PH2ACF_BASE_DIR}/Results/Run_${run_number_2}/"


if [ -d "$directory_2" ]; then
    # Change to the directory
    cd "$directory_2" || exit

    for file in ${file_pattern}; do
        # Check if the file exists
        if [ -e "$file" ]; then
            echo "$file" 
            # Use the cat command on each file

            ${PH2ACF_BASE_DIR}/RegisterDebugUtils/RegisterDifference "${directory_1}${file}" "${directory_2}${file}" "$chipType"
            echo "--------"  # Optional separator between file contents
        else
            echo "File not found: $file"
        fi
    done

    cd ..
else
    echo "Directory does not exist: $directory_2"
fi
