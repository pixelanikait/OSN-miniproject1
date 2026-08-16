#include<stdio.h>
#include<stdlib.h>

#include "input.h"

char* read_input(){
    char* line = NULL;
    size_t capacity = 0;

    if(getline(&line, &capacity, stdin) == -1) {
        free(line);
        return NULL;
    }

    return line;
}