#include "prompt.h"
#include "input.h"
#include "lexer.h"

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int main(){
    prompt_init();
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
            break;
        }

        curr = tokens.head;

        while(curr != NULL){
            // printf("WORD(\"%s\")\n", curr->token.value);
            curr = curr->next;
        }

        free_tokens(&tokens);
        free(line);
    }
    return 0;
}