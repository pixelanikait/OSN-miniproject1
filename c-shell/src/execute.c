#include "execute.h"
#include "hop.h"
#include "reveal.h"
#include "peek.h"
#include "locate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PATH_BUFFER_SIZE 4096
#define MAX_PIPE_COMMANDS 256

static int is_exec(const char* path){
    return access(path,X_OK) == 0;
}

static int has_pipe(const token_list* tokens){
    token_node* curr = tokens->head;
    while(curr != NULL){
        if(curr->token.type == token_pipe){
            return 1;
        }
        curr = curr->next;
    }
    return 0;
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

static int add_token_copy(token_list* list, const token_node* source){
    token_node* node = malloc(sizeof(token_node));
    if(node == NULL) return 0;
    node->token.type = source->token.type;
    node->token.value = malloc(strlen(source->token.value) + 1);

    if(node->token.value == NULL){
        free(node);
        return 0;
    }

    strcpy(node->token.value, source->token.value);
    node->next = NULL;

    if(list->head == NULL){
        list->head=node;
        list->tail=node;
    }
    else{
        list->tail->next=node;
        list->tail=node;
    }
    return 1;
}

static int split_at_pipe(const token_list* tokens, token_list* commands, int* count){
    for(int i=0;i<MAX_PIPE_COMMANDS ;i++){
        commands[i].head = NULL;
        commands[i].tail = NULL;
    }

    int command_count = 0;

    if(tokens->head == NULL) return 0;

    command_count = 1;
    token_node* curr = tokens->head;
    while(curr != NULL){
        if(curr->token.type == token_pipe){
            if(commands[command_count - 1].head == NULL) return 0;
            if(command_count >= MAX_PIPE_COMMANDS) return 0;
            command_count++;
        }
        else{
            if(!add_token_copy(&commands[command_count - 1], curr)){
                for(int i=0;i<command_count;i++){
                    free_tokens(&commands[i]);
                }
                return 0;
            }
        }
        curr = curr->next;
    }
    if(commands[command_count - 1].head == NULL){
        for(int i=0;i<command_count;i++){
            free_tokens(&commands[i]);
        }
        return 0;
    }
    *count = command_count;
    return 1;
}

static int build_argv(token_list* tokens, char** argv, int max_args){
    token_node* curr = tokens->head;
    int argc = 0;
    while(curr != NULL){
        if(curr->token.type == token_lt || curr->token.type == token_gt || curr->token.type == token_gtgt){
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
    argv[argc] = NULL;
    return argc > 0;
}

static int build_builtin_tokens(token_list* tokens,token_list* clean_tokens){
    token_node* curr = tokens->head;
    clean_tokens->head = NULL;
    clean_tokens->tail = NULL;
    while(curr != NULL){
        if(curr->token.type == token_lt ||
           curr->token.type == token_gt ||
           curr->token.type == token_gtgt){
            if(curr->next == NULL ||
               curr->next->token.type != token_word){
                free_tokens(clean_tokens);
                return 0;
            }
            curr = curr->next->next;
            continue;
        }
        if(curr->token.type == token_pipe ||
           curr->token.type == token_semi ||
           curr->token.type == token_amp){
            break;
        }
        token_node* node = malloc(sizeof(token_node));
        if(node == NULL){
            free_tokens(clean_tokens);
            return 0;
        }
        node->token.type = curr->token.type;
        node->token.value = malloc(strlen(curr->token.value) + 1);
        if(node->token.value == NULL){
            free(node);
            free_tokens(clean_tokens);
            return 0;
        }
        strcpy(node->token.value, curr->token.value);
        node->next = NULL;

        if(clean_tokens->head == NULL){
            clean_tokens->head = node;
            clean_tokens->tail = node;
        }
        else{
            clean_tokens->tail->next = node;
            clean_tokens->tail = node;
        }
        curr = curr->next;
    }
    return 1;
}

static int set_input_redirect(token_list* tokens){
    token_node* curr = tokens->head;
    while(curr != NULL){
        if(curr->token.type == token_lt){
            if(curr->next == NULL || curr->next->token.type != token_word){
                fprintf(stderr, "cshell: invalid input redirection\n");
                return 0;
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

static int set_output_redirect(token_list* tokens){
    token_node* curr = tokens->head;
    while(curr != NULL){
        if(curr->token.type == token_gt || curr->token.type == token_gtgt){
            if(curr->next == NULL || curr->next->token.type != token_word){
                fprintf(stderr, "cshell: invalid output redirection\n");
                return 0;
            }
            int flags;
            if(curr->token.type == token_gt) flags = O_WRONLY | O_CREAT | O_TRUNC;
            else flags = O_WRONLY | O_CREAT | O_APPEND;

            int fd = open(curr->next->token.value, flags, 0644);
            if(fd <  0){
                perror(curr->next->token.value);
                return 0;
            }
            if(dup2(fd, STDOUT_FILENO) < 0){
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


static int run_builtin(token_list* tokens){
    token_node* curr = tokens->head;
    if(curr == NULL) return -1;
    if(strcmp(curr->token.value, "hop") == 0) return hop(tokens);
    if(strcmp(curr->token.value, "peek") == 0) return peek(tokens);
    if(strcmp(curr->token.value, "reveal") == 0) return reveal(tokens);
    if(strcmp(curr->token.value, "locate") == 0) return locate(tokens);
    return -1;
}

static int run_command_child(token_list* tokens){
    char* argv[256];
    char resolved_path[PATH_BUFFER_SIZE];

    if(!set_input_redirect(tokens)) return 1;
    if(!set_output_redirect(tokens)) return 1;
    
    token_list clean_tokens;
    if(!build_builtin_tokens(tokens, &clean_tokens)) return 1;
    int is_builtin = run_builtin(&clean_tokens);
    if(is_builtin != -1){
        free_tokens(&clean_tokens);
        return is_builtin;
    }
    free_tokens(&clean_tokens);

    if(!build_argv(tokens, argv, 256)) return 1;
    if(!resolve(argv[0], resolved_path)){
        fprintf(stderr, "cshell: command not found(%s)\n", argv[0]);
        return 127;
    }
    execv(resolved_path, argv);
    perror("exec");
    return 127;
}

static int execute_pipeline(token_list* tokens){
    token_list commands[MAX_PIPE_COMMANDS];
    int command_count = 0;

    if(!split_at_pipe(tokens, commands, &command_count)){
        fprintf(stderr, "cshell: invalid pipeline\n");
        return 1;
    }
    int pipes[MAX_PIPE_COMMANDS - 1][2];
    for(int i = 0; i < command_count - 1; i++){
        if(pipe(pipes[i]) < 0){
            perror("pipe");
            for(int j = 0; j < command_count; j++){
                free_tokens(&commands[j]);
            }
            return 1;
        }
    }
    pid_t pids[MAX_PIPE_COMMANDS];
    for(int i = 0; i < command_count; i++){
        pids[i] = fork();
        if(pids[i] < 0){
            perror("fork");
            for(int j = 0; j < command_count - 1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            for(int j = 0; j < command_count; j++){
                free_tokens(&commands[j]);
            }
            return 1;
        }

        if(pids[i] == 0){
            if(i > 0){
                if(dup2(pipes[i - 1][0], STDIN_FILENO) < 0){
                    perror("dup2");
                    _exit(1);
                }
            }
            if(i < command_count - 1){
                if(dup2(pipes[i][1], STDOUT_FILENO) < 0){
                    perror("dup2");
                    _exit(1);
                }
            }
            for(int j = 0; j < command_count - 1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            int status = run_command_child(&commands[i]);
            _exit(status);
        }
    }
    for(int i = 0; i < command_count - 1; i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for(int i = 0; i < command_count; i++){
        waitpid(pids[i], NULL, 0);
    }
    for(int i = 0; i < command_count; i++){
        free_tokens(&commands[i]);
    }
    return 0;
}

int execute(token_list* tokens){
    if(has_pipe(tokens)) return execute_pipeline(tokens);
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