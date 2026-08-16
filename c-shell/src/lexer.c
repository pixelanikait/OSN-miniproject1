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

static int is_op(char c){
    return c == '|' ||
           c == '&' ||
           c == ';' ||
           c == '>' ||
           c == '<';
}

static Token_type get_token_type(char c){
    switch (c)
    {
    case '|':
        return token_pipe;
    case '&':
        return token_amp;
    case ';':
        return token_semi;
    case '>':
        return token_gt;
    case '<':
        return token_lt;
    default:
        return token_word;
    }
}

static int add_op(token_list* tokens, Token_type type, const char* value, size_t l){
    token_node* node;
    node = malloc(sizeof(token_node));
    if(node == NULL) return 0;

    node->token.type = type;
    node->token.value = cpy_str(value, l);

    if(node->token.value == NULL){
        free(node);
        return 0;
    }

    node->next = NULL;
    if(tokens->head == NULL){
        tokens->head = node;
        tokens->tail = node;
    }
    else{
        tokens->tail->next = node;
        tokens->tail = node;
    }

    return 1;
}

static int add_word(token_list* tokens, const char* start, size_t l){
    token_node* node;
    char* value;
    size_t i=0;
    size_t j=0;
    char q = '\0';
    node = malloc(sizeof(token_node));

    if(node == NULL) return 0;

    value = malloc(l+1);
    if(value == NULL){
        free(node);
        return 0;
    }

    while(i<l){
        if(q == '\0' && (start[i] == '\'' || start[i] == '"')){
            q = start[i];
            i++;
            continue;
        }

        if(q != '\0' && start[i] == q){
            q = '\0';
            i++;
            continue;
        }

        if(start[i] == '\\' && q != '\''){
            i++;
            if(i<l){
                value[j] = start[i];
                j++;
                i++;
            }
            continue;
        }
        value[j] = start[i];
        j++;
        i++;
    }
    value[j] = '\0';
    node->token.type = token_word;
    node->token.value=value;
    node->next=NULL;

    if(tokens->head == NULL){
        tokens->head = node;
        tokens->tail = node;
    }
    else{
        tokens->tail->next = node;
        tokens->tail = node;
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
        //handle max munch
        if(line[i] == '>' && line[i+1] == '>'){
            if(!add_op(tokens, token_gtgt, line+i, 2)){
                free_tokens(tokens);
                return 0;
            }
            i+=2;
            continue;
        }
        //handle single op
        if(is_op(line[i])){
            Token_type type;
            type = get_token_type(line[i]);
            if(!add_op(tokens, type, line+i, 1)){
                free_tokens(tokens);
                return 0;
            }
            i++;
            continue;
        }
        //handle normal word
        start = i;
        while(line[i] != '\0'){
            //backslash escape
            if(line[i] == '\\'){
                i++;
                if(line[i] == '\0'){
                    free_tokens(tokens);
                    return 0;
                }
                i++;
                continue;
            }
            //quoted
            if(line[i] == '\'' || line[i] == '"'){
                char q = line[i];
                i++;
                while(line[i] != '\0' && line[i] != q){
                    if(q == '"' && line[i] == '\\'){
                        i++;
                        if(line[i] == '\0'){
                            free_tokens(tokens);
                            return 0;
                        }
                        i++;
                        continue;
                    }
                    i++;
                }
                if(line[i] == '\0'){
                    free_tokens(tokens);
                    return 0;
                }
                i++;
                continue;
            }

            if(isspace((unsigned char)line[i]) || is_op(line[i])) break;
            i++;
        }
        if(!add_word(tokens, line+start, i-start)){
            free_tokens(tokens);
            return 0;
        }
    }
    return 1;
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
