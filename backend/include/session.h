#ifndef SESSION_H
#define SESSION_H

#include <time.h>

#define SESSION_TOKEN_LEN 64
#define MAX_SESSIONS 1000
#define SESSION_EXPIRE_TIME (60 * 60)  // 1 hour in seconds

typedef struct {
    char token[SESSION_TOKEN_LEN];
    char username[64];
    time_t created_at;
    time_t last_used;
    int is_admin;
} Session;

void init_session_manager(void);
char* create_session(const char* username, int is_admin);
int validate_session(const char* token);
void cleanup_expired_sessions(void);
void remove_session(const char* token);

#endif // SESSION_H
