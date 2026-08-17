#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct{
    token_node* current;
}parser;

int validate(const token_list* tokens);

#endif