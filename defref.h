#pragma once
#include "ast_types.h"

/* DefRef creates and allocates the scope tree and its symbols
 * It also checks all defs and refs to make sure their types match up
 */

void visitDefRef(Primary* node);
void defref_destroy();
