#include "peek.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PEEK_CHUNK_SIZE 4096

typedef struct peek_line{
    char* text;
    int line_num;
}peek_line;

static int parse_flags(const char* value, int* number_lines, int* reverse){
    if(value[0] != '-' || value[1] == '\0'){
        return 0;
    }
    for(int i=1;value[i] != '\0';i++){
        if(value[i] == 'n') *number_lines = 1;
        else if(value[i] == 'r') *reverse = 1;
        else return 0;
    }
    return 1;
}

static void print_numbered(FILE* file){
    char buffer[4096];
    int line = 0;
    
    while(fgets(buffer, sizeof(buffer), file) != NULL){
        int non_empty = 0;
        for(int i=0;buffer[i] != '\0';i++){
            if(buffer[i] != '\n' && buffer[i] != '\r'){
                non_empty = 1;
                break;
            }
        }
        if(non_empty){
            line++;
            printf("%d %s", line, buffer);
        }
        else{
            printf("%s", buffer);
        }
    }
}

static int find_previous_newline(FILE* file, long before, long* newline_pos){
    char buffer[PEEK_CHUNK_SIZE];
    long end = before;

    while(end > 0){
        long chunk_start;
        if(end > PEEK_CHUNK_SIZE){
            chunk_start = end - PEEK_CHUNK_SIZE;
        }
        else{
            chunk_start = 0;
        }
        if(fseek(file, chunk_start, SEEK_SET) != 0){
            return 0;
        }

        size_t count = fread(buffer, 1, (size_t)(end - chunk_start), file);

        if(ferror(file)){
            return 0;
        }

        for(long i = (long)count - 1;i>=0;i--){
            if(buffer[i] == '\n'){
                *newline_pos = chunk_start + i;
                return 1;
            }
        }
        end = chunk_start;
    }
    return 0;
}

static int print_range(FILE* file, long start, long end){
    char buffer[PEEK_CHUNK_SIZE];

    if(fseek(file, start, SEEK_SET) != 0){
        return 0;
    }

    while(start < end){
        long remaining = end - start;
        size_t wanted = (remaining > PEEK_CHUNK_SIZE) ? PEEK_CHUNK_SIZE : (size_t)remaining;
        size_t got = fread(buffer, 1, wanted, file);

        if(got != wanted){
            return 0;
        }

        fwrite(buffer, 1, got, stdout);
        start+=(long)got;
    }
    return 1;
}

static int count_non_empty_lines(FILE* file){
    char buffer[PEEK_CHUNK_SIZE];
    int count = 0;
    int non_empty = 0;

    if(fseek(file, 0, SEEK_SET) != 0){
        return -1;
    }
    size_t bytes_read;
    while((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0){
        for(size_t i = 0; i < bytes_read; i++){
            if(buffer[i] == '\n'){
                if(non_empty) count++;
                non_empty = 0;
            }
            else if(buffer[i] != '\r'){
                non_empty = 1;
            }
        }
    }
    if(ferror(file))return -1;
    if(non_empty)count++;
    return count;
}

static int range_is_non_empty(FILE* file, long start, long end){
    char buffer[PEEK_CHUNK_SIZE];
    if(fseek(file, start, SEEK_SET) != 0){
        return 0;
    }
    while(start < end){
        long remaining = end - start;
        size_t wanted = (remaining > PEEK_CHUNK_SIZE) ? PEEK_CHUNK_SIZE : (size_t)remaining;
        size_t got = fread(buffer, 1, wanted, file);

        if(got != wanted) return 0;
        for(size_t i = 0; i < got; i++){
            if(buffer[i] != '\r' && buffer[i] != '\n'){
                return 1;
            }
        }
        start += (long)got;
    }
    return 0;
}

static int print_reverse_seek(FILE* file, int number_lines){
    if(fseek(file, 0, SEEK_END) != 0) return 0;
    long cursor = ftell(file);
    if(cursor < 0) return 0;
    int curr_line_num = 0;
    if(number_lines){
        curr_line_num = count_non_empty_lines(file);
        if(curr_line_num < 0) return 0;
        if(fseek(file, 0, SEEK_END) != 0) return 0;
        cursor = ftell(file);
        if(cursor < 0) return 0;
    }
    while(cursor > 0){
        long line_end = cursor;
        long search_end = line_end;
        if(fseek(file, line_end - 1, SEEK_SET) != 0) return 0;
        int last = fgetc(file);
        if(last == '\n') search_end--;
        long previous_newline;
        if(find_previous_newline(file, search_end, &previous_newline)){
            long start = previous_newline + 1;
            int non_empty = range_is_non_empty(file, start, line_end);
            if(number_lines && non_empty){
                printf("%d ", curr_line_num);
                curr_line_num--;
            }
            if(!print_range(file, start, line_end)){
                return 0;
            }
            cursor = start;
        }
        else{
            long start = 0;
            int non_empty = range_is_non_empty(file, start, line_end);
            if(number_lines && non_empty) printf("%d ",curr_line_num);
            if(!print_range(file, start, line_end)){
                return 0;
            }
            break;
        }
    }
    return 1;
}

static void print_reverse_buffer(FILE* file, int number_lines){
    peek_line* lines = NULL;
    size_t count = 0;
    size_t capacity = 0;
    char buffer[4096];

    int line_number = 0;

    while(fgets(buffer, sizeof(buffer), file) != NULL){
        if(count == capacity){
            size_t new_capacity = (capacity == 0) ? 16 : capacity*2;
            peek_line* new_lines = realloc(lines, new_capacity * sizeof(peek_line));
            if(new_lines == NULL){
                for(size_t i = 0; i < count; i++){
                    free(lines[i].text);
                }
                free(lines);
                return;
            }
            lines = new_lines;
            capacity = new_capacity;
        }
        int non_empty = 0;
        for(int i=0;buffer[i] != '\0';i++){
            if(buffer[i] != '\n' && buffer[i] != '\r'){
                non_empty = 1;
                break;
            }
        }
        if(non_empty){
            line_number++;
        }
        lines[count].text = malloc(strlen(buffer) + 1);
        if(lines[count].text == NULL){
            for(size_t i = 0; i < count; i++){
                free(lines[i].text);
            }
            free(lines);
            return; 
        }
        strcpy(lines[count].text, buffer);
        lines[count].line_num = non_empty ? line_number : 0;
        count++;
    }
    for(size_t i = count;i>0;i--){
        peek_line* curr = &lines[i-1];
        if(number_lines && curr->line_num != 0){
            printf("%d %s", curr->line_num, curr->text);
        }
        else{
            printf("%s", curr->text);
        }
    }
    for(size_t i = 0;i<count;i++) free(lines[i].text);
    free(lines);
    return;
}

int peek(token_list* tokens){
    int number_lines = 0;
    int reverse = 0;
    struct stat info;

    token_node* curr = tokens->head;
    curr = curr->next;

    while(curr != NULL && curr->token.type == token_word && parse_flags(curr->token.value, &number_lines, &reverse)){
        curr = curr->next;
    }
    if(curr == NULL){
        FILE* file = stdin;
        if(reverse) print_reverse_buffer(file, number_lines);
        else if(number_lines) print_numbered(file);
        else{
            int c;
            while((c = fgetc(file)) != EOF){
                putchar(c);
            }
        }
        clearerr(stdin);
        return 0;
    }
    while(curr != NULL){
        FILE* file;
        if(strcmp(curr->token.value, "-") == 0){
            file = stdin;
        }
        else{
            file = fopen(curr->token.value, "r");

            if(file == NULL){
                fprintf(stderr, "peek: no such file or directory\n");
                curr = curr->next;
                continue;
            }

            if(fstat(fileno(file), &info) != 0){
                fprintf(stderr, "peek: no such file or directory\n");
                fclose(file);
                curr = curr->next;
                continue;
            }
        }
        if(reverse){
            if(S_ISREG(info.st_mode)) print_reverse_seek(file, number_lines);
            else{
                print_reverse_buffer(file, number_lines);
            }
        }
        else if(number_lines){
            print_numbered(file);
        }
        else{
            int c;
            while((c = fgetc(file)) != EOF){
                putchar(c);
            }
        }
        if (file != stdin) fclose(file);
        else clearerr(stdin);
        curr = curr->next;
    }
    return 0;
}