#ifndef TOKEN_H
#define TOKEN_H

typedef enum{
    token_word,
    token_pipe,
    token_amp,
    token_semi,
    token_lt,
    token_gt,
    token_gtgt
}Token_type;

typedef struct{
    Token_type type;
    char* value;
}token;


#endif