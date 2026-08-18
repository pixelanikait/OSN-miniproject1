#include "locate.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>

#define PATH_BUFFER_SIZE 4096
#define MAX_PATH_ENTRIES 4096

static int is_exec(const char* path){
    struct stat info;
    if(stat(path, &info) != 0) return 0;
    if(!S_ISREG(info.st_mode)) return 0;
    return access(path, X_OK) == 0;
}

static int search_path(const char* command){
    const char* path_env = getenv("PATH");
    if(path_env == NULL) return 0;
    char* copy_path = malloc(strlen(path_env) + 1);
    if(copy_path == NULL) return 0;
    strcpy(copy_path, path_env);

    int found = 0;
    char* saveptr = NULL;
    char* entry = strtok_r(copy_path, ":", &saveptr);

    while(entry != NULL){
        char dir[PATH_BUFFER_SIZE];
        if(entry[0] == '\0'){
            if(getcwd(dir, sizeof(dir)) == NULL){
                entry = strtok_r(NULL, ":", &saveptr);
                continue;
            }
        }
        else{
            if(strlen(entry) >= sizeof(dir)){
                entry = strtok_r(NULL, ":", &saveptr);
                continue;;
            }
            strcpy(dir, entry);
        }

        char candidate[PATH_BUFFER_SIZE];

        if(strlen(dir) + 1 + strlen(command) >= sizeof(candidate)){
            entry = strtok_r(NULL, ":", &saveptr);
            continue;
        }

        strcpy(candidate, dir);
        strcat(candidate, "/");
        strcat(candidate, command);

        if(is_exec(candidate)){
            printf("%s\n", candidate);
            found = 1;
        }
        entry = strtok_r(NULL, ":", &saveptr);
    } 
    free(copy_path);
    return found;
}

int locate(token_list* tokens){
    token_node* curr = tokens->head;
    curr = curr->next;
    if(curr == NULL){
        fprintf(stderr, "locate: invalid syntax\n");
        return 1;
    }
    while(curr != NULL){
        int found = 0;
        char cwd[PATH_BUFFER_SIZE];
        if(getcwd(cwd, sizeof(cwd)) != NULL){
            char candidate[PATH_BUFFER_SIZE];
            if(strlen(cwd) + 1 + strlen(curr->token.value) < sizeof(candidate)){
                strcpy(candidate, cwd);
                strcat(candidate, "/");
                strcat(candidate, curr->token.value);
                if(is_exec(candidate)){
                    printf("%s\n", candidate);
                    found = 1;
                }
            }
        }
        if(search_path(curr->token.value)) found = 1;
        if(!found){
            fprintf(stderr, "locate: command not found(%s)\n", curr->token.value);
        }
        curr = curr->next;
    }
    return 0;
}