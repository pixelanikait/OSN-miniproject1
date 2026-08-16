#include "prompt.h"
#include "input.h"

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int main(){
    prompt_init();
    while (1){
        char* line;
        show_prompt();
        line = read_input();
        if(line == NULL) break;
        free(line);
    }
    return 0;
}