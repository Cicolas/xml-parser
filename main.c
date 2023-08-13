#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define LOG_OUT(...)                           \
    {                                          \
        printf("%s:%i: ", __FILE__, __LINE__); \
        printf(__VA_ARGS__);                   \
        printf("\n");                          \
    }

#define RAISE(...)           \
    {                        \
        LOG_OUT(__VA_ARGS__) \
        assert(false);       \
    }

#define MINI_BUFFER_LIMIT 128
#define BUFFER_LIMIT 1024
#define CHUNK_SIZE 8
#define XML_INDENTATION 2

#ifndef BOOL
    typedef char bool;
    #define true 1
    #define false 0
#endif

// --Helper Functions--------------------------------------------------------------------------------------------------------

bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
bool is_numeric(char c) {
    return c >= '0' && c <= '9';
}
bool is_alphanumeric(char c) {
    return is_alpha(c) || is_numeric(c) || c == '_';
}
bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void strtrimr(char *str) {
    for (int i = strlen(str)-1; i >= 0; i--) {
        if (!is_space(str[i])) break;
        else str[i] = '\0';
    }
}

char *indentify(int i) {
    i *= XML_INDENTATION;

    char *str = malloc(sizeof(char) * (i + 1));
    str[i] = '\0';

    i -= XML_INDENTATION;
    for (;i >= 0; i -= XML_INDENTATION) {
        for (int j = 0; j < XML_INDENTATION; j++) {
            str[i+j] = ' ';
        }   
    }

    return str;   
}

// --Symbol-----------------------------------------------------------------------------------------------------------------

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
    ATTRIBUTE,
    TEXT
} ElementsEnum;

typedef struct Element Element;
typedef struct Attribute Attribute;
typedef struct AttributeList AttributeList;
typedef struct Tag Tag;
typedef struct Text Text;

struct Tag {
    char name[MINI_BUFFER_LIMIT];
    
    int child_count;
    int child_size; // read-only
    Element *children;
    
    int attr_count;
    Attribute *attrs;
};

struct Attribute {
    char name[MINI_BUFFER_LIMIT];
    char value[MINI_BUFFER_LIMIT];
};

struct AttributeList {
    int attr_count;
    int attr_size;
    Attribute *attrs;
};

struct Text {
    char *text;
};

struct Element {
    ElementsEnum type;

    union {
        Tag tag;
        Text text;
    };
};

Tag *tag_new(char *name, AttributeList *attrs) {
    Tag *t = malloc(sizeof(Tag));
    
    strcpy(t->name, name);

    t->child_count = 0;
    t->child_size = CHUNK_SIZE;
    t->children = malloc(sizeof(Element) * (t->child_size));

    t->attr_count = attrs == NULL ? 0 : attrs->attr_count;
    t->attrs = malloc(sizeof(Attribute) * (t->attr_count));
    if (attrs != NULL)
        memcpy(t->attrs, attrs->attrs, sizeof(Attribute) * attrs->attr_count);

    return t;
}

void tag_add_child(Tag *tag, Element elem) {
    if (tag->child_count >= tag->child_size) {
        LOG_OUT("children realocation (%i -> %i)", tag->child_size, tag->child_size + CHUNK_SIZE);
        
        tag->child_size += CHUNK_SIZE;
        tag->children = realloc(tag->children, sizeof(Element) * tag->child_size);
        if (tag->children == NULL) RAISE("not enough memory to realocate?");
    }
    
    tag->children[tag->child_count++] = elem;
}

Attribute attr_new(char *name, char *value) {
    Attribute attr;

    strcpy(attr.name, name);
    strcpy(attr.value, value);

    return attr;
}

AttributeList *attrlist_new() {
    AttributeList *attrs = malloc(sizeof(AttributeList));

    attrs->attr_count = 0;
    attrs->attr_size = CHUNK_SIZE;
    attrs->attrs = malloc(sizeof(Attribute) * attrs->attr_size);

    return attrs;
}

void attrlist_add(AttributeList *list, Attribute attr) {
    if (list->attr_count >= list->attr_size) {
        LOG_OUT("attribute realocation (%i -> %i)", list->attr_size, list->attr_size + CHUNK_SIZE);

        list->attr_size += CHUNK_SIZE;
        list->attrs = realloc(list->attrs, sizeof(Attribute) * list->attr_size);
        if (list->attrs == NULL) RAISE("not enough memory to realocate?");
    }
    
    list->attrs[list->attr_count++] = attr;
}

Text *text_new(char *str) {
    Text *t = malloc(sizeof(Text));
    
    t->text = malloc(sizeof(char) * strlen(str));
    strcpy(t->text, str);

    return t;
}

void text_set(Text *t, char *str) {
    t->text = realloc(t->text, strlen(str));

    strcpy(t->text, str);
}

Element elem_new(ElementsEnum type, void *ptr) {
    Element elem; 
    elem.type = type;

    switch (type) {
        case TAG: elem.tag = *(Tag*)ptr; break;
        case TEXT: elem.text = *(Text*)ptr; break;
        default: RAISE("unknonw element type (%i)", type);
    }

    free(ptr);
    return elem;
}

// --Stack-------------------------------------------------------------------------------------------------------------------

typedef struct StackNode StackNode;

struct StackNode {
    StackNode *next;
    Tag *tag;
};

StackNode *SNode_new(Tag *tag) {
    StackNode *sn = malloc(sizeof(StackNode));
    sn->tag = tag;
    sn->next = NULL;
    return sn;
}

void stack(StackNode **node, Tag *tag) {
    StackNode *ref = *node;
    *node = SNode_new(tag);
    (*node)->next = ref;
}

Tag *unstack(StackNode **node) {
    assert(*node != NULL);

    Tag *tag = (*node)->tag;
    *node = (*node)->next;

    return tag;
}

// --------------------------------------------------------------------------------------------------------------------------

Symbol parse_xml_char(char c);
Element parse_xml_file(char* path);
void print_xml_element(Element *elem, int indentation);

int main(void) {
    Element root = parse_xml_file("teste.xml");
    print_xml_element(&root, 0);

    return 0;
}

Symbol parse_xml_char(char c) {
    switch (c) {
        case '<': return (Symbol) { OPEN, c };
        case '>': return (Symbol) { CLOSE, c };
        case '=': return (Symbol) { EQUALS, c };
        case '"': return (Symbol) { QUOTE, c };
        case '/': return (Symbol) { SLASH, c };
        case '?': return (Symbol) { QUESTION, c };
        case '!': return (Symbol) { EXCLAMATION, c };
        default: break;
    }

    if (is_alphanumeric(c)) return (Symbol) { ALPHA_NUMERIC, c };
    if (is_space(c)) return (Symbol) { SPACE, c };
    
    return (Symbol) { SYMBOL, c };
}

Element parse_xml_file(char *path) {
    FILE *fd;
    fd = fopen(path, "r");
    if (fd == NULL) 
        RAISE("cannot open the file: %s", path);

    Symbol sym = {0};
    char c;
    
    StackNode *node = NULL;

    bool is_ending_tag = false;
    bool tag_opened = false;
    bool str_opened = false;
    bool pending = false;
    bool comment = false;
    char str[BUFFER_LIMIT] = "\0";

    char attr_name[MINI_BUFFER_LIMIT] = "\0";
    char tag_name[MINI_BUFFER_LIMIT] = "\0";
    AttributeList *attrs = attrlist_new();

    while ((c = fgetc(fd)) != EOF) {
        if (comment) {
            if (c == '-' && fgetc(fd) == '-') {
                if (fgetc(fd) != '>') RAISE("'--' not allowed in comments");
                
                comment = false;
                tag_opened = false;
            }
        }

        sym = parse_xml_char(c);

        if (sym.type == OPEN) {
            if (tag_opened) 
                RAISE("cannot open a tag inside another tag %s", str);

            tag_opened = true;
            
            if (!strlen(str)) continue;
            if (node == NULL) RAISE("text before the first tag isnt valid");

            strtrimr(str);
            tag_add_child(
                node->tag,
                elem_new(TEXT, text_new(str))
            );
            str[0] = '\0';
        }

        else if (sym.type == CLOSE) {
            if (!tag_opened) RAISE("cannot close an unopened tag");
            if (str_opened) RAISE("unclosed string");

            tag_opened = false;
            pending = false;
            
            if (!is_ending_tag) {
                if (!strlen(tag_name))
                    strcpy(tag_name, str);
                
                stack(&node, tag_new(tag_name, attrs));
                
                attrs->attr_count = 0;
            }
            else {
                Tag *l = unstack(&node);
                is_ending_tag = false;

                if(!(strcmp(l->name, str) == 0))
                    RAISE("something went wrong with closing tags '%s' '%s'\n", l->name, str);

                if(node == NULL) {
                    while ((c = fgetc(fd)) != EOF) {
                        sym = parse_xml_char(c);
                        if (sym.type != SPACE) RAISE("found %c after end of xml", sym.value);
                    }

                    return elem_new(TAG, l);
                }

                tag_add_child(node->tag, elem_new(TAG, l));
            }

            tag_name[0] = '\0';
            str[0] = '\0';
        }

        else if (sym.type == ALPHA_NUMERIC || (sym.type == SYMBOL && !tag_opened)) {
            if (!str_opened && !strlen(str) && !is_alpha(sym.value) && !(sym.value != '_')) 
                RAISE("names and attributes shouldn't start with numbers");

            if (pending && strlen(str)) {
                if (is_ending_tag) RAISE("ending tag only needs to have name");
                if (strlen(tag_name)) RAISE("invalid attribute declaration");

                strcpy(tag_name, str);

                str[0] = '\0';
                pending = false;
            }

            int len = strlen(str); 
            str[len] = sym.value;
            str[len + 1] = '\0';
        }

        else if (sym.type == SPACE) {
            if (tag_opened && !str_opened && strlen(str) && !strlen(tag_name)) pending = true;
            
            if (!tag_opened && strlen(str) && str[strlen(str)-1] != ' ') strcat(str, " ");
        } 

        else if (sym.type == SLASH) {
            is_ending_tag = true;
        
            if (strlen(str)) {
                if (!strlen(tag_name))
                    strcpy(tag_name, str);
                
                stack(&node, tag_new(tag_name, NULL));
            }
        }

        else if (sym.type == EQUALS) {
            if (is_ending_tag) RAISE("ending tag only needs to have name");
            
            strcpy(attr_name, str);
            str[0] = '\0';
        }

        else if (sym.type == QUOTE && tag_opened) {
            if (is_ending_tag) RAISE("ending tag only needs to have name");
            if (!str_opened && strlen(str)) RAISE("character before attribute definition");

            if (str_opened) {
                Attribute attr = attr_new(attr_name, str);
                str[0] = '\0';
                attr_name[0] = '\0';

                attrlist_add(attrs, attr);
            }

            str_opened = !str_opened;
        }

        else if (sym.type == EXCLAMATION && tag_opened) {
            if (fgetc(fd) != '-' || fgetc(fd) != '-')
                RAISE("bad comment");
            
            comment = true;
        }
    }

    RAISE("reached EOF before end of <%s>", node->tag->name);
    return elem_new(0, NULL);
}

void print_xml_element(Element *elem, int indentation) {
    printf(indentify(indentation));

    if (elem->type == TEXT) {
        printf("%s\n", elem->text.text);
    } else if (elem->type == TAG) {
        Tag *tag = &elem->tag;
        printf("<%s", tag->name);
        
        for(int i = 0; i < tag->attr_count; i++) {
            Attribute attr = tag->attrs[i];
            printf(" %s = \"%s\"", attr.name, attr.value);
        }
        
        printf(">\n");

        for(int i = 0; i < tag->child_count; i++) {
            print_xml_element(&tag->children[i], indentation + 1);
        }

        printf(indentify(indentation));
        printf("</%s>\n", tag->name);
    }
}