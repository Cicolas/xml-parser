#include "xmlparser.h"

#define CHUNK_SIZE 8

// --Stack-------------------------------------------------------------------------------------------------------------------

typedef struct StackNode StackNode;

struct StackNode {
    StackNode *next;
    Tag *tag;
};

// --Helper Functions--------------------------------------------------------------------------------------------------------

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static bool is_numeric(char c) {
    return c >= '0' && c <= '9';
}
static bool is_alphanumeric(char c) {
    return is_alpha(c) || is_numeric(c) || c == '_';
}
static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void strtrimr(char *str) {
    for (int i = strlen(str)-1; i >= 0; i--) {
        if (!is_space(str[i])) break;
        else str[i] = '\0';
    }
}

// --Elements----------------------------------------------------------------------------------------------------------------

static Tag *tag_new(String name, AttributeList *attrs) {
    Tag *t = malloc(sizeof(Tag));
    
    t->name = name;

    t->child_count = 0;
    t->child_size = CHUNK_SIZE;
    t->children = malloc(sizeof(Element) * (t->child_size));

    t->attr_count = attrs == NULL ? 0 : attrs->attr_count;
    t->attrs = malloc(sizeof(Attribute) * (t->attr_count));
    if (attrs != NULL)
        memcpy(t->attrs, attrs->attrs, sizeof(Attribute) * attrs->attr_count);

    return t;
}

static void tag_add_child(Tag *tag, Element elem) {
    if (tag->child_count >= tag->child_size) {
        LOG_OUT("children realocation (%i -> %i)", tag->child_size, tag->child_size + CHUNK_SIZE);
        
        tag->child_size += CHUNK_SIZE;
        tag->children = realloc(tag->children, sizeof(Element) * tag->child_size);
        if (tag->children == NULL) RAISE("not enough memory to realocate?");
    }
    
    tag->children[tag->child_count++] = elem;
}

static Attribute attr_new(String name, String value) {
    Attribute attr;

    attr.name = name;
    attr.value = value;

    return attr;
}

static AttributeList *attrlist_new() {
    AttributeList *attrs = malloc(sizeof(AttributeList));

    attrs->attr_count = 0;
    attrs->attr_size = CHUNK_SIZE;
    attrs->attrs = malloc(sizeof(Attribute) * attrs->attr_size);

    return attrs;
}

static void attrlist_add(AttributeList *list, Attribute attr) {
    if (list->attr_count >= list->attr_size) {
        LOG_OUT("attribute realocation (%i -> %i)", list->attr_size, list->attr_size + CHUNK_SIZE);

        list->attr_size += CHUNK_SIZE;
        list->attrs = realloc(list->attrs, sizeof(Attribute) * list->attr_size);
        if (list->attrs == NULL) RAISE("not enough memory to realocate?");
    }
    
    list->attrs[list->attr_count++] = attr;
}

static Text *text_new(String str) {
    Text *t = malloc(sizeof(Text));
    
    t->text = str;

    return t;
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

static StackNode *SNode_new(Tag *tag) {
    StackNode *sn = malloc(sizeof(StackNode));
    sn->tag = tag;
    sn->next = NULL;
    return sn;
}

static void stack(StackNode **node, Tag *tag) {
    StackNode *ref = *node;
    *node = SNode_new(tag);
    (*node)->next = ref;
}

static Tag *unstack(StackNode **node) {
    assert(*node != NULL);

    Tag *tag = (*node)->tag;
    *node = (*node)->next;

    return tag;
}

// --------------------------------------------------------------------------------------------------------------------------

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
    String buffer = string_from("\0");

    String attr_name = string_from("\0");
    String tag_name = string_from("\0");
    AttributeList *attrs = attrlist_new();

    while ((c = fgetc(fd)) != EOF) {
        if (comment) {
            if (c == '-' && fgetc(fd) == '-') {                
                if (fgetc(fd) != '>') RAISE("'--' not allowed in comments");

                comment = false;
                tag_opened = false;
            }

            continue;
        }

        sym = parse_xml_char(c);

        if (sym.type == OPEN) {
            if (tag_opened) 
                RAISE("cannot open a tag inside another tag %s", buffer.str);

            tag_opened = true;
            
            if (!string_len(buffer)) continue;
            if (node == NULL) RAISE("text before the first tag isnt valid");

            strtrimr(buffer.str);
            tag_add_child(
                node->tag,
                elem_new(TEXT, text_new(string_clone(buffer)))
            );
            string_clear(&buffer);
        }

        else if (sym.type == CLOSE) {
            if (!tag_opened) RAISE("cannot close an unopened tag");
            if (str_opened) RAISE("unclosed string");

            tag_opened = false;
            pending = false;
            
            if (!is_ending_tag) {
                if (!string_len(tag_name))
                    tag_name = string_clone(buffer);
                
                stack(&node, tag_new(string_clone(tag_name), attrs));
                
                attrs->attr_count = 0;
            }
            else {
                Tag *l = unstack(&node);
                is_ending_tag = false;

                if(!string_equal(l->name, buffer))
                    RAISE("something went wrong with closing tags '%s' '%s'\n", l->name.str, buffer.str);

                if(node == NULL) {
                    while ((c = fgetc(fd)) != EOF) {
                        sym = parse_xml_char(c);
                        if (sym.type != SPACE) RAISE("found %c after end of xml", sym.value);
                    }

                    return elem_new(TAG, l);
                }

                tag_add_child(node->tag, elem_new(TAG, l));
            }

            string_clear(&tag_name);
            string_clear(&buffer);
        }

        else if (sym.type == ALPHA_NUMERIC || (sym.type == SYMBOL && !tag_opened)) {
            if (!str_opened && !string_len(buffer) && !is_alpha(sym.value) && !(sym.value != '_')) 
                RAISE("names and attributes shouldn't start with numbers");

            if (pending && string_len(buffer)) {
                if (is_ending_tag) RAISE("ending tag only needs to have name");
                if (string_len(tag_name)) RAISE("invalid attribute declaration");

                tag_name = string_clone(buffer);

                string_clear(&buffer);
                pending = false;
            }

            string_append(&buffer, sym.value);
        }

        else if (sym.type == SPACE) {
            if (tag_opened && !str_opened && string_len(buffer) && !string_len(tag_name)) pending = true;
            
            if (!tag_opened && string_len(buffer) && buffer.str[string_len(buffer)-1] != ' ') string_append(&buffer, ' ');
        } 

        else if (sym.type == SLASH) {
            is_ending_tag = true;
        
            if (string_len(buffer)) {
                if (!string_len(tag_name))
                    tag_name = string_clone(buffer);
                
                stack(&node, tag_new(tag_name, NULL));
            }
        }

        else if (sym.type == EQUALS) {
            if (is_ending_tag) RAISE("ending tag only needs to have name");
            
            string_clear(&attr_name);
            string_concat(&attr_name, buffer);
            string_clear(&buffer);
        }

        else if (sym.type == QUOTE && tag_opened) {
            if (is_ending_tag) RAISE("ending tag only needs to have name");
            if (!str_opened && string_len(buffer)) RAISE("character before attribute definition");

            if (str_opened) {
                Attribute attr = attr_new(string_clone(attr_name), string_clone(buffer));
                string_clear(&buffer);
                string_clear(&attr_name);

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

    RAISE("reached EOF before end of <%s>", node->tag->name.str);
    return elem_new(0, NULL);
}
