#!/bin/sh
export PATH=$PATH:$PWD/bin
export PATH=$PATH:$PWD/scripts
export POWER_SUPPLY_PATH=$PWD

# Clang-format command
if command -v clang-format &> /dev/null; then
  clang_command="clang-format" 
else
  clang_command="/opt/rh/llvm-toolset-7.0/root/usr/bin/clang-format"
fi

if [[ -z "$PH2ACF_BASE_DIR" ]]; then
  alias formatAll="find ${POWER_SUPPLY_PATH} -type d -name NetworkUtils -prune -false -o -iname '*.h' -o -iname '*.cc' | xargs ${clang_command} -i"
else
  alias formatAll="find ${PH2ACF_BASE_DIR} -type d -name NetworkUtils -prune -false -o -iname '*.h' -o -iname '*.cc' | xargs ${clang_command} -i"
fi
