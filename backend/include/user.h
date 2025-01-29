#ifndef USER_H
#define USER_H

#define MAX_USERS 100
#define MAX_USERNAME_LEN 64
#define MAX_PASSWORD_LEN 128
#define SALT_LEN 16
#define HASH_LEN 32

struct User {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    unsigned char salt[SALT_LEN];
    int is_admin;
};

void generate_salt(unsigned char *salt);
void hash_password(const char *password, const unsigned char *salt, char *hashed, size_t hashed_size);
void save_users_to_file(void);
void load_users_from_file(void);
int add_user(const char *username, const char *password);
int check_auth(const char *username, const char *password);
int promote_to_admin(const char *username);

#endif // USER_H
