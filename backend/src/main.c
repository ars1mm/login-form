#include <stdio.h>
#include "mongoose.h"
#include "server.h"

int main(void) {
    setup_server("0.0.0.0:8080");
    return 0;
}