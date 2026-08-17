#include "parser.h"

#include<stdlib.h>

static int parse_arg(parser* p);
static int parse_tgt(parser* p);
static int parse_cmd(parser* p);
static int parse_bg(parser* p);

static int parse_arg(parser* p){
    if(p->current == NULL){
        return 1;
    }
    switch (p->current->token.type)
    {
    case token_word:
        p->current = p->current->next;
        return parse_arg(p);
    case token_lt:
    case token_gt:
    case token_gtgt:
        p->current = p->current->next;
        return parse_tgt(p);
    case token_semi:
    case token_pipe:
        p->current = p->current->next;
        return parse_cmd(p);
    case token_amp:
        p->current = p->current->next;
        return parse_bg(p);
    default:
        return 0;
    }
}

static int parse_tgt(parser* p){
    if(p->current == NULL) return 0;
    if(p->current->token.type != token_word) return 0;
    p->current = p->current->next;
    return parse_arg(p);
}

static int parse_cmd(parser* p){
    if(p->current == NULL) return 0;
    if(p->current->token.type != token_word) return 0;
    p->current = p->current->next;
    return parse_arg(p);
}

static int parse_bg(parser* p){
    if(p->current == NULL) return 1;
    if(p->current->token.type != token_word) return 0;
    p->current = p->current->next;
    return parse_arg(p);
}

static int parse_line(parser* p){
    if(p->current == NULL) return 1;
    if(p->current->token.type != token_word) return 0;
    p->current = p->current->next;
    return parse_arg(p);
}

int validate(const token_list* tokens){
    parser p;
    p.current=tokens->head;
    if(!parse_line(&p)) return 0;
    if(p.current != NULL) return 0;
    return 1;
}