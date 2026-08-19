#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PATH_BUFFER_SIZE 4096

static int is_exec(const char* path){
    return access(path,X_OK) == 0;
}

static int resolve(const char* command, char* resolved_path){
    if(strchr(command, '/') != NULL){
        if(is_exec(command)){
            if(strlen(command) >= PATH_BUFFER_SIZE) return 0;
            strcpy(resolved_path, command);
            return 1;
        }
        return 0;
    }

    const char* path_command = command;
    int skip_cwd = 0;
    if(command[0] == '%'){
        skip_cwd = 1;
        path_command = command + 1;
        if(path_command[0] == '\0') return 0;
    }

    if(!skip_cwd){
        char cwd[PATH_BUFFER_SIZE];
        if(getcwd(cwd, sizeof(cwd)) == NULL) return 0;
        if(strlen(cwd) + 1 + strlen(path_command) < sizeof(cwd)){
            char candidate[PATH_BUFFER_SIZE];
            strcpy(candidate, cwd);
            strcat(candidate, "/");
            strcat(candidate, path_command);
            if(is_exec(candidate)){
                strcpy(resolved_path, candidate);
                return 1;
            }
        }
    }

    const char* path_env = getenv("PATH");
    if(path_env == NULL) return 0;
    char* path_copy = malloc(strlen(path_env) + 1);
    if(path_copy == NULL) return 0;
    strcpy(path_copy, path_env);

    char* saveptr = NULL;
    char* entry = strtok_r(path_copy, ":", &saveptr);

    while(entry != NULL){
        char candidate[PATH_BUFFER_SIZE];

        if(entry[0] == '\0'){
            char cwd[PATH_BUFFER_SIZE];
            if(getcwd(cwd, sizeof(cwd)) == NULL){
                entry = strtok_r(NULL, ":", &saveptr);
                continue;
            }
            if(strlen(cwd) + 1 + strlen(path_command) >= sizeof(candidate)){
                entry = strtok_r(NULL, ":", &saveptr);
                continue;
            }
            strcpy(candidate, cwd);
            strcat(candidate, "/");
            strcat(candidate, path_command);
        }
        else{
            if(strlen(entry) + 1 + strlen(path_command) >= sizeof(candidate)){
                entry = strtok_r(NULL, ":", &saveptr);
                continue;
            }
            strcpy(candidate, entry);
            strcat(candidate, "/");
            strcat(candidate, path_command);
        }
        if(is_exec(candidate)){
            if(strlen(candidate) >= PATH_BUFFER_SIZE){
                free(path_copy);
                return 0;
            }
            strcpy(resolved_path, candidate);
            free(path_copy);
            return 1;
        }
        entry = strtok_r(NULL, ":", &saveptr);
    }
    free(path_copy);
    return 0;
}

static int build_argv(token_list* tokens, char** argv, int max_args){
    token_node* curr = tokens->head;
    int argc = 0;
    while(curr != NULL){
        if(curr->token.type == token_lt){
            if(curr->next == NULL || curr->next->token.type != token_word){
                return 0;
            }
            curr = curr->next->next;
            continue;
        }
        if(curr->token.type == token_gt || curr->token.type == token_gtgt){
            if(curr->next == NULL || curr->next->token.type != token_word){
                return 0;
            }
            curr = curr->next->next;
            continue;

        }
        if(curr->token.type == token_semi || curr->token.type == token_amp || curr->token.type == token_pipe){
            break;
        }
        if(argc >= max_args - 1){
            return 0;
        }

        argv[argc] = curr->token.value;
        argc++;
        curr = curr->next;
    }
    argc[argv] = NULL;
    return argc > 0;
}

static int set_input_redirect(token_list* tokens){
    token_node* curr = tokens->head;
    while(curr != NULL){
        if(curr->token.type == token_lt){
            if(curr->next == NULL || curr->next->token.type != token_word){
                fprintf(stderr, "cshell: invalid input redirection\n");
            }
            int fd = open(curr->next->token.value, O_RDONLY);
            if(fd < 0){
                perror(curr->next->token.value);
                return 0;
            }
            if(dup2(fd, STDIN_FILENO) < 0){
                perror("dup2");
                close(fd);
                return 0;
            }
            close(fd);
            return 1;
        }
        curr = curr->next;
    }
    return 1;
}

static int run_command_child(token_list* tokens){
    char* argv[256];
    char resolved_path[PATH_BUFFER_SIZE];
    if(!set_input_redirect(tokens)) return 1;
    if(!build_argv(tokens, argv, 256)) return 1;
    if(!resolve(argv[0], resolved_path)){
        fprintf(stderr, "cshell: command not found(%s)\n", argv[0]);
        return 127;
    }
    execv(resolved_path, argv);
    perror("exec");
    return 127;
}

int execute(token_list* tokens){
    pid_t pid = fork();
    if(pid < 0){
        perror("fork");
        return 1;
    }
    if(pid == 0){
        int status = run_command_child(tokens);
        _exit(status);
    }
    int status;
    if(waitpid(pid, &status, 0) < 0){
        perror("waitpid");
        return 1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1; 
}