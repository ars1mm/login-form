#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "mongoose.h"
#include "server.h"

#define MAX_USERS 100
#define MAX_USERNAME_LEN 64
#define MAX_PASSWORD_LEN 128
#define SALT_LEN 32
#define HASH_LEN 32

struct User {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    unsigned char salt[SALT_LEN];
};

static struct User users[MAX_USERS];
static int user_count = 0;

static void generate_salt(unsigned char *salt) {
    RAND_bytes(salt, SALT_LEN);
}

static void hash_password(const char *password, const unsigned char *salt, char *hashed, size_t hashed_size) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, salt, SALT_LEN);
    EVP_DigestUpdate(mdctx, password, strlen(password));
    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);

    memset(hashed, 0, hashed_size);
    for (unsigned int i = 0; i < hash_len; i++) {
        snprintf(hashed + (i * 2), 3, "%02x", hash[i]);
    }
}

static void save_users_to_file(void) {
    FILE *fp = fopen("users.json", "w");
    if (fp == NULL) return;
    
    fprintf(fp, "{\n  \"users\": [\n");
    for (int i = 0; i < user_count; i++) {
        char salt_hex[SALT_LEN * 2 + 1] = {0};
        for (int j = 0; j < SALT_LEN; j++) {
            snprintf(salt_hex + (j * 2), 3, "%02x", users[i].salt[j]);
        }
        fprintf(fp, "    {\"username\": \"%s\", \"password\": \"%s\", \"salt\": \"%s\"}%s\n",
                users[i].username, users[i].password, salt_hex,
                i < user_count - 1 ? "," : "");
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp);
}

static void load_users_from_file(void) {
    FILE *fp = fopen("users.json", "r");
    if (fp == NULL) return;
    
    char line[512];
    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN], salt_hex[SALT_LEN * 2 + 1];
    user_count = 0;
    
    while (fgets(line, sizeof(line), fp) && user_count < MAX_USERS) {
        if (sscanf(line, "    {\"username\": \"%63[^\"]\", \"password\": \"%127[^\"]\", \"salt\": \"%32[^\"]\"}", 
                   username, password, salt_hex) == 3) {
            strncpy(users[user_count].username, username, MAX_USERNAME_LEN - 1);
            strncpy(users[user_count].password, password, MAX_PASSWORD_LEN - 1);
            
            // Convert salt from hex to bytes
            for (int i = 0; i < SALT_LEN; i++) {
                sscanf(salt_hex + (i * 2), "%2hhx", &users[user_count].salt[i]);
            }
            user_count++;
        }
    }
    fclose(fp);
}

static int add_user(const char *username, const char *password) {
    //Check if the user count is at the maximum, if so return 0
    if (user_count >= MAX_USERS) return 0;
    
    //Check if the username already exists, if so return 0
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) return 0;
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
    strftime(created_at, sizeof(created_at) - 1, "%Y-%m-%d %H:%M:%S", t);
    printf("[New User Created] Username: %s [Created At]: %s [Hash]: %s\n", username, created_at, hashed_password);
    save_users_to_file();
    return 1;
}

static int check_auth(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            char hashed_password[65] = {0};
            hash_password(password, users[i].salt, hashed_password, sizeof(hashed_password));
            if (strcmp(users[i].password, hashed_password) == 0) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
            mg_http_reply(c, 200,
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n", "");
            return;
        }

        if (mg_strcmp(hm->uri, mg_str("/signup")) == 0) {
            char username[64], password[64];
            struct mg_str json = hm->body;
            
            char *user = mg_json_get_str(json, "$.username");
            char *pass = mg_json_get_str(json, "$.password");
            
            if (user && pass) {
                strncpy(username, user, sizeof(username) - 1);
                strncpy(password, pass, sizeof(password) - 1);
                free(user);
                free(pass);
                
                int success = add_user(username, password);
                
                mg_http_reply(c, success ? 200 : 400,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"success\": %s, \"message\": \"%s\"}\n",
                    success ? "true" : "false",
                    success ? "User created" : "Username already exists or storage full");
            } else {
                mg_http_reply(c, 400,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"success\": false, \"message\": \"Invalid JSON format\"}\n");
            }
            return;
        }

        if (mg_strcmp(hm->uri, mg_str("/login")) == 0) {
            char username[64], password[64];
            struct mg_str json = hm->body;
            
            char *user = mg_json_get_str(json, "$.username");
            char *pass = mg_json_get_str(json, "$.password");
            
            if (user && pass) {
                strncpy(username, user, sizeof(username) - 1);
                strncpy(password, pass, sizeof(password) - 1);
                free(user);
                free(pass);
                
                int auth_success = check_auth(username, password);
                
                mg_http_reply(c, auth_success ? 200 : 401,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"success\": %s}\n",
                    auth_success ? "true" : "false");
            } else {
                mg_http_reply(c, 400,
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n",
                    "{\"success\": false, \"message\": \"Invalid JSON format\"}\n");
            }
            return;
        }

        mg_http_reply(c, 404,
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n",
            "{\"success\": false, \"error\": \"Not found\"}\n");
    }
}

void setup_server(const char *url) {
    load_users_from_file();
    printf("Loaded %d users from storage\n", user_count);
    printf("Server starting on %s\n", url);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    struct mg_connection *c = mg_http_listen(&mgr, url, fn, NULL);
    if (c == NULL) return;

    while (1) mg_mgr_poll(&mgr, 1000);
    
    mg_mgr_free(&mgr);
}
