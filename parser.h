#pragma once
#include "lex.h"
#include "parser_types.h"

Primary* parse(Token* tokens);
void parse_destroy();

