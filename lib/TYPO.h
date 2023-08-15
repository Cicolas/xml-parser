// Nícolas dos Santos Carvalho
// Maringá - PR / Brasil
// Universidade Estadual de Maringá - 2023

// TYPO é uma biblioteca que implementa diversas funções para a manipulação
// de strings na linguagem C

#ifndef _TYPO_H
#define _TYPO_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifndef bool
    #define bool char
    #define true 1
    #define false 0
#endif

#define STRING_BUFFER_SIZE 256

typedef struct {
    unsigned int size;
    char *str;
} String;

unsigned int string_len(String s);

String string_from(char *str);

String string_clone(String s);

String *string_set(String *s, char *str);

String *string_concat(String *s1, String s2);

String *string_append(String *s, char c);

String *string_clear(String *s);

bool string_equal(String s1, String s2);

char *string_to_arr(String s);

#endif