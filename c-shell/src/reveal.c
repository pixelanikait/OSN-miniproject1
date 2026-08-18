#include "reveal.h"
#include "hop.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include<unistd.h>

#define MAX_ENTRIES 1024
#define PATH_BUFFER_SIZE 4096

static char reveal_home[PATH_BUFFER_SIZE];

void reveal_init(){
    if(getcwd(reveal_home, sizeof(reveal_home)) == NULL){
        perror("reveal");
        exit(EXIT_FAILURE);
    }
}

static int cmp(const void* a, const void* b){
    const char* first = *(const char**)a;
    const char* second = *(const char**)b;
    return strcmp(first,second);
}

static int is_directory(const char* path){
    struct stat info;
    if(stat(path, &info) != 0) return 0;
    return S_ISDIR(info.st_mode);
}

static int parse_flags(const char* value, int* show_hidden, int* recursive){
    if(value[0] != '-' || value[1] == '\0') return 0;
    for(int i=1;value[i] != '\0'; i++){
        if(value[i] == 'a') *show_hidden = 1;
        else if(value[i] == 't') *recursive = 1;
        else{
            return 0;
        }
    }
    return 1;
}

static int reveal_recursive(const char* path, const char* display_path, int show_hidden){
    DIR* directory;
    struct dirent* entry;

    char* entries[MAX_ENTRIES];
    int count = 0;

    directory = opendir(path);

    if(directory == NULL) return 1;

    while((entry = readdir(directory)) != NULL){
        if(!show_hidden && entry->d_name[0] == '.'){
            continue;
        }
        if(count >= MAX_ENTRIES){
            break;
        }
        entries[count] = malloc(sizeof(strlen(entry->d_name) + 1));
        if(entries[count] == NULL){
            closedir(directory);
            for(int i=0;i<count;i++){
                free(entries[i]);
            }
            return 1;
        }
        strcpy(entries[count], entry->d_name);
        count++;
    }
    closedir(directory);
    qsort(entries, count, sizeof(char*), cmp);
    for(int i=0;i<count;i++){
        char child_path[PATH_BUFFER_SIZE];
        char child_display_path[PATH_BUFFER_SIZE];

        if(strcmp(path, ".") == 0){
            strcpy(child_path, entries[i]);
        }
        else{
            snprintf(child_path, sizeof(child_path), "%s/%s", path, entries[i]);
        }
        if(display_path[0] == '\0'){
            strcpy(child_display_path, entries[i]);
        }
        else{
            snprintf(child_display_path, sizeof(child_display_path), "%s/%s", display_path, entries[i]);
        }
        printf("%s\n", child_display_path);
        if(is_directory(child_path) && strcmp(entries[i], ".") != 0 && strcmp(entries[i], "..") != 0){
            reveal_recursive(child_path, child_display_path, show_hidden);
        }
        free(entries[i]);
    }
    return 0;
}

int reveal(token_list* tokens){
    int show_hidden = 0;
    int recursive = 0;
    token_node* curr = tokens->head;
    curr = curr->next;
    while(curr != NULL && curr->token.type == token_word && parse_flags(curr->token.value, &show_hidden, &recursive)){
        curr = curr->next;
    }
    if(curr != NULL && curr->next != NULL){
        fprintf(stderr, "reveal: too many arguments\n");
        return 1;
    }

    char target_path[PATH_BUFFER_SIZE];
    if(curr == NULL) strcpy(target_path, ".");
    else if(strcmp(curr->token.value, "~") == 0) strcpy(target_path, reveal_home);
    else if(strcmp(curr->token.value, ".") == 0) strcpy(target_path, ".");
    else if(strcmp(curr->token.value, "..") == 0) strcpy(target_path, "..");
    else if(strcmp(curr->token.value, "-") == 0){
        const char* prev = hop_previous_dir();
        if(prev == NULL){
            fprintf(stderr, "reveal: no previous directory\n");
            return 1;
        }
        strcpy(target_path, prev);
    }
    else if(curr->token.value[0] == '~' && curr->token.value[1] == '/'){
        if(strlen(reveal_home) + strlen(curr->token.value) >= sizeof(target_path)) {
            fprintf(stderr, "reveal: path too long\n");
            return 1;
        }
        strcpy(target_path, reveal_home);
        strcat(target_path, curr->token.value + 1);
    }
    else strcpy(target_path, curr->token.value);

    if(recursive){
        return reveal_recursive(target_path, "", show_hidden);
    }

    DIR* directory;
    struct dirent* entry;
    char* entries[MAX_ENTRIES];
    int count = 0;

    directory = opendir(target_path);

    if(directory == NULL){
        fprintf(stderr, "reval: no such directory\n");
        return 1;
    }

    while((entry = readdir(directory)) != NULL){
        if(entry->d_name[0] == '.' && !show_hidden){
            continue;
        }
        if(count >= MAX_ENTRIES){
            break;
        }
        entries[count] = malloc(strlen(entry->d_name) + 1);
        if(entries[count] == NULL){
            closedir(directory);
            for(int i=0;i<count;i++) free(entries[i]);
            return 1;
        }
        strcpy(entries[count], entry->d_name);
        count++;
    }
    closedir(directory);
    qsort(entries, count, sizeof(char*), cmp);
    for(int i=0;i<count;i++){
        printf("%s\n",entries[i]);
        free(entries[i]);
    }
    return 0;
}