#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include "user.h"

static struct User users[MAX_USERS];
static int user_count = 0;

void generate_salt(unsigned char *salt) {
    RAND_bytes(salt, SALT_LEN);
}

void hash_password(const char *password, const unsigned char *salt, char *hashed, size_t hashed_size) {
    size_t pass_len = strlen(password);
    size_t total_len = SALT_LEN + pass_len;
    unsigned char *combined = malloc(total_len);
    
    memcpy(combined, salt, SALT_LEN);
    memcpy(combined + SALT_LEN, password, pass_len);
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, combined, total_len);
    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);
    
    memset(hashed, 0, hashed_size);
    for (unsigned int i = 0; i < hash_len; i++) {
        snprintf(hashed + (i * 2), 3, "%02x", hash[i]);
    }
    
    free(combined);
}

void save_users_to_file(void) {
    FILE *fp = fopen("users.json", "w");
    if (fp == NULL) return;
    
    fprintf(fp, "{\n  \"users\": [\n");
    for (int i = 0; i < user_count; i++) {
        char salt_hex[SALT_LEN * 2 + 1] = {0};
        for (int j = 0; j < SALT_LEN; j++) {
            snprintf(salt_hex + (j * 2), 3, "%02x", users[i].salt[j]);
        }
        fprintf(fp, "    {\"username\": \"%s\", \"password\": \"%s\", \"salt\": \"%s\", \"is_admin\": %d}%s\n",
                users[i].username, users[i].password, salt_hex, users[i].is_admin,
                i < user_count - 1 ? "," : "");
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp);
}

void load_users_from_file(void) {
    FILE *fp = fopen("users.json", "r");
    if (fp == NULL) return;
    
    char line[512];
    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN], salt_hex[SALT_LEN * 2 + 1];
    user_count = 0;
    
    while (fgets(line, sizeof(line), fp) && user_count < MAX_USERS) {
        if (sscanf(line, "    {\"username\": \"%63[^\"]\", \"password\": \"%127[^\"]\", \"salt\": \"%32[^\"]\", \"is_admin\": %d}", 
                   username, password, salt_hex, &users[user_count].is_admin) == 4) {
            strncpy(users[user_count].username, username, MAX_USERNAME_LEN - 1);
            strncpy(users[user_count].password, password, MAX_PASSWORD_LEN - 1);
            
            for (int i = 0; i < SALT_LEN; i++) {
                sscanf(salt_hex + (i * 2), "%2hhx", &users[user_count].salt[i]);
            }
            user_count++;
        }
    }
    fclose(fp);
}

int add_user(const char *username, const char *password) {
    if (user_count >= MAX_USERS) {
        printf("Error: Storage full!\n");
        return 0;
    }
    
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("Error: Username already exists!\n");
            return 0;
        }
    }
    
    generate_salt(users[user_count].salt);
    char hashed_password[65] = {0};
    hash_password(password, users[user_count].salt, hashed_password, sizeof(hashed_password));
    
    strncpy(users[user_count].username, username, MAX_USERNAME_LEN - 1);
    strncpy(users[user_count].password, hashed_password, MAX_PASSWORD_LEN - 1);
    user_count++;
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char created_at[22];
    strftime(created_at, sizeof(created_at), "%Y-%m-%d %H:%M:%S", t);
    printf("[New User Created] Username: %s [Created At]: %s [Hash]: %s\n", 
           username, created_at, hashed_password);
    
    save_users_to_file();
    return 1;
}

int check_auth(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            char hashed_password[65] = {0};
            hash_password(password, users[i].salt, hashed_password, sizeof(hashed_password));
            if (strcmp(users[i].password, hashed_password) == 0) {
                return users[i].is_admin ? 2 : 1;
            }
            break;
        }
    }
    return 0;
}

int promote_to_admin(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            users[i].is_admin = 1;
            save_users_to_file();
            printf("[Admin Promotion] User %s promoted to admin\n", username);
            return 1;
        }
    }
    return 0;
}
