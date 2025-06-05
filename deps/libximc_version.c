#include <stdio.h>

#include "ximc.h"


int main(void) {
    char buf[64];
    ximc_version(buf);
    printf("%64s", ximc_version);
    return 0;
}
