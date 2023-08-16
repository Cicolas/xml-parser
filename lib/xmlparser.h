// Nícolas dos Santos Carvalho
// Maringá - PR / Brasil
// Universidade Estadual de Maringá - 2023

// xmlparser é uma biblioteca que implementa funções para a manipulação
// de arquivos xml na linguagem C

//! obs.: feito para estudar a linguagem C!!!
//!       provavelmente possui muitos bugs e
//!       memory leaks.

#ifndef _XML_H
#define _XML_H

#include "TYPO.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#ifndef _LOG_H
    #define _LOG_H
    #define LOG_OUT(...)                           \
        {                                          \
            printf("%s:%i: ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                   \
            printf("\n");                          \
        }
    #define RAISE(...)           \
        {                        \
            LOG_OUT(__VA_ARGS__) \
            exit(1);       \
        }
#endif

#ifndef bool
    #define bool char
    #define true 1
    #define false 0
#endif

// --Symbol------------------------------------------------------------------------------------------------------------------

typedef enum {
    OPEN,
    CLOSE,
    ALPHA_NUMERIC,
    SPACE,
    EQUALS,
    QUOTE,
    SLASH,
    QUESTION,
    EXCLAMATION,
    SYMBOL
} SymbolsEnum;

typedef struct {
    SymbolsEnum type;
    char value;
} Symbol;


// --Elements----------------------------------------------------------------------------------------------------------------

typedef enum {
    TAG,
    TEXT
} ElementsEnum;

typedef struct Element Element;
typedef struct Attribute Attribute;
typedef struct AttributeList AttributeList;
typedef struct Tag Tag;
typedef struct Text Text;

struct Tag {
    String name;

    int child_count;
    int child_size; // read-only
    Element *children;

    int attr_count;
    Attribute *attrs;
};

struct Attribute {
    String name;
    String value;
};

struct AttributeList {
    int attr_count;
    int attr_size;
    Attribute *attrs;
};

struct Text {
    String text;
};

struct Element {
    ElementsEnum type;

    union {
        Tag tag;
        Text text;
    };
};

// --------------------------------------------------------------------------------------------------------------------------

Symbol parse_xml_char(char c);
Element parse_xml_file(char* path);
void elem_free(Element *elem);

#endif
