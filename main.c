#include "lib/xmlparser.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define XML_INDENTATION 2

char *indentify(int i); 
void print_xml_element(Element *elem, int indentation);

int main(int argc, char *argv[]) {
    if (argc < 2) RAISE("you need provide the file to read");

    Element root = parse_xml_file(argv[1]);
    print_xml_element(&root, 0);

    return 0;
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

void print_xml_element(Element *elem, int indentation) {
    printf("%s", indentify(indentation));

    if (elem->type == TEXT) {
        printf("%s\n", elem->text.text.str);
    } else if (elem->type == TAG) {
        Tag *tag = &elem->tag;
        printf("<%s", tag->name.str);
        
        for(int i = 0; i < tag->attr_count; i++) {
            Attribute attr = tag->attrs[i];
            printf(" %s = \"%s\"", attr.name.str, attr.value.str);
        }
        
        printf(">\n");

        for(int i = 0; i < tag->child_count; i++) {
            print_xml_element(&tag->children[i], indentation + 1);
        }

        printf("%s", indentify(indentation));
        printf("</%s>\n", tag->name.str);
    }
}