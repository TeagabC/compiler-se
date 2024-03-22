#pragma once
#include "lex.h"
#include "ast_types.h"

Primary* parse(Token* tokens);
void parse_destroy();

