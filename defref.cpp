#include "defref.h"
#include "ast_types.h"
#include "scope.h"
#include "symbol.h"

#include <cassert>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#define DEFREF_DEBUG

#ifdef DEFREF_DEBUG
#define DEFREF_CALLSTACK_DEBUG
#define DEFREF_PRINT_DEBUG
#endif

#ifdef DEFREF_CALLSTACK_DEBUG
#include <cstdio>
#define DEBUG_ENTRY() fprintf(stderr, "DEFREF: Enter %s\n", __func__);
#else
#define DEBUG_ENTRY()
#endif

#ifdef DEFREF_PRINT_DEBUG
#include <cstdio>
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

Scope* current_scope;

struct Name {
  const char* start;
  const char* end;
};

namespace defref {

Symbol* createSymbol(Name name, Symbol type_sym) {
  switch (type_sym.type) {
    case SymbolType::STRUCT:
      return symbolCreateStructInstance(name.start, name.end, &type_sym.struct_);

    case SymbolType::ENUM:
      return symbolCreateEnumInstance(name.start, name.end, &type_sym.enum_);
    
    // TODO: special case for pointer

    default:
      return symbolCreateVariable(type_sym.type, name.start, name.end);
  }
}

bool matchAssignmentTypes(Symbol* symbol, Symbol expr_sym) {
  if (symbol->type != expr_sym.type) {
    // TODO: check implicit casting rules
    if (symbol->type == SymbolType::U8 && expr_sym.type == SymbolType::U32) {
      return true;
    }
    if (symbol->type == SymbolType::I8 && 
        (expr_sym.type == SymbolType::U8 || expr_sym.type == SymbolType::U32)) {
      return true;
    }
    if (symbol->type == SymbolType::I32 && expr_sym.type == SymbolType::U32) {
      return true;
    }
        
    assert(false && "Assignment type doesn't match");
  }

  switch (symbol->type) {
    case SymbolType::ENUM_INSTANCE:
      if (symbol->enum_instance.enum_decl != expr_sym.enum_instance.enum_decl) {
        assert(false && "Assignment different enum types");
      }

      break;

    case SymbolType::STRUCT_INSTANCE:
      if (symbol->struct_instance.struct_decl != expr_sym.struct_instance.struct_decl) {
        assert(false && "Assignment different struct types");
      }

    default:
      break;
  }

  return true;
}

bool matchCondition(Symbol expr_sym) {
  if (expr_sym.type != SymbolType::BOOL) {
    // TODO: check if type is castable to bool
    assert(false && "Bool expr isn't bool");
  }
  return true;
}

SymbolType simpleType(SimpleType* node) {
  DEBUG_ENTRY();
  switch (node->type) {
    case ASTType::SIMPLE_TYPE_I8:
      return SymbolType::I8;

    case ASTType::SIMPLE_TYPE_U8:
      return SymbolType::U8;

    case ASTType::SIMPLE_TYPE_I32:
      return SymbolType::I32;

    case ASTType::SIMPLE_TYPE_U32:
      return SymbolType::U32;

    case ASTType::SIMPLE_TYPE_F32:
      return SymbolType::F32;

    default:
      assert(false && "Simple Type");
  }
}
    
Symbol* identifierResolve(Identifier* node) {
  DEBUG_ENTRY();
  Symbol* symbol = scopeResolve(current_scope, node->identifier->start, node->identifier->end);

  if (symbol != nullptr) {
    Identifier* current_id = node->next;
    Symbol* current_sym = symbol;

    if (current_id == nullptr) {
      return symbol;
    }

    while (current_id->next != nullptr) {
      switch (current_sym->type) {
        case SymbolType::ENUM:
          if (current_id->next != nullptr) {
            assert(false && "Enum member dot access");
          }
          // return get enum value from current_sym with current_id

        case SymbolType::STRUCT:
          current_sym = scopeResolveMember(current_sym->struct_.members_table, 
              current_id->identifier->start, current_id->identifier->end);

          current_id = current_id->next;

          if (current_sym == nullptr) {
            assert(false && "identifierResolve: Struct member resolution failed");
          }

          break;

        default:
          assert(false && "Dot access of not a struct or enum");
      }
    }

    return current_sym;
  }

  assert(false && "Failed to resolve identifier");
}

Name identifierDecl(Identifier* node) {
  DEBUG_ENTRY();
  Symbol* symbol = scopeResolveMember(current_scope, node->identifier->start, node->identifier->end);
  if (symbol != nullptr) {
    assert(false && "Redeclaration");
  }

  if (node->next != nullptr) {
    // TODO: maybe support this in the future
    assert(false && "Declaration of dotted identifier");
  }
  return {node->identifier->start, node->identifier->end};
}

Symbol* identifierType(Identifier* node) {
  DEBUG_ENTRY();
  Symbol* symbol = scopeResolve(current_scope, node->identifier->start, node->identifier->end);
 
  if (symbol == nullptr) {
      assert(false && "Failed to resolve type identifier");
  }

  if (node->next != nullptr) {
    Identifier* current_id = node->next;
    Symbol* current_sym = symbol;
    
    while (current_id != nullptr) {
      switch (symbol->type) {
        case SymbolType::STRUCT:
          current_sym = scopeResolveMember(current_sym->struct_.members_table, 
              current_id->identifier->start, current_id->identifier->end);

          current_id = current_id->next;

          if(current_sym == nullptr) {
            assert(false && "identifierType: Struct member resolution failed");
          }
          break;
        default:
          assert(false && "Dot access of not a struct");
      }
    }

    return current_sym;
  }

  return symbol;
}

Symbol type(Type* node) {
  DEBUG_ENTRY();
  switch (node->type) {
    case ASTType::TYPE_SIMPLE:
      return {simpleType(node->simple_type)};
    
    case ASTType::TYPE_ID:
      return *identifierType(node->identifier);

    default:
      assert(false);
  }
}

Symbol qualifier(Qualifier* node) {
  DEBUG_ENTRY();
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
  return {};
}

Symbol expr(Expr* node);
void block(Block* node);

Symbol* declaration(Declaration* node) {
  DEBUG_ENTRY();
  Name id = identifierDecl(node->identifier);
  Symbol type_sym = type(node->decl_type);
  Symbol* symbol = createSymbol(id, type_sym);
  Symbol expr_sym;

  // TODO: implement
  for (int i = 0; i < node->qualifiers_count; i++) {
    qualifier(node->qualifiers[i]);
  }

  // TODO: handle declarations of struct or enum instances

  if (node->expr != nullptr) {
    DEBUG_PRINT("Declaration expr exists");
    expr_sym = expr(node->expr);
    if (!matchAssignmentTypes(symbol, expr_sym)) {
      assert(false && "Declaration expr assignment type doesn't match");
    }
  }

  scopeDeclare(current_scope, symbol);

  return symbol;
}

void assignment(Assignment* node) {
  DEBUG_ENTRY();
  Symbol* symbol = identifierResolve(node->identifier);
  Symbol expr_sym = expr(node->expr);

  if (!matchAssignmentTypes(symbol, expr_sym)) {
    assert(false && "Assignment types don't match");
  }
}

void conditional(Conditional* node) {
  DEBUG_ENTRY();
  Symbol expr_sym = expr(node->condition);

  if (!matchCondition(expr_sym)) {
    assert(false && "Conditional condition is not bool");
  }

  block(node->block);

  if (node->other != nullptr) {
    block(node->other);
  }
}

void while_(While* node) {
  DEBUG_ENTRY();
  Symbol expr_sym = expr(node->condition);
  
  if (!matchCondition(expr_sym)) {
    assert(false && "While condition is not bool");
  }

  block(node->block);
}

void break_(Break* node) {
  DEBUG_ENTRY();
}

void continue_(Continue* node) {
  DEBUG_ENTRY();
}

void return_(Return* node) {
  DEBUG_ENTRY();
  // TODO: we need to manage if we are currentlly in a function
  if (node->expr != nullptr) {
    expr(node->expr);
  }
  else {

  }
}

void statement(Statement* node) {
  DEBUG_ENTRY();
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
  DEBUG_ENTRY();
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

void block(Block* node) {
  DEBUG_ENTRY();
  if (node->namespace_ != nullptr) {
    // TODO: 
    identifierDecl(node->namespace_);
  }

  current_scope = scopeCreate(current_scope, node);

  if (node->statement != nullptr) {
    statement(node->statement);
  }

  for (int i = 0; i < node->declarations_count; i++) {
    declaration(node->declarations[i]);
  }

  for (int i = 0; i < node->block_tags_count; i++) {
    blockTag(node->block_tags[i]);
  }
  
  current_scope = current_scope->parent;
}

Symbol literal(Literal* node) {
  DEBUG_ENTRY();
  switch (node->type) {
    case ASTType::LITERAL_STRING:
      return {SymbolType::STRING};

    case ASTType::LITERAL_INT:
      return {SymbolType::U32};

    case ASTType::LITERAL_FLOAT:
      return {SymbolType::F32};

    case ASTType::LITERAL_BOOL:
      return {SymbolType::BOOL};

    default:
      assert(false && "Literal");
  }
}

Symbol call(Call* node) {
  DEBUG_ENTRY();
  Symbol* func_sym = identifierResolve(node->identifier);

  if (func_sym->type != SymbolType::FUNCTION) {
    assert(false && "Calling a symbol which is not a function");
  }
  
  for (int i = 0; i < node->arguments_count; i++) {
    Symbol expr_sym = expr(node->arguments[i]);
    if (func_sym->function.parameter_vector->size <= i ||
      !matchAssignmentTypes((Symbol*)vecGet(func_sym->function.parameter_vector, i), expr_sym)) {
      assert(false && "Function call parameter types don't match definition");
    }
  }

  if (func_sym->function.struct_return_type != nullptr) {
    Symbol output = {SymbolType::STRUCT_INSTANCE};
    output.struct_instance.struct_decl = func_sym->function.struct_return_type;
    return output;
  }
  else {
    return {func_sym->function.simple_return_type};
  }
}

Symbol unary(Unary* node) {
  DEBUG_ENTRY();
  Symbol symbol = expr(node->expr);
  switch (node->type) {
    case ASTType::UNARY_NOT:
      if (symbol.type != SymbolType::BOOL) assert(false && "Can only not a bool");
      return symbol;
    case ASTType::UNARY_PLUS:
    case ASTType::UNARY_MINUS:
      switch (symbol.type) {
        case SymbolType::I8:
        case SymbolType::U8:
        case SymbolType::I32:
        case SymbolType::U32:
        case SymbolType::F32:
          return symbol;
        default:
          assert(false && "Can only unary plus/minus int or float types");
      }
    default:
      assert(false && "Unary");
  }
  // TODO: use some unary op table instead of this
}

Symbol binary(Binary* node) {
  DEBUG_ENTRY();
  Symbol first = expr(node->first);
  Symbol second = expr(node->second);
  if (first.type != second.type) {
    assert(false && "Binary expression has implicit cast");
  }

  switch (first.type) {
    case SymbolType::I8:
    case SymbolType::U8:
    case SymbolType::I32:
    case SymbolType::U32:
    case SymbolType::F32:
    case SymbolType::BOOL:
      break;
    // TOOD: handle pointers
    default:
      assert(false && "Binary op on non-arithmetic types");
  }

  switch (node->type) {
    case ASTType::BINARY_MUL:
    case ASTType::BINARY_DIV:
    case ASTType::BINARY_MOD:
    case ASTType::BINARY_ADD:
    case ASTType::BINARY_SUB:
      return first;

    case ASTType::BINARY_LT:
    case ASTType::BINARY_GT:
    case ASTType::BINARY_LE:
    case ASTType::BINARY_GE:
    case ASTType::BINARY_EQ:
    case ASTType::BINARY_NE:
      return {SymbolType::BOOL};

    case ASTType::BINARY_AND:
    case ASTType::BINARY_OR:
    case ASTType::BINARY_XOR:
      if (first.type != SymbolType::BOOL) assert(false && "Binary bitwise on non bool types");
      return first;

    default:
      assert(false && "Binary");
  }

  // TODO: use some binary op table instead of this
}

Symbol expr(Expr* node) {
  DEBUG_ENTRY();
  switch (node->type) {
    case ASTType::EXPRESSION_CALL:
      return call(node->call);

    case ASTType::EXPRESSION_UNARY:
      return unary(node->unary);

    case ASTType::EXPRESSION_BINARY:
      return binary(node->binary);

    case ASTType::EXPRESSION_IDENTIFIER:
      return *identifierResolve(node->identifier);

    case ASTType::EXPRESSION_LITERAL:
      return literal(node->literal);

    default:
      assert(false && "Expr");
  }
}

Symbol* functionParam(FunctionParam* node) {
  DEBUG_ENTRY();
  Name id = identifierDecl(node->identifier);
  Symbol type_sym = type(node->decl_type);
  Symbol* symbol = createSymbol(id, type_sym);

  return symbol;
}

void function(Function* node) {
  DEBUG_ENTRY();
  Name id = identifierDecl(node->header->identifier);
  Symbol return_type = type(node->header->return_type);
  Symbol* symbol = symbolCreateFunction(node, id.start, id.end, current_scope, return_type);

  current_scope = symbol->function.scope;

  for (int i = 0; i < node->header->parameter_count; i++) {
    Symbol* param_sym = functionParam(node->header->parameter_list[i]);
    symbolAddFunctionParamChild(symbol, param_sym->start, param_sym->end, param_sym);
  }

  if (node->expr != nullptr) {
    // TODO: does the type of this match return type?
    expr(node->expr);
  }

  if (node->block != nullptr) {
    block(node->block);
  }

  current_scope = current_scope->parent;
  scopeDeclare(current_scope, symbol);
}

void struct_(Struct* node) {
  DEBUG_ENTRY();
  Name id = identifierDecl(node->identifier);
  Symbol* symbol = symbolCreateStruct(node, id.start, id.end, current_scope);

  current_scope = symbol->struct_.members_table;

  for (int i = 0; i < node->declarations_count; i++) {
    Symbol* decl_sym = declaration(node->declarations[i]);
    symbolAddStructChild(symbol, decl_sym->start, decl_sym->end, decl_sym);

  }

  current_scope = symbol->struct_.members_table->parent;
  scopeDeclare(current_scope, symbol);
}

void enum_(Enum* node) {
  DEBUG_ENTRY();
  Name id = identifierDecl(node->identifier);
  Symbol* symbol = symbolCreateEnum(id.start, id.end);

  for (int i = 0; i < node->members_count; i++) {
    symbolAddEnumChild(symbol, node->members[i]->start, node->members[i]->end);
  }

  scopeDeclare(current_scope, symbol);
}

void primaryTag(PrimaryTag* node) {
  DEBUG_ENTRY();
  switch (node->type) {
    case ASTType::PRIMARY_TAG_DECL:
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
  DEBUG_ENTRY();
  for (int i = 0; i < node->primary_tags_count; i++) {
    primaryTag(node->primary_tags[i]);
  }
}

}

void visitDefRef(Primary* node) {
  scopeStackCreate();
  symbolStackCreate();

  current_scope = scopeCreate(nullptr, node);
  
  defref::primary(node);
}

void defref_destroy() {
  symbolStackDestroy();
  scopeStackDestroy();
}
