#include "mongoose.h"
#include <arpa/inet.h>
#include "utils.h"

void mg_ntoa(struct mg_addr* addr, char* buf, size_t len) {
    if (addr->is_ip6) {
        inet_ntop(AF_INET6, &addr->ip, buf, len);
    } else {
        inet_ntop(AF_INET, &addr->ip, buf, len);
    }
}
