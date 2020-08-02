#include "vault.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_VAULT_NAME    "test_vault.vault"
#define FIRST_ITEM_NAME    "FIRST.ITEM"
#define FIRST_ITEM_CONTENT "This is the content of the first vault item."

int main ()
{
    vault_t vlt;
    vault_header_t hdr;

    fprintf(stdout, "Opening test fault: '%s'\n", TEST_VAULT_NAME);

    if (vault_open(&vlt, TEST_VAULT_NAME, WRITE) != VAULT_ESUCCESS) {
        fprintf(stderr, "Error opening vault\n");
        return -1;
    }

    fprintf(stdout, "Writing first item.\n  '%s':'%s'\n", FIRST_ITEM_NAME, FIRST_ITEM_CONTENT);

    memset(&hdr, 0, sizeof(hdr));
    memcpy(&hdr.item_name, FIRST_ITEM_NAME, strlen(FIRST_ITEM_NAME));

    if (vault_write_header(&vlt, &hdr) != VAULT_ESUCCESS) {
        fprintf(stderr, "Error writing vault\n");
        return -1;
    }

    if (vault_write_data(&vlt, FIRST_ITEM_CONTENT, strlen(FIRST_ITEM_CONTENT)) != VAULT_ESUCCESS) {
        fprintf(stderr, "Error writing vault\n");
        return -1;
    }

    fclose(vlt.file_stream);

    return 0;
}
