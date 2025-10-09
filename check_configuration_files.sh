#!/bin/bash
REQUIRED_VERSION=6 # Will need to change this by hand

# Loop over all files passed as arguments
for FILE in "$@"; do
    if [[ ! -f "$FILE" ]]; then
        echo "❌ File not found: $FILE"
        exit 1
    fi

    # Try to find a CONFIG_VER line using regex
    VERSION_LINE=$(grep -E '^\s*CONFIG_VER\s*=?\s*' "$FILE" | head -n1)

    if [[ -z "$VERSION_LINE" ]]; then
        echo "❌ CONFIG_VER not found in $FILE"
        exit 1
    fi

    # Extract numeric version (assumes format like: CONFIG_VER = 1 or CONFIG_VER=1)
    ACTUAL_VERSION=$(echo "$VERSION_LINE" | grep -oE '[0-9]+')

    if [[ "$ACTUAL_VERSION" != "$REQUIRED_VERSION" ]]; then
        echo "❌ Version mismatch in $FILE"
        echo "Expected: $REQUIRED_VERSION, Found: $ACTUAL_VERSION"
        exit 1
    else
        echo "✅ $FILE has correct version: $ACTUAL_VERSION"
    fi
done
