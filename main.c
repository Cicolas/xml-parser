#include "lib/xmlparser.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define XML_INDENTATION 2

void indentify(int i);
void print_xml_element(Element *elem, int indentation);

int main(int argc, char *argv[]) {
    if (argc < 2) RAISE("you need provide the file to read");

    Element root = parse_xml_file(argv[1]);
    // Element root = parse_xml_file("resources/teste.xml");
    print_xml_element(&root, 0);

    elem_free(&root);

    return 0;
}

void indentify(int i) {
    i *= XML_INDENTATION;
    i -= XML_INDENTATION;
    for (;i >= 0; i -= XML_INDENTATION) {
        for (int j = 0; j < XML_INDENTATION; j++) {
            printf("%c", ' ');
        }
    }
}

void print_xml_element(Element *elem, int indentation) {
    indentify(indentation);

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

        indentify(indentation);
        printf("</%s>\n", tag->name.str);
    }
}
