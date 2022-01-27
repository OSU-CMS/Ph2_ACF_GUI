# Project abstract

This project is meant to develop a generic c++ application to pilot several power supply types used for IT and OT testing.

# Doxygen documentation

Here is the link of the authomatically generated Doxygen main web page:

https://ancasses.web.cern.ch/ancasses/private/cms/DevicesCode/

# Clone instructions

Since April 2021 Networkutils submodule has been added. When you clone the repository don't forget it and please add the recursive submodule flag, i.e.:

`git clone --recurse-submodules XXX`

where XXX is your clone link.

If you already cloned the project and forgot the `--recurse-submodules`, you can run:

`git submodule update --init --recursive`

# Building instructions

## Prerequisite

In order to be able to compile you need the following libraries installed:
* **BOOST**. You can installed them from the repository: `sudo yum install boost-devel`
* **PUGIXML** (https://pugixml.org/). You can installed them from the repository: `sudo yum install pugixml-devel`

### CAEN prerequisite
In order to use the CAEN-flavour power supplies it is necessary to install the following libraries:
* **CAENHVWRAPPER** https://www.caen.it/download/?filter=CAEN%20HV%20Wrapper%20Library
* **CAENGeco** https://www.caen.it/download/?filter=GECO2020

## Building


* In the project folder create a subfolder "build" for example `mkdir build`
* Navigate to that `cd build`
* Run `cmake ../`
* Run `make` or `make -jN` where N is the number of core you would like to use for compiling

# CLANG-FORMAT instructions

## CLANG installation

Instructions of clang-installation here.

## vim

There are many possibilities to integrate clang-format with your vim. Here following a list of tested and untested possibilities

### Tested

#### vim-autoformat

For a detailed view of the plugin, please refer to the original page: https://github.com/Chiel92/vim-autoformat .
Here following some basic instructions on how to install and use it:
1. Use you favourite PluginIn installer to install the plugin. For example:
    * *VAM* (https://github.com/MarcWeber/vim-addon-manager). Add this line to your .vimrc configuration file `VAMActivate git:https://github.com/Chiel92/vim-autoformat`
    * *vim-plug* (https://github.com/junegunn/vim-plug). Add the github repository (https://github.com/Chiel92/vim-autoformat) inside your call of vim plugins (usually between `call plug#begin('~/.vim/plugged')` and `call plug#end()`)
    * *Vundle* . Put this in your .vimrc `Plugin 'Chiel92/vim-autoformat'`
    * *Others using github*. Link to the vim/plug repository (https://github.com/Chiel92/vim-autoformat).
2. Close vim and reopen it in order to let him install the plugin.
3. Add to your .vimrc the wanted options:
    * `let g:autoformat_verbosemode=1`  suggested to have a verbose output on automatic format call. It helps for debugging.
    * `au BufWrite * :Autoformat` if you want Autoformat to be authomatically executed on save.
    * `noremap <F3> :Autoformat<CR>` if you want (for example) to have Autoformat executed with F3 button press. You can map to whatever else.
4. Beware of what formatter is used for your file and eventually change it using: the following syntax `let g:formatterpath = ['/some/path/to/a/folder', '/home/superman/formatters']` (more details on the original web page).

### Untested

* http://clang.llvm.org/docs/ClangFormat.html#vim-integration
* https://github.com/rhysd/vim-clang-format
* https://github.com/cjuniet/clang-format.vim

# Known issues

## Serial communicaion

When using serial communication the program has to be executed with root privileges (sudo ...). Alternatively, you should change the access privileges of your USB port.

### How to mount USB device with custom name

Using as reference https://unix.stackexchange.com/questions/66901/how-to-bind-usb-device-under-a-static-name you can do something like this:
* Edit the file /etc/udev/rules.d/10-local.rules with something like `ACTION=="add", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", SYMLINK+="myDevice"`, where
    * idVendor and idProduct can be found by running `udevadm info -a -p  $(udevadm info -q path -n /dev/ttyUSB0)` (`ttyUSB0` should be properly changed)
    * myDevice can be whatever name you choose and you device will appear under /dev/myDevice
* Reboot your system or execute `udevadm control --reload-rules && udevadm trigger`
