#ifndef RATE_LIMIT_H
#define RATE_LIMIT_H

#include <time.h>

// Constants for rate limiting
#define MAX_RATE_LIMITS 1000
#define MAX_ATTEMPTS 5
#define ATTEMPT_WINDOW 3600  // 1 hour in seconds
#define BLOCK_DURATION 300   // 5 minutes blocking period

// Rate limit entry structure
struct RateLimitEntry {
    char ip[46];
    time_t timestamp;
    time_t block_until;
    int attempts;
    int is_blocked;
};

typedef struct RateLimitEntry RateLimitEntry;

// Function declarations
void init_rate_limiter(void);
int check_rate_limit(const char *ip);
void record_attempt(const char *ip, int success);
void print_rate_limit_status(const char *ip);

#endif // RATE_LIMIT_H
