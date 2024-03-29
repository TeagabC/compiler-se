#include "codegen.h"
#include "ast_types.h"
#include "scope.h"
#include "symbol.h"

#include <cassert>

#include <cstdio>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <llvm-c/Types.h>
#include <stack>
#include <string>

#define CODEGEN_DEBUG_SCOPES

#ifdef CODEGEN_DEBUG

#endif

#ifdef CODEGEN_DEBUG_SCOPES
#define DEBUG_PRINT_SCOPE(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT_SCOPE(...)
#endif

LLVMModuleRef module;
LLVMBuilderRef builder;

namespace codegen {

unsigned scope_index = 0;
Scope* current_scope = nullptr;

LLVMValueRef* current_func;
LLVMBasicBlockRef* while_check;
LLVMBasicBlockRef* while_end;

struct LLVMTypedValue {
  LLVMTypeRef type;
  LLVMValueRef value;
};

std::stack<Scope*> stack;

void pushScope(void* node) {
  DEBUG_PRINT_SCOPE("Scope push: prev: %p ", current_scope); 
  stack.push(current_scope);
  current_scope = scopeGet(node);
  DEBUG_PRINT_SCOPE("new: %p\n", current_scope);
  assert(current_scope != nullptr);
}

void popScope() {
  DEBUG_PRINT_SCOPE("Scope pop: prev: %p ", current_scope);
  current_scope = stack.top();
  DEBUG_PRINT_SCOPE("new: %p\n", current_scope);
  stack.pop();
}

Symbol* identifierDef(Identifier* node) {
  // TODO: it seems like this might need to do more but im not sure
  Symbol* symbol = scopeResolve(current_scope, node->identifier->start, node->identifier->end);
  for (auto i = node->identifier->start; i != node->identifier->end; i++) {
    fprintf(stderr, "%c", *i);
  }
  fprintf(stderr, "\n");
  assert(symbol != nullptr);
  return symbol;
}

LLVMTypedValue identifierRef(Identifier* node) {
  Symbol* symbol = scopeResolve(current_scope, node->identifier->start, node->identifier->end);
  Token* next_id;
  LLVMValueRef indices[64];
  Symbol* child_sym;
  LLVMTypeRef struct_type = symbol->llvm_type;
  LLVMValueRef output = symbol->llvm_value;
  int i = 0;
  switch (symbol->type) {
    case SymbolType::BOOL:
    case SymbolType::I8:
    case SymbolType::U8:
    case SymbolType::I32:
    case SymbolType::U32:
    case SymbolType::F32:
    case SymbolType::ENUM_INSTANCE:
      return {symbol->llvm_type, symbol->llvm_value};
    // TODO handle pointers and strings
    case SymbolType::STRUCT_INSTANCE:
      if (node->next == nullptr) {
        return {symbol->llvm_type, symbol->llvm_value};
      }

      indices[0] = LLVMConstInt(LLVMInt32Type(), 0, false);
      child_sym = symbol;
      for (; node->next != nullptr; i++) {
        next_id = node->next->identifier;
        child_sym = symbolGetStructChild(child_sym->struct_instance.struct_decl, next_id->start, next_id->end);
        
        indices[i] = child_sym->llvm_value;
        node = node->next;
      }

      output = LLVMBuildGEP2(builder, symbol->llvm_type, symbol->llvm_value, indices, i, "");

    default:
      assert(false && "Value refing incorrect type"); 
  }

  return {child_sym->llvm_type, output};
}

LLVMValueRef identifierValue(Identifier* node) {
  Symbol* symbol = scopeResolve(current_scope, node->identifier->start, node->identifier->end);
  Token* next_id;
  unsigned long enum_value;
  Symbol* child_sym;
  LLVMTypeRef struct_type = symbol->llvm_type;
  LLVMValueRef output = symbol->llvm_value;
  LLVMValueRef indices[64];
  int i = 0;
  switch (symbol->type) {
    case SymbolType::BOOL:
    case SymbolType::I8:
    case SymbolType::U8:
    case SymbolType::I32:
    case SymbolType::U32:
    case SymbolType::F32:
    case SymbolType::ENUM_INSTANCE:
      return LLVMBuildLoad2(builder, symbol->llvm_type, symbol->llvm_value, "");
    // TODO handle pointers and strings

    case SymbolType::ENUM:
      next_id = node->next->identifier;
      enum_value = symbolGetEnumChild(symbol, next_id->start, next_id->end);
      return LLVMConstInt(LLVMInt32Type(), enum_value, false);

    case SymbolType::STRUCT_INSTANCE:
      if (node->next == nullptr) {
        return LLVMBuildLoad2(builder, symbol->llvm_type, symbol->llvm_value, "");
      }

      indices[0] = LLVMConstInt(LLVMInt32Type(), 0, false);
      child_sym = symbol;
      for (; node->next != nullptr; i++) {
        next_id = node->next->identifier;
        child_sym = symbolGetStructChild(child_sym->struct_instance.struct_decl, next_id->start, next_id->end);
        
        indices[i] = child_sym->llvm_value;
        node = node->next;
      }

      output = LLVMBuildGEP2(builder, symbol->llvm_type, symbol->llvm_value, indices, i, "");
      return LLVMBuildLoad2(builder, child_sym->llvm_type, output, "");

    default:
      assert(false && "identifierValue");
  }
}

LLVMTypeRef type(Type* node) {
  Symbol* symbol;
  if (node->type == ASTType::TYPE_SIMPLE) {

    switch (node->simple_type->type) {
      case ASTType::SIMPLE_TYPE_I8:
      case ASTType::SIMPLE_TYPE_U8:
        return LLVMInt8Type();
      case ASTType::SIMPLE_TYPE_I32:
      case ASTType::SIMPLE_TYPE_U32:
        return LLVMInt32Type();
      case ASTType::SIMPLE_TYPE_F32:
        return LLVMFloatType();
      default:
        assert(false && "Type not simple as advertised");
    }
  }
  else if (node->type == ASTType::TYPE_ID) {
    symbol = identifierDef(node->identifier);
    switch (symbol->type) {
      case SymbolType::ENUM:
        return LLVMInt32Type();
      case SymbolType::STRUCT:
        return symbol->llvm_type;
      default:
        assert(false && "Type isn't a instantiable type");
    }
  }
  else {
    assert(false && "Type");
  }
}

void qualifier(Qualifier* node) {
  switch (node->type) {
    case ASTType::QUALIFIER_CONST:
      break;
    case ASTType::QUALIFIER_EXPORT:
      break;
    case ASTType::QUALIFIER_MUT:
      break;
    default:
      assert(false && "Qualifier");
  }
}

LLVMValueRef expr(Expr* node);
LLVMBasicBlockRef block(Block* node);

void declaration(Declaration* node) {
  Symbol* symbol = identifierDef(node->identifier);
  LLVMTypeRef llvm_type = type(node->decl_type);

  // TODO this probably doesn't need to do anything
  // maybe for export though?
  for (int i = 0; i < node->qualifiers_count; i++) {
    qualifier(node->qualifiers[i]);
  }

  // TODO: maybe this should insert the alloca at the top of the function

  symbol->llvm_type = llvm_type;
  symbol->llvm_value = LLVMBuildAlloca(builder, llvm_type, "");

  if (node->expr != nullptr) {
    LLVMValueRef value = expr(node->expr);
    LLVMBuildStore(builder, value, symbol->llvm_value);
  }
}

void assignment(Assignment* node) {
  LLVMValueRef rhs = expr(node->expr);
  LLVMTypedValue lhs = identifierRef(node->identifier);
  LLVMBuildStore(builder, rhs, lhs.value);
}

void conditional(Conditional* node) {
  LLVMBasicBlockRef current = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef iftrue = block(node->block);
  LLVMBasicBlockRef iffalse;
  LLVMBasicBlockRef end;
  LLVMValueRef condition;

  if (node->other != nullptr) {
    iffalse = block(node->other);
  }
  else {
    iffalse = LLVMAppendBasicBlock(*current_func, "");
  }

  end = LLVMAppendBasicBlock(*current_func, "");

  LLVMPositionBuilderAtEnd(builder, iftrue);
  LLVMBuildBr(builder, end);

  LLVMPositionBuilderAtEnd(builder, iffalse);
  LLVMBuildBr(builder, end);
  
  LLVMPositionBuilderAtEnd(builder, current);

  condition = expr(node->condition);
  LLVMBuildCondBr(builder, condition, iftrue, iffalse);

  LLVMPositionBuilderAtEnd(builder, end);
}



void while_(While* node) {
  LLVMBasicBlockRef current = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef check = LLVMAppendBasicBlock(*current_func, "");
  LLVMBasicBlockRef loop;
  LLVMBasicBlockRef end;
  LLVMBasicBlockRef* current_while_check = while_check;
  LLVMBasicBlockRef* current_while_end = while_end;

  while_check = &check;
  // TODO: &end is curently nullptr and will be when block is visited
  while_end = &end;

  loop = block(node->block);
  LLVMBuildBr(builder, check);

  LLVMPositionBuilderAtEnd(builder, current);
  LLVMBuildBr(builder, check);

  LLVMPositionBuilderAtEnd(builder, check);
  LLVMValueRef condition = expr(node->condition);
  LLVMValueRef cond_value = LLVMBuildICmp(builder, LLVMIntEQ, LLVMConstInt(LLVMInt8Type(), 0, false), condition, "");
  LLVMBuildCondBr(builder, cond_value, end, loop);

  LLVMPositionBuilderAtEnd(builder, end);

  while_check = current_while_check;
  while_end = current_while_end;
}

void break_(Break* node) {

}

void continue_(Continue* node) {
  LLVMBuildBr(builder, *while_check);
}

void return_(Return* node) {
  if (node->expr != nullptr) {
    LLVMValueRef value = expr(node->expr);
    LLVMBuildRet(builder, value);
    return;
  }
  LLVMBuildRetVoid(builder);
}

void statement(Statement* node) {
  switch (node->type) {
    case ASTType::STATEMENT_CONDITION:
      conditional(node->conditional);
      break;
    case ASTType::STATEMENT_WHILE:
      while_(node->while_);
      break;
    case ASTType::STATEMENT_BREAK:
      break_(node->break_);
      break;
    case ASTType::STATEMENT_CONTINUE:
      continue_(node->continue_);
      break;
    case ASTType::STATEMENT_RETURN:
      return_(node->return_);
      break;
    case ASTType::STATEMENT_ASSIGN:
      assignment(node->assignment);
      break;
    case ASTType::STATEMENT_EXPR:
      expr(node->expr);
      break;
    default:
      assert(false && "Statement");
  }
}

void blockTag(BlockTag* node) {
  switch (node->type) {
    case ASTType::BLOCK_TAG_BLOCK:
      block(node->block);
      break;
    case ASTType::BLOCK_TAG_STATEMENT:
      statement(node->statement);
      break;
    default:
      assert(false && "Block Tag");
  }
}

LLVMBasicBlockRef block(Block* node) {
  pushScope(node);

  LLVMBasicBlockRef current = LLVMAppendBasicBlock(*current_func, "");
  LLVMBuildBr(builder, current);
  LLVMPositionBuilderAtEnd(builder, current);

  // TODO: implement
  if (node->namespace_ != nullptr) {
    //identifierDef(node->namespace_);
  }

  if (node->statement != nullptr) {
    statement(node->statement);
  }

  for (int i = 0; i < node->declarations_count; i++) {
    declaration(node->declarations[i]);
  }

  for (int i = 0; i < node->block_tags_count; i++) {
    blockTag(node->block_tags[i]);
  }

  popScope();

  return current;
}

LLVMValueRef literal(Literal* node) {
  switch (node->type) {
    case ASTType::LITERAL_STRING:
      return LLVMConstString(node->start->start, node->start->end - node->start->end, false);

    case ASTType::LITERAL_INT:
      return LLVMConstInt(LLVMInt32Type(), std::stol(std::string {node->start->start, node->start->end}), false);

    case ASTType::LITERAL_FLOAT:
      return LLVMConstReal(LLVMFloatType(), std::stof(std::string {node->start->start, node->start->end}));

    case ASTType::LITERAL_BOOL:
      return LLVMConstInt(LLVMInt8Type(), node->bool_, false);

    default:
      assert(false && "Literal");
  }
}

LLVMValueRef call(Call* node) {
  Symbol* symbol = identifierDef(node->identifier);

  LLVMValueRef args[64];

  int i = 0;
  for (; i < node->arguments_count; i++) {
    args[i] = expr(node->arguments[i]);
  }

  return LLVMBuildCall2(builder, symbol->llvm_type, symbol->llvm_value, args, i, "");
}

LLVMValueRef unary(Unary* node) {
  // TODO: this needs to be more type aware
  // LLVMBuildFNeg exists
  LLVMValueRef value = expr(node->expr);
  switch (node->type) {
    case ASTType::UNARY_NOT:
      return LLVMBuildNot(builder, value, "");
      break;
    case ASTType::UNARY_PLUS:
      return value;
      break;
    case ASTType::UNARY_MINUS:
      return LLVMBuildNeg(builder, value, "");
    default:
      assert(false && "Unary");
  }
}

LLVMValueRef binary(Binary* node) {
  // TODO: this needs to know if the types are signed or unsigned 
  // as well as float or int
  LLVMValueRef lhs = expr(node->first);
  LLVMValueRef rhs = expr(node->second);

  switch (node->type) {
    case ASTType::BINARY_ADD:
      return LLVMBuildAdd(builder, lhs, rhs, "");

    case ASTType::BINARY_SUB:
      return LLVMBuildSub(builder, lhs, rhs, "");

    case ASTType::BINARY_MUL:
      return LLVMBuildMul(builder, lhs, rhs, "");

    case ASTType::BINARY_DIV:
      return LLVMBuildUDiv(builder, lhs, rhs, "");

    case ASTType::BINARY_MOD:
      return LLVMBuildSub(builder, lhs, rhs, "");

    case ASTType::BINARY_LT:
      return LLVMBuildICmp(builder, LLVMIntULT, lhs, rhs, "");

    case ASTType::BINARY_GT:
      return LLVMBuildICmp(builder, LLVMIntUGT, lhs, rhs, "");

    case ASTType::BINARY_LE:
      return LLVMBuildICmp(builder, LLVMIntULE, lhs, rhs, "");

    case ASTType::BINARY_GE:
      return LLVMBuildICmp(builder, LLVMIntUGE, lhs, rhs, "");

    case ASTType::BINARY_EQ:
      return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "");

    case ASTType::BINARY_NE:
      return LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "");

    case ASTType::BINARY_AND:
      return LLVMBuildAnd(builder, lhs, rhs, "");

    case ASTType::BINARY_OR:
      return LLVMBuildOr(builder, lhs, rhs, "");

    case ASTType::BINARY_XOR:
      return LLVMBuildXor(builder, lhs, rhs, "");

    default:
      assert(false && "Binary");
  }
}

LLVMValueRef expr(Expr* node) {
  switch (node->type) {
    case ASTType::EXPRESSION_CALL:
      return call(node->call);

    case ASTType::EXPRESSION_UNARY:
      return unary(node->unary);

    case ASTType::EXPRESSION_BINARY:
      return binary(node->binary);
      
    case ASTType::EXPRESSION_IDENTIFIER:
      return identifierValue(node->identifier);

    case ASTType::EXPRESSION_LITERAL:
      return literal(node->literal);
      
    default:
      assert(false && "Expr");
  }
}

void function(Function* node) {
  Symbol* symbol = identifierDef(node->header->identifier);
  LLVMTypeRef return_type = type(node->header->return_type);
  LLVMTypeRef param_types[64];

  int i = 0;
  for (;i < node->header->parameter_count; i++) {
    LLVMTypeRef param_type = type(node->header->parameter_list[i]->decl_type);
    param_types[i] = param_type;
  }


  // TODO: this is so hacky
  char c = *symbol->end;
  *(char*)symbol->end = '\0';
  symbol->llvm_type = LLVMFunctionType(return_type, param_types, i, false);
  symbol->llvm_value = LLVMAddFunction(module, symbol->start, symbol->llvm_type);
  *(char*)symbol->end = c;

  LLVMBasicBlockRef init_block = LLVMAppendBasicBlock(symbol->llvm_value, "");
  LLVMPositionBuilderAtEnd(builder, init_block);

  pushScope(node);

  for (int j = 0; j < node->header->parameter_count; j++) {
    Symbol* param_sym = identifierDef(node->header->parameter_list[j]->identifier);
    param_sym->llvm_type = param_types[j];
    param_sym->llvm_value = LLVMBuildAlloca(builder, param_types[j], "");
    LLVMBuildStore(builder, LLVMGetParam(symbol->llvm_value, j), param_sym->llvm_value);
  }

  current_func = &symbol->llvm_value;
/* TODO : either implement or remove this
  if (node->expr != nullptr) {
    expr(node->expr);
  }
*/
  if (node->block != nullptr) {
    block(node->block);
  }

  popScope();

  current_func = nullptr;
}

void struct_(Struct* node) {
  Symbol* symbol = identifierDef(node->identifier);
  LLVMTypeRef struct_types[64];

  // TODO: this is so hacky
  char c = *symbol->end;
  *(char*)symbol->end = '\0';
  symbol->llvm_type = LLVMStructCreateNamed(LLVMGetGlobalContext(), symbol->start);
  *(char*)symbol->end = c;

  pushScope(node);
  int i = 0;
  for (; i < node->declarations_count; i++) {
    Symbol* child = identifierDef(node->declarations[i]->identifier);
    child->llvm_value = LLVMConstInt(LLVMInt32Type(), i, false);
    child->llvm_type = type(node->declarations[i]->decl_type);
    struct_types[i] = child->llvm_type;
  }

  popScope();

  LLVMStructSetBody(symbol->llvm_type, struct_types, i, false);
}

void enum_(Enum* node) {
  /*
  identifier(node->identifier);
  
  for (int i = 0; i < node->members_count; i++) {
  }
  */
}

void primaryTag(PrimaryTag* node) {
  switch (node->type) {
    case ASTType::PRIMARY_TAG_DECL:
      // TODO: build a global yourself
      declaration(node->decl);
      break;
    case ASTType::PRIMARY_TAG_ENUM:
      enum_(node->enum_);
      break;
    case ASTType::PRIMARY_TAG_FUNC:
      function(node->func);
      break;
    case ASTType::PRIMARY_TAG_STRUCT:
      struct_(node->struct_);
      break;
    default:
      assert(false && "Primary Tag");
  }
}

void primary(Primary* node) {
  for (int i = 0; i < node->primary_tags_count; i++) {
    primaryTag(node->primary_tags[i]);
  }
}

};

void visitCodeGen(Primary* node) {
  char* error = nullptr;
  module = LLVMModuleCreateWithName("main_module");

  builder = LLVMCreateBuilder();

  LLVMValueRef global = LLVMAddGlobal(module, LLVMInt32Type(), "name");

  codegen::scope_index = 0;
  codegen::current_scope = scopeGet(node);
  codegen::primary(node);

  LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
  LLVMDisposeMessage(error);
  error = nullptr;

  LLVMDumpModule(module);
  // TODO: LLVMPrintModuleToFile(LLVMModuleRef M, const char *Filename, char **ErrorMessage)

  LLVMDisposeBuilder(builder);

}

void codegen_destroy() {
  LLVMDisposeModule(module);
}
