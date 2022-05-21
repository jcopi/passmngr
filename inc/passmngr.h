#pragma once

#include <kv.h>
#include <slice.h>

guarded_result_t read_user_password ();
guarded_result_t gen_random_char_password (const char* const dictionary, size_t entropy);
guarded_result_t gen_random_word_password (const char* const words_file, size_t entropy);

slice trim_space (slice s) {
    ssize_t start = 0;
    for (; start < (ssize_t)s.len; start++) {
        if (((char*)s.ptr)[start] != ' ' && ((char*)s.ptr)[start] != '\t' && ((char*)s.ptr)[start] != '\r' && ((char*)s.ptr)[start] != '\n') {
            break;
        }
    }
    s = sub_slice(s, start, s.len);

    ssize_t end = s.len;
    for (end--; end >= 0; end--) {
        if (((char*)s.ptr)[end] != ' ' && ((char*)s.ptr)[end] != '\t' && ((char*)s.ptr)[end] != '\r' && ((char*)s.ptr)[end] != '\n') {
            break;
        }
    }

    s = sub_slice(s, 0, end+1);

    return s;
}

void print_slice_hex(slice s) {
    for (size_t i = 0; i < s.len; i++) {
        printf("%02x", ((uint8_t*)s.ptr)[i]);
    }
}

void print_slice_as_string(slice s) {
    for (size_t i = 0; i < s.len; i++) {
        printf("%c", ((char*)s.ptr)[i]);
    }
}