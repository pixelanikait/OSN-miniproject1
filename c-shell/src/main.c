#include "prompt.h"
#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "hop.h"
#include "reveal.h"
#include "peek.h"
#include "locate.h"
#include "execute.h"

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

// static void print_token(token_node *node)
// {
//     switch (node->token.type) {
//         case token_word:
//             printf("WORD(\"%s\")\n", node->token.value);
//             break;

//         case token_pipe:
//             printf("OP_PIPE(\"%s\")\n", node->token.value);
//             break;

//         case token_amp:
//             printf("OP_AMP(\"%s\")\n", node->token.value);
//             break;

//         case token_semi:
//             printf("OP_SEMI(\"%s\")\n", node->token.value);
//             break;

//         case token_lt:
//             printf("OP_LT(\"%s\")\n", node->token.value);
//             break;

//         case token_gt:
//             printf("OP_GT(\"%s\")\n", node->token.value);
//             break;

//         case token_gtgt:
//             printf("OP_GTGT(\"%s\")\n", node->token.value);
//             break;
//     }
// }

int main(){
    prompt_init();
    hop_init();
    reveal_init();
    while (1){
        char* line;
        token_list tokens;
        token_node* curr;

        show_prompt();

        line = read_input();
        if(line == NULL) break;

        if(!lexer_line(line, &tokens)){
            fprintf(stderr, "Lexer failed\n");
            free(line);
            continue;
        }
        
        if(!validate(&tokens)){
            fprintf(stderr,"Parser rejected input\n");
            free_tokens(&tokens);
            free(line);
            continue;
        }

        curr = tokens.head;

        if(curr != NULL && curr->token.type == token_word && strcmp(curr->token.value, "hop") == 0){
            hop(&tokens);
            free_tokens(&tokens);
            free(line);
            continue;
        }

        if(curr != NULL && curr->token.type == token_word && strcmp(curr->token.value, "peek") == 0){
            peek(&tokens);
            free_tokens(&tokens);
            free(line);
            continue;
        }

        if(curr != NULL && curr->token.type == token_word && strcmp(curr->token.value, "reveal") == 0){
            reveal(&tokens);
            free_tokens(&tokens);
            free(line);
            continue;
        }

        if(curr != NULL && curr->token.type == token_word && strcmp(curr->token.value, "locate") == 0){
            locate(&tokens);
            free_tokens(&tokens);
            free(line);
            continue;
        }
        
        // while(curr != NULL){
        //     print_token(curr);
        //     curr = curr->next;
        // }
        execute(&tokens);
        free_tokens(&tokens);
        free(line);
        continue;
    }
    return 0;
}