#include "passmngr.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>

#define PASSWORD_CHUNK_SIZE (32)
#define DEFAULT_CHUNK_SIZE (8192)

#if defined(__WIN_32) || defined(__WIN_64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>

int getch() {
	struct termios o_term, n_term;
	int ch;

	if (tcgetattr(STDIN_FILENO, &o_term) != 0) {
		return -1;
	}
	n_term = o_term;
	n_term.c_lflag &= ~(ICANON | ECHO);
	if (tcsetattr(STDIN_FILENO, TCSANOW, &n_term) != 0) {
		return -1;
	}
	ch = getchar();
	if (tcsetattr(STDIN_FILENO, TCSANOW, &o_term) != 0) {
		return -1;
	}

	return ch;
}
#endif

#include <sodium.h>

int main2 (int argc, char** argv) {
	/* The sequence for initializing the password manager is:
		- Init Sodium 
		- Allocate a locked buffer for the password and master encryption key
		- read the password into guarded allocation and g
		- all data will be encrypted (chunked) with crypto_secretstream with a configurable rekey period
		- The data will be stored in one large file
	*/

	// These items are sensitive and need to be put in guarded allocations
	unsigned char* password_buffer = NULL;
	unsigned char* master_key = NULL;
	unsigned char** file_keys = NULL;

	// This is not a sensitive allocation, it can be allocated with calloc, etc.
	char** file_names = NULL;
	unsigned char salt[crypto_pwhash_SALTBYTES];

	size_t password_buffer_size = PASSWORD_CHUNK_SIZE;
	size_t password_length = 0;
	size_t master_key_size = crypto_secretstream_xchacha20poly1305_KEYBYTES;

	int errcode = 0;

	//struct rlimit limits;

	// Initialize libsodium, failure here is an unrecoverable error
	if (sodium_init() != 0) {
		printf("An error occured starting libsodium, exiting...\n");
		return 1;
	}

	password_buffer = sodium_allocarray(password_buffer_size, sizeof (unsigned char));
	master_key = sodium_allocarray(master_key_size, sizeof (unsigned char));
	if (password_buffer == NULL || master_key == NULL) {
		fprintf(stderr, "An error occured allocating locked memory, exiting...\n");
		errcode = 1;
		goto cleanup;
	}

	fprintf(stdout, "Password: ");
	for (;;) {
		int ch = getch();
		if (ch < 0) {
			fprintf(stderr, "An error occured reading user password, exiting...\n");
			errcode = 1;
			goto cleanup;
		} else if (ch == '\n') {
			break;
		} else {
			if (password_length >= password_buffer_size) {
				size_t new_password_buffer_size = password_buffer_size + PASSWORD_CHUNK_SIZE;
				char* tmp = sodium_allocarray(new_password_buffer_size, sizeof (unsigned char));
				if (tmp == NULL) {
					fprintf(stderr, "An error occured allocating locked memory, exiting...\n");
					errcode = 1;
					goto cleanup;
				}
				memcpy(tmp, password_buffer, password_buffer_size);
				sodium_free(password_buffer);
				password_buffer = tmp;
				password_buffer_size = new_password_buffer_size;
			}

			password_buffer[password_length] = (unsigned char)ch;
			password_length += 1;
		}
	}

	randombytes_buf(salt, sizeof salt);

	// The password needs to be turned into the master key
	if (crypto_pwhash(master_key, master_key_size, password_buffer, password_length, salt,
    crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
		fprintf(stderr, "An error occured generating the master key, exiting...\n");
		errcode = 1;
		goto cleanup;
	}

	// The password is no longer needed it can be removed from memory
	sodium_free(password_buffer);
	password_buffer = NULL;
	password_length = 0;
	password_buffer_size = 0;

	printf("Generated Key: ");
	for (int i = 0; i < master_key_size; i++) {
		printf("%x ", master_key[i]);
	}
	printf("\n");

cleanup:
	sodium_free(password_buffer);
	sodium_free(master_key);
	sodium_free(file_keys);

	free(file_names);

	return errcode;
}
