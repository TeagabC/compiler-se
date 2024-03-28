#pragma once
#include "ast_types.h"
#include <llvm-c/Types.h>

extern LLVMModuleRef module;

void visitCodeGen(Primary* node);
void codegen_destroy();

