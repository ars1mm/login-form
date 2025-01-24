#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "mongoose.h"
#include "server.h"

#define MAX_USERS 100
#define MAX_USERNAME_LEN 64
#define MAX_PASSWORD_LEN 128

struct User {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
};

static struct User users[MAX_USERS];
static int user_count = 0;

static void hash_password(const char *password, char *hashed, size_t hashed_size) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("sha256");
    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);
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
        fprintf(fp, "    {\"username\": \"%s\", \"password\": \"%s\"}%s\n",
                users[i].username, users[i].password,
                i < user_count - 1 ? "," : "");
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp);
}

static void load_users_from_file(void) {
    FILE *fp = fopen("users.json", "r");
    if (fp == NULL) return;
    
    char line[512];
    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN];
    user_count = 0;
    
    while (fgets(line, sizeof(line), fp) && user_count < MAX_USERS) {
        if (sscanf(line, "    {\"username\": \"%63[^\"]\", \"password\": \"%127[^\"]\"}",
                   username, password) == 2) {
            strncpy(users[user_count].username, username, MAX_USERNAME_LEN - 1);
            strncpy(users[user_count].password, password, MAX_PASSWORD_LEN - 1);
            user_count++;
        }
    }
    fclose(fp);
}

static int add_user(const char *username, const char *password) {
    if (user_count >= MAX_USERS) return 0;
    
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) return 0;
    }
    
    char hashed_password[65] = {0};
    hash_password(password, hashed_password, sizeof(hashed_password));
    
    if (strlen(hashed_password) != 64) return 0;
    
    strncpy(users[user_count].username, username, MAX_USERNAME_LEN - 1);
    strncpy(users[user_count].password, hashed_password, MAX_PASSWORD_LEN - 1);
    user_count++;
    
    save_users_to_file();
    return 1;
}

static int check_auth(const char *username, const char *password) {
    char hashed_password[65] = {0};
    hash_password(password, hashed_password, sizeof(hashed_password));
    
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, hashed_password) == 0) {
            return 1;
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

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    struct mg_connection *c = mg_http_listen(&mgr, url, fn, NULL);
    if (c == NULL) return;

    while (1) mg_mgr_poll(&mgr, 1000);
    
    mg_mgr_free(&mgr);
}
