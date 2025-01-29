#include "rate_limit.h"
#include <string.h>
#include <stdio.h>

static RateLimitEntry rate_limits[MAX_RATE_LIMITS];
static int rate_limit_count = 0;

void init_rate_limiter(void) {
    rate_limit_count = 0;
    printf("[Rate Limiter] Initialized\n");
    fflush(stdout);
}

int check_rate_limit(const char *ip) {
    time_t now = time(NULL);
    
    // Find existing rate limit entry
    for (int i = 0; i < rate_limit_count; i++) {
        if (strcmp(rate_limits[i].ip, ip) == 0) {
            // Check if IP is blocked
            if (rate_limits[i].is_blocked) {
                if (now < rate_limits[i].block_until) {
                    // If they're still trying while blocked, extend the block duration
                    if (rate_limits[i].attempts >= MAX_ATTEMPTS) {
                        time_t current_block = rate_limits[i].block_until - now;
                        time_t extended_block = current_block * 2;
                        rate_limits[i].block_until = now + extended_block;
                        rate_limits[i].attempts = 0; // Reset attempts for next block
                        
                        printf("[Rate Limiter] IP %s block extended to %ld seconds for repeated attempts\n", 
                               ip, extended_block);
                        fflush(stdout);
                    }
                    
                    long remaining = rate_limits[i].block_until - now;
                    printf("[Rate Limiter] IP %s is blocked for %ld more seconds\n", 
                           ip, remaining);
                    fflush(stdout);
                    rate_limits[i].attempts++; // Count attempts during block
                    return 0;
                } else {
                    // Unblock IP after duration
                    rate_limits[i].is_blocked = 0;
                    rate_limits[i].attempts = 0;
                    rate_limits[i].timestamp = now;
                    printf("[Rate Limiter] IP %s block period expired, unblocking\n", ip);
                    fflush(stdout);
                    return 1;
                }
            }

            // Reset if window expired
            if (difftime(now, rate_limits[i].timestamp) > ATTEMPT_WINDOW) {
                rate_limits[i].attempts = 1;
                rate_limits[i].timestamp = now;
                printf("[Rate Limiter] Reset window for IP %s\n", ip);
                fflush(stdout);
                return 1;
            }
            
            // Check if limit exceeded and block if necessary
            if (rate_limits[i].attempts >= MAX_ATTEMPTS) {
                rate_limits[i].is_blocked = 1;
                rate_limits[i].block_until = now + BLOCK_DURATION;
                printf("[Rate Limiter] IP %s blocked for %d seconds\n", 
                       ip, BLOCK_DURATION);
                fflush(stdout);
                return 0;
            }
            
            // Increment attempts
            rate_limits[i].attempts++;
            printf("[Rate Limiter] Attempt %d/%d for IP %s\n", 
                   rate_limits[i].attempts, MAX_ATTEMPTS, ip);
            fflush(stdout);
            return 1;
        }
    }
    
    // Add new rate limit entry
    if (rate_limit_count < MAX_RATE_LIMITS) {
        strncpy(rate_limits[rate_limit_count].ip, ip, sizeof(rate_limits[rate_limit_count].ip) - 1);
        rate_limits[rate_limit_count].timestamp = now;
        rate_limits[rate_limit_count].attempts = 1;
        rate_limits[rate_limit_count].is_blocked = 0;
        rate_limits[rate_limit_count].block_until = 0;
        printf("[Rate Limiter] New entry for IP %s\n", ip);
        fflush(stdout);
        rate_limit_count++;
        return 1;
    }
    
    // If we get here, we're out of rate limit slots
    printf("[Rate Limiter] Warning: Rate limit storage full\n");
    fflush(stdout);
    return 1;
}

void record_attempt(const char *ip, int success) {
    if (success) {
        for (int i = 0; i < rate_limit_count; i++) {
            if (strcmp(rate_limits[i].ip, ip) == 0) {
                printf("[Rate Limiter] Successful attempt from IP %s, resetting counter\n", ip);
                fflush(stdout);
                rate_limits[i].attempts = 0;
                rate_limits[i].timestamp = time(NULL);
                break;
            }
        }
    }
}

void print_rate_limit_status(const char *ip) {
    time_t now = time(NULL);
    for (int i = 0; i < rate_limit_count; i++) {
        if (strcmp(rate_limits[i].ip, ip) == 0) {
            if (rate_limits[i].is_blocked) {
                long remaining = rate_limits[i].block_until - now;
                printf("[Rate Limiter] Status for IP %s: BLOCKED for %ld more seconds\n",
                       ip, remaining);
            } else {
                printf("[Rate Limiter] Status for IP %s: %d attempts, window started %ld seconds ago\n",
                       ip, rate_limits[i].attempts, 
                       now - rate_limits[i].timestamp);
            }
            fflush(stdout);
            return;
        }
    }
    printf("[Rate Limiter] No entry found for IP %s\n", ip);
    fflush(stdout);
}
