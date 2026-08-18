#ifndef HOP_H
#define HOP_H

#include "lexer.h"

void hop_init();
int hop(token_list* tokens);
const char* hop_previous_dir();

#endif
