# Ph2_ACF software installation

Follow these instructions to install and compile the Ph2_ACF software.

## Cloning the repository

```shell
git clone --recurse-submodules https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF.git
# N.B. to synchronize only the submodule, run:
# git submodule sync; git submodule update --init --recursive
cd Ph2_ACF
# Check out the desired tag (replace v4-XX accordingly)
git checkout -b v4-XX v4-XX
source setup.sh
mkdir build
cd build
cmake .. # add -D CMAKE_BUILD_TYPE=Debug if you plan to use gdb for debugging
```

## Building the software

Run `make -jN` in the `build` directory or alternatively do
`make -C build/ -jN` in the `Ph2_ACF` root directory.

## Everytime setup

Don't forget to `source setup.sh` to set all the environment variables correctly whenever you open a new shell.

## Test installation

```shell
fpgaconfig --help
```
