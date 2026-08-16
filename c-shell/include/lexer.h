#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct token_node{
    token token;
    struct token_node* next;
}token_node;

typedef struct{
    token_node* head;
    token_node* tail;
}token_list;

int lexer_line(const char* line, token_list* tokens);
void free_tokens(token_list* tokens);

#endif