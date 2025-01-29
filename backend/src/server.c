#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mongoose.h"
#include "server.h"
#include "rate_limit.h"
#include "user.h"
#include "utils.h"
#include "session.h"

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    char ip[46] = {0};
    
    if (c != NULL) {
        mg_ntoa(&c->rem, ip, sizeof(ip));
    }

    switch (ev) {
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            if (hm == NULL || ev_data == NULL) {
                printf("[%ld] Warning: Null HTTP message received from %s\n", (long)time(NULL), ip);
                return;
            }

            // Handle OPTIONS requests first
            if (mg_strcmp(hm->uri, mg_str("/signup")) == 0 || 
                mg_strcmp(hm->uri, mg_str("/login")) == 0 || 
                mg_strcmp(hm->uri, mg_str("/admin/users")) == 0 || 
                mg_strcmp(hm->uri, mg_str("/admin/promote")) == 0) {
                
                if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
                    mg_http_reply(c, 200,
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                        "Access-Control-Allow-Headers: Content-Type\r\n", 
                        "");
                    return;
                }

                // Verify we have a POST request with valid JSON
                if (mg_strcmp(hm->method, mg_str("POST")) != 0) {
                    mg_http_reply(c, 405,
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n",
                        "{\"success\": false, \"message\": \"Method not allowed\"}\n");
                    return;
                }

                if (hm->body.len == 0) {
                    mg_http_reply(c, 400,
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n",
                        "{\"success\": false, \"message\": \"Empty request body\"}\n");
                    return;
                }
            }

            // Now handle each endpoint
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
                    record_attempt(ip, success);
                    
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
            }
            else if (mg_strcmp(hm->uri, mg_str("/login")) == 0) {
                print_rate_limit_status(ip); // Add debug output
                if (!check_rate_limit(ip)) {
                    mg_http_reply(c, 429,
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n",
                        "{\"success\": false, \"message\": \"Too many attempts. Please try again later.\"}\n");
                    return;
                }

                char username[64], password[64];
                struct mg_str json = hm->body;
                
                char *user = mg_json_get_str(json, "$.username");
                char *pass = mg_json_get_str(json, "$.password");
                
                if (user && pass) {
                    strncpy(username, user, sizeof(username) - 1);
                    strncpy(password, pass, sizeof(password) - 1);
                    free(user);
                    free(pass);
                    
                    int auth_result = check_auth(username, password);
                    record_attempt(ip, auth_result > 0);
                    
                    if (auth_result > 0) {
                        char* session_token = create_session(username, auth_result == 2);
                        if (session_token) {
                            mg_http_reply(c, 200,
                                "Content-Type: application/json\r\n"
                                "Access-Control-Allow-Origin: *\r\n",
                                "{\"success\": true, \"is_admin\": %s, \"token\": \"%s\"}\n",
                                auth_result == 2 ? "true" : "false",
                                session_token);
                        } else {
                            mg_http_reply(c, 500,
                                "Content-Type: application/json\r\n"
                                "Access-Control-Allow-Origin: *\r\n",
                                "{\"success\": false, \"message\": \"Session creation failed\"}\n");
                        }
                    } else {
                        mg_http_reply(c, 401,
                            "Content-Type: application/json\r\n"
                            "Access-Control-Allow-Origin: *\r\n",
                            "{\"success\": false, \"message\": \"Invalid credentials\"}\n");
                    }
                }
            }
            else if (mg_strcmp(hm->uri, mg_str("/logout")) == 0) {
                struct mg_str *token_header = mg_http_get_header(hm, "X-Session-Token");
                if (token_header) {
                    char token[SESSION_TOKEN_LEN + 1] = {0};
                    snprintf(token, sizeof(token), "%.*s", (int)token_header->len, token_header->ptr);
                    remove_session(token);
                    mg_http_reply(c, 200,
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n",
                        "{\"success\": true}\n");
                } else {
                    mg_http_reply(c, 400,
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n",
                        "{\"success\": false, \"message\": \"No session token provided\"}\n");
                }
            }
            break;
        }
    }
}

static struct mg_mgr mgr;

void setup_server(const char *url) {
    load_users_from_file();
    init_rate_limiter();
    init_session_manager();  // Initialize session management
    printf("[%ld] Loaded users from storage\n", (long)time(NULL));
    printf("[%ld] Server starting on %s\n", (long)time(NULL), url);
    fflush(stdout);

    mg_mgr_init(&mgr);
    struct mg_connection *c = mg_http_listen(&mgr, url, fn, NULL);
    
    if (c == NULL) {
        printf("[%ld] Failed to start server on %s\n", (long)time(NULL), url);
        fflush(stdout);
        mg_mgr_free(&mgr);
        return;
    }
    
    printf("[%ld] Server successfully started\n", (long)time(NULL));
    fflush(stdout);

    while (1) {
        mg_mgr_poll(&mgr, 1000);
    }
}

void stop_server() {
    mg_mgr_free(&mgr);
}
