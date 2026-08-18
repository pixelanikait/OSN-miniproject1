#include "hop.h"

#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>

#define HISTORY_FILE_NAME "history.csv"

#define PATH_BUFFER_SIZE 4096
#define MAX_VISITED 1024

typedef struct{
    char path[PATH_BUFFER_SIZE];
    int score;
    unsigned long last_access;
}vis_dir;


static char hop_home[PATH_BUFFER_SIZE];
static char hop_prev[PATH_BUFFER_SIZE];
static int has_prev = 0;
static vis_dir visited[MAX_VISITED];
static int vis_count = 0;
static unsigned long access_counter = 0;
static char history_file[PATH_BUFFER_SIZE];

static void save_history(){
    FILE* file;
    file = fopen(history_file,"w");
    if(file == NULL) return;
    for(int i=0;i<vis_count;i++){
        fprintf(file, "%d,%lu,%s\n",
                        visited[i].score,
                        visited[i].last_access,
                        visited[i].path);
    }
    fclose(file);
}

static void load_history(){
    FILE* file;
    char line [PATH_BUFFER_SIZE + 64];

    file = fopen(history_file,"r");

    if(file == NULL) return;
    while(fgets(line, sizeof(line), file) != NULL){
        int score;
        unsigned long lst_ac;
        char path[PATH_BUFFER_SIZE];
        if(sscanf(line, "%d,%lu,%4095[^\n]",
                    &score,
                    &lst_ac,
                    path) != 3) continue;
        if(vis_count >= MAX_VISITED) break;
        strcpy(visited[vis_count].path, path);
        visited[vis_count].score=score;
        visited[vis_count].last_access = lst_ac;
        if(lst_ac > access_counter) access_counter = lst_ac;
        vis_count++;
    }
    fclose(file);
}

void hop_init(){
    if(getcwd(hop_home, sizeof(hop_home)) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    if (strlen(hop_home) + 1 + strlen(HISTORY_FILE_NAME) >= sizeof(history_file)) {
        fprintf(stderr, "hop: history path is too long\n");
        exit(EXIT_FAILURE);
    }
    strcpy(history_file, hop_home);
    strcat(history_file, "/");
    strcat(history_file, HISTORY_FILE_NAME);
    load_history();
}

const char* hop_previous_dir(){
    if(!has_prev) return NULL;
    return hop_prev;
}

static void record_visit(const char* path){
    for(int i=0;i<vis_count;i++){
        if(strcmp(visited[i].path, path) == 0){
            access_counter++;
            visited[i].score++;
            visited[i].last_access = access_counter;
            return;
        }
    }
    if(vis_count>=MAX_VISITED) return;

    strcpy(visited[vis_count].path, path);
    access_counter++;
    visited[vis_count].score=1;
    visited[vis_count].last_access = access_counter;
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
    save_history();
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
           (visited[i].last_access > visited[best].last_access && visited[i].score == visited[best].score))
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