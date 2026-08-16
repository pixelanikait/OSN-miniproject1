#include "lexer.h"

#include<string.h>
#include<stdlib.h>
#include<ctype.h>

static char* cpy_str(const char* start, size_t l){
    char* res;
    res = malloc(l+1);
    if(res == NULL) return NULL;
    strncpy(res, start, l);
    res[l] = '\0';
    return res;
}

static int add_word(token_list* tokens, const char* start, size_t l){
    token_node* node;
    node = malloc(sizeof(token_node));

    if(node == NULL) return 0;

    node->token.type = token_word;
    node->token.value = cpy_str(start, l);

    if(node->token.type == NULL){
        free(node);
        return;
    }

    return 1;
}

int lexer_line(const char* line, token_list* tokens){
    size_t i = 0;
    tokens->head = NULL;
    tokens->tail = NULL;
    while(line[i] != '\0'){
        size_t start;
        if(isspace((unsigned char)line[i])){
            i++;
            continue;
        }
        start = i;
        while(line[i] != '\0' && !isspace((unsigned char)line[i])) i++;
        if(!add_word(tokens, line + start, i - start)){
            free_tokens(tokens);
            return 0;
        }
    }
}

void free_tokens(token_list* tokens){
    token_node* curr = tokens->head;
    while(curr != NULL){
        token_node* next = curr->next;
        free(curr->token.value);
        free(curr);
        curr = next;
    }
    tokens->head = NULL;
    tokens->tail = NULL;
    return;
}
