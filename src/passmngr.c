#include <stdio.h>
#include <time.h>
#include <string.h>

#include <passmngr.h>

#define MAX_UNLOCK_TIME (600)

int main (int argc, char* argv[]) {
    guarded_t password = {0};
    vault_t vault = {0};
    int exit_status = EXIT_SUCCESS;

    // Verify that a db file path was provided in the command line
    if (argc < 2) {
        printf("Usage: %s db_file\n", argv[0]);
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    const char* const db_file = argv[1];

    if (sodium_init() < 0) {
        printf("Failed to initialize libsodium\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    printf("Master Password > ");
    guarded_result_t gr = read_user_password();
    if (IS_ERROR(gr)) {
        printf("Failed to read user password\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    password = UNWRAP(gr);

    nil_result_t nr = guarded_read_only(password);
    if (IS_ERROR(nr)) {
        printf("Failed to mprotect allocated memory\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    vault_result_t vr = new_vault(db_file, password);
    if (IS_ERROR(vr)) {
        printf("Failed to initialize a new vault\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    vault = UNWRAP(vr);

    password = sodium_free_slice(password);

    time_t start_time = time(NULL);
    // Process commands
    char command_buffer[1024] = {0};

    while (difftime(time(NULL), start_time) < MAX_UNLOCK_TIME) {
        // wipe the command buffer
        memset(command_buffer, 0, sizeof (command_buffer));
        // Print Prompt
        printf("[cmd]> ");
        // read line
        if (fgets(command_buffer, sizeof (command_buffer), stdin) == NULL) {
            // Somehow handle this case
            printf("Unknown command, or invalid input\n");
        } else {
            slice cmd_slice = slice_ptr(command_buffer, strlen(command_buffer), sizeof (command_buffer));
            slice trimmed = trim_space(cmd_slice);
            printf("Command: ");
            print_slice_as_string(trimmed);
            printf("\n");
        }
    }
    // Print prompt,
    // Read input line 
    // Print result or prompt for a password
    // repeat
cleanup:
    sodium_free_slice(password);
    destroy_vault(&vault);
    exit(exit_status);
}

#include <termios.h>
#include <unistd.h>

#define MAX_PASSWORD_SIZE (512)
#define MIN_PASSWORD_SIZE (8)

guarded_result_t read_user_password() {
    static struct termios old_terminal;
    static struct termios new_terminal;

    guarded_t password = {0};
    guarded_result_t result = {0};

    //get settings of the actual terminal
    tcgetattr(STDIN_FILENO, &old_terminal);

    // do not echo the characters
    new_terminal = old_terminal;
    new_terminal.c_lflag &= ~(ECHO);

    // set this as the new terminal options
    tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);

    guarded_result_t gr = sodium_alloc_slice(MAX_PASSWORD_SIZE);
    if (IS_ERROR(gr)) {
        result = ERROR(guarded_result_t, -1);
        goto cleanup;
    }
    password = UNWRAP(gr);

    // the user can add chars and delete if he puts it wrong
    // the input process is done when he hits the enter
    // the \n is stored, we replace it with \0
    if (fgets(password.ptr, password.cap, stdin) == NULL) {
        password = set_slice_len(password, 0);
    } else {
        password = set_slice_len(password, strlen(password.ptr));
        if (password.len > 1 && ((char*)password.ptr)[password.len-1] == '\n') {
            if (password.len > 2 && ((char*)password.ptr)[password.len-2] == '\r') {
                password = set_slice_len(password, password.len - 2);
            }
        }

        if (password.len < MIN_PASSWORD_SIZE) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;            
        }
    }

    result = VALUE(guarded_result_t, password);
    goto success;

cleanup:
    sodium_free_slice(password);
success:
    // Put back old terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);

    return result;
}