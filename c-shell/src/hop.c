#include "hop.h"

#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>

#define PATH_BUFFER_SIZE 4096
#define MAX_VISITED 1024

typedef struct{
    char path[PATH_BUFFER_SIZE];
    int score;
}vis_dir;


static char hop_home[PATH_BUFFER_SIZE];
static char hop_prev[PATH_BUFFER_SIZE];
static int has_prev = 0;
static vis_dir visited[MAX_VISITED];
static int vis_count = 0;


void hop_init(){
    if(getcwd(hop_home, sizeof(hop_home)) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}

static void record_visit(const char* path){
    for(int i=0;i<vis_count;i++){
        if(strcmp(visited[i].path, path) == 0){
            visited[i].score++;
            return;
        }
    }
    if(vis_count>=MAX_VISITED) return;

    strcpy(visited[vis_count].path, path);
    visited[vis_count].score=1;
    vis_count++;
}

static int change_dir(const char* path){
    char curr[PATH_BUFFER_SIZE];
    char true_path[PATH_BUFFER_SIZE];
    if(getcwd(curr, sizeof(curr)) == NULL){
        return 1;
    }

    if(chdir(path) != 0){
        return 1;
    }

    if(getcwd(true_path, sizeof(true_path)) == NULL){
        return 1;
    }

    strcpy(hop_prev, curr);
    has_prev = 1;
    
    record_visit(true_path);
    return 0;
}

static const char* best_match(const char* name){
    int best = -1;
    for(int i=0;i<vis_count;i++){
        if(strstr(visited[i].path, name) == NULL){
            continue;
        }
        if(best == -1 ||
           visited[i].score > visited[best].score ||
           (strcmp(visited[i].path, visited[best].path) < 0 && visited[i].score == visited[best].score))
        {
            best = i;
        }
    }
    if(best == -1) return NULL;
    return visited[best].path;
}

int hop(token_list* tokens){
    token_node* curr = tokens->head;
    curr = curr->next;
    char expanded_path[PATH_BUFFER_SIZE];
    if(curr == NULL){
        return change_dir(hop_home);
    }

    if(strcmp(curr->token.value, "~") == 0){
        return change_dir(hop_home);
    }

    if(curr->token.value[0] == '~' && curr->token.value[1] == '/'){
        strcpy(expanded_path, hop_home);
        strcat(expanded_path, curr->token.value + 1);
        return change_dir(expanded_path);
    }

    if(strcmp(curr->token.value, "..") == 0){
        return change_dir("..");
    }

    if(strcmp(curr->token.value, ".") == 0){
        return change_dir(".");
    }

    if(strcmp(curr->token.value, "-") == 0){
        if(!has_prev) return 0;
        return change_dir(hop_prev);
    }

    if(change_dir(curr->token.value) == 0){
        return 0;
    }

    const char* match = best_match(curr->token.value);
    if(match == NULL){
        fprintf(stderr, "hop: no such directory\n");
        return 1;
    }
    
    if(change_dir(match) != 0){
        fprintf(stderr, "hop: no such directory\n");
        return 1;
    }
    return 0;
}