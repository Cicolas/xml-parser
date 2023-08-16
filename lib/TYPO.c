#include "TYPO.h"

static bool _string_check_realocation(String *s, unsigned int new_size);

String string_from(char *str) {
    String s = {
        malloc(STRING_BUFFER_SIZE),
        STRING_BUFFER_SIZE,
    };

    string_set(&s, str);

    return s;
}

String string_clone(String s) {
    String s_new = string_from(s.str);
    return s_new;
}

unsigned int string_len(String s) {
    return strlen(s.str);
}

String *string_set(String *s, char *str) {
    int new_size = strlen(str);
    _string_check_realocation(s, new_size);

    strcpy(s->str, str);

    return s;
}

String *string_concat(String *s1, String s2) {
    int new_size = strlen(s1->str) + strlen(s2.str);
    _string_check_realocation(s1, new_size);

    strcat(s1->str, s2.str);
    return s1;
}

String *string_append(String *s, char c) {
    int len = string_len(*s);
    _string_check_realocation(s, len  + 1);

    s->str[len] = c;
    s->str[len + 1] = '\0';
    return s;
}

String *string_clear(String *s) {
    s->str[0] = '\0';

    return s;
}

void string_free(String *s) {
    free(s->str);
}

bool string_equal(String s1, String s2) {
    return strcmp(s1.str, s2.str) == 0;
}

static bool _string_check_realocation(String *s, unsigned int new_size) {
    if (new_size + 1 > s->size) {
        // printf("String realocation (%i -> %i)", s->size, new_size + 1);
        s->size = new_size + 1;
        s->str = realloc(s->str, s->size);
        assert(s->str != NULL);
        return true;
    }
    return false;
}
