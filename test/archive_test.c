#include <stdio.h>
#include <string.h>

#include <archive.h>

#define T_F(X) ((X) ? "true" : "false")

int main () {
    char file_name[] = "test_archive.ar";
    const char item_0_name[] = "item0";
    const char item_1_name[] = "item1";
    const char item_2_name[] = "item2";

    const char item_0_content[] = "This is the contents of Item 0";
    const char item_1_content[] = "This is the contents of Item 1";
    const char item_2_content[] = "This is the contents of Item 2";
    char item_read_buffer[50] = {0};

    archive_result_t ar = archive_open(file_name, WRITE);
    archive_t archive = UNWRAP(ar);

    item_result_t ir = archive_item_open(&archive, (const byte_t*)item_0_name, strlen(item_0_name));
    archive_item_t item = UNWRAP(ir);
    size_result_t sr = archive_item_write(&item, (const byte_t*)item_0_content, strlen(item_0_content));
    printf("Wrote %lu bytes to item '%s'\n", UNWRAP(sr), item_0_name);
    archive_item_close(&item);

    ir = archive_item_open(&archive, (const byte_t*)item_1_name, strlen(item_1_name));
    item = UNWRAP(ir);
    sr = archive_item_write(&item, (const byte_t*)item_1_content, strlen(item_1_content));
    printf("Wrote %lu bytes to item '%s'\n", UNWRAP(sr), item_1_name);
    archive_item_close(&item);

    ir = archive_item_open(&archive, (const byte_t*)item_2_name, strlen(item_2_name));
    item = UNWRAP(ir);
    sr = archive_item_write(&item, (const byte_t*)item_2_content, strlen(item_2_content));
    printf("Wrote %lu bytes to item '%s'\n", UNWRAP(sr), item_2_name);
    archive_item_close(&item);

    archive_close(&archive);

    ar = archive_open(file_name, READ);
    archive = UNWRAP(ar);

    ir = archive_item_open(&archive, (const byte_t*)item_0_name, strlen(item_0_name));
    item = UNWRAP(ir);
    memset(item_read_buffer, 0, 50);
    sr = archive_item_read(&item, (byte_t*)item_read_buffer, 50);
    printf("Read %lu bytes from item '%s'\n", UNWRAP(sr), item_0_name);
    printf("Read matched written: %s\n", T_F(strcmp(item_0_content, item_read_buffer) == 0));
    archive_item_close(&item);

    ir = archive_item_open(&archive, (const byte_t*)item_1_name, strlen(item_1_name));
    item = UNWRAP(ir);
    memset(item_read_buffer, 0, 50);
    sr = archive_item_read(&item, (byte_t*)item_read_buffer, 50);
    printf("Read %lu bytes from item '%s'\n", UNWRAP(sr), item_1_name);
    printf("Read matched written: %s\n", T_F(strcmp(item_1_content, item_read_buffer) == 0));
    archive_item_close(&item);

    ir = archive_item_open(&archive, (const byte_t*)item_2_name, strlen(item_2_name));
    item = UNWRAP(ir);
    memset(item_read_buffer, 0, 50);
    sr = archive_item_read(&item, (byte_t*)item_read_buffer, 50);
    printf("Read %lu bytes from item '%s'\n", UNWRAP(sr), item_2_name);
    printf("Read matched written: %s\n", T_F(strcmp(item_2_content, item_read_buffer) == 0));
    archive_item_close(&item);

    archive_close(&archive);
    
    return 0;
}