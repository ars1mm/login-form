#include "session.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/rand.h>

static Session sessions[MAX_SESSIONS];
static int session_count = 0;

void init_session_manager(void) {
    session_count = 0;
    printf("[Session Manager] Initialized\n");
    fflush(stdout);
}

static void generate_token(char* token) {
    unsigned char rand_bytes[32];
    RAND_bytes(rand_bytes, sizeof(rand_bytes));
    
    for (int i = 0; i < sizeof(rand_bytes); i++) {
        sprintf(token + (i * 2), "%02x", rand_bytes[i]);
    }
}

char* create_session(const char* username, int is_admin) {
    cleanup_expired_sessions();
    
    if (session_count >= MAX_SESSIONS) {
        printf("[Session Manager] Error: Max sessions reached\n");
        return NULL;
    }
    
    Session* session = &sessions[session_count++];
    generate_token(session->token);
    strncpy(session->username, username, sizeof(session->username) - 1);
    session->created_at = time(NULL);
    session->last_used = session->created_at;
    session->is_admin = is_admin;
    
    printf("[Session Manager] Created session for %s\n", username);
    return session->token;
}

int validate_session(const char* token) {
    time_t now = time(NULL);
    
    for (int i = 0; i < session_count; i++) {
        if (strcmp(sessions[i].token, token) == 0) {
            if (now - sessions[i].last_used > SESSION_EXPIRE_TIME) {
                printf("[Session Manager] Session expired for %s\n", sessions[i].username);
                return 0;
            }
            sessions[i].last_used = now;
            return sessions[i].is_admin ? 2 : 1;  // 2 for admin, 1 for user
        }
    }
    return 0;
}

void cleanup_expired_sessions(void) {
    time_t now = time(NULL);
    int write_idx = 0;
    
    for (int i = 0; i < session_count; i++) {
        if (now - sessions[i].last_used <= SESSION_EXPIRE_TIME) {
            if (write_idx != i) {
                sessions[write_idx] = sessions[i];
            }
            write_idx++;
        } else {
            printf("[Session Manager] Removing expired session for %s\n", sessions[i].username);
        }
    }
    
    session_count = write_idx;
}

void remove_session(const char* token) {
    for (int i = 0; i < session_count; i++) {
        if (strcmp(sessions[i].token, token) == 0) {
            printf("[Session Manager] Removing session for %s\n", sessions[i].username);
            if (i < session_count - 1) {
                memmove(&sessions[i], &sessions[i + 1], 
                        (session_count - i - 1) * sizeof(Session));
            }
            session_count--;
            return;
        }
    }
}
