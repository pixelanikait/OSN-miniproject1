#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "prompt.h"

#define PATH_BUFFER_SIZE 4096
#define HOSTNAME_BUFFER_SIZE 256

static char shell_home[PATH_BUFFER_SIZE];

void prompt_init(){
    if(getcwd(shell_home, sizeof(shell_home)) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}

static void get_prompt_path(char* prompt_path){
    char current_path[PATH_BUFFER_SIZE];
    if(getcwd(current_path, sizeof(current_path)) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    size_t home_length = strlen(shell_home);

    //for directory where shell is executed
    if(strcmp(current_path, shell_home) == 0){
        strcpy(prompt_path, "~");
        return;
    }

    //cwd is inside dir where shell launched
    if(strncmp(current_path, shell_home, home_length) == 0 && current_path[home_length] == '/'){
        strcpy(prompt_path, "~");
        strcat(prompt_path, current_path + home_length);
        return;
    }

    //otherwise absolute path
    strcpy(prompt_path, current_path);
}

void show_prompt(){
    struct passwd* user_info;
    char hostname[HOSTNAME_BUFFER_SIZE];
    char path[PATH_BUFFER_SIZE];
    user_info = getpwuid(getuid());

    if(user_info == NULL){
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }

    if(gethostname(hostname, sizeof(hostname)) != 0){
        perror("gethostname");
        exit(EXIT_FAILURE);
    }

    hostname[sizeof(hostname) - 1] = '\0';
    get_prompt_path(path);
    printf("%s@%s:%s> ", user_info->pw_name, hostname, path);

    fflush(stdout);
}
