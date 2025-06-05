#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <DefaultFile> <RunNumber>"
    exit 1
fi

# Extract arguments
defaultFile="$1"
run_number="$2"
file_pattern="BE*_Config.xml"

# Set the directory path
directory="${PH2ACF_BASE_DIR}/Results/Run_${run_number}/"

for file in "${directory}"${file_pattern}; do
    # Check if the file exists
    if [ -e "$file" ]; then
        # Use the cat command on each file

        ${PH2ACF_BASE_DIR}/RegisterDebugUtils/BoardRegisterDifference "$defaultFile" "$file"
        echo "--------"  # Optional separator between file contents
    else
        echo "File not found: $file"
    fi
done
