#ifndef PASSMNGR_H
#define PASSMNGR_H

// Functions for setting up a terminal for password input
// As a password manager it is important to get password input correct
//int setup_password_input(int stream);
//int teardown_password_input(int stream);
//int read_password(int stream, size_t max, char* pwd_buffer);

enum state {
    IDLE,
    VAULT_OPEN,
    VAULT_UNLOCKED
}

struct passmngr {
    enum state state;
    char* vault_name;
    
}


#endif