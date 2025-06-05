# Ph2_ACF overview

## Installation

The installation and setup of the Ph2_ACF software is described in the following steps:

1. [Installing RHEL/AlmaLinux 9](alma9_install.md)
1. [Installing additionally required software](required_install.md)
1. [Installing rarpd](rarpd.md)
1. [Setting up the Ph2_ACF software](ph2acf_install.md)

## Contributing

For the C++ code of Ph2_ACF, `clang-format` is used to format the code according to the `.clang-format` configuration file in the repository. Before creating a merge request, please run:

```shell
formatAll
```

This requires the `setup.sh` to be sourced first.
