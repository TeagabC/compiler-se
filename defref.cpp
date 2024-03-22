#include "defref.h"
#include "ast_types.h"
#include "scope.h"
#include "symbol.h"

#include <cassert>
#include <ios>

Scope* current_scope;

struct Name {
  const char* start;
  const char* end;
};


SymbolType simpleType(SimpleType* node) {
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

bool matchTypes(Symbol* symbol, Symbol expr_sym) {
  if (symbol->type != expr_sym.type) {
    // TODO: check implicit casting rules
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

bool matchBool(Symbol expr_sym) {
  if (expr_sym.type != SymbolType::BOOL) {
    // TODO: check if type is castable to bool
    assert(false && "Bool expr isn't bool");
  }
  return true;
}

Symbol expr(Expr* node);
void block(Block* node);

void declaration(Declaration* node) {
  Name id = identifierDecl(node->identifier);
  Symbol type_sym = type(node->decl_type);
  Symbol* symbol = createSymbol(id, type_sym);
  Symbol expr_sym;

  // TODO: implement
  for (int i = 0; i < node->qualifiers_count; i++) {
    qualifier(node->qualifiers[i]);
  }

  // TODO: implement
  if (node->expr != nullptr) {
    expr_sym = expr(node->expr);
  }

  if (!matchTypes(symbol, expr_sym)) {
    // this never actually hits
    assert(false && "Declaration expr assignment type doesn't match");
  }

  scopeDeclare(current_scope, symbol);
}

void assignment(Assignment* node) {
  Symbol* symbol = identifierResolve(node->identifier);
  Symbol expr_sym = expr(node->expr);

  if (!matchTypes(symbol, expr_sym)) {
    // this never actually hits
    assert(false && "Assignment types don't match");
  }
}

void conditional(Conditional* node) {
  Symbol expr_sym = expr(node->condition);

  if (!matchBool(expr_sym)) {
    // this never actually hits
    assert(false && "Conditional condition is not bool");
  }

  block(node->block);

  if (node->other != nullptr) {
    block(node->other);
  }
}

void while_(While* node) {
  Symbol expr_sym = expr(node->condition);
  
  if (!matchBool(expr_sym)) {
    // this never actually hits
    assert(false && "While condition is not bool");
  }

  block(node->block);
}

void break_(Break* node) {
}

void continue_(Continue* node) {
}

void return_(Return* node) {
  if (node->expr != nullptr) {
    expr(node->expr);
  }
  else {

  }
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

void block(Block* node) {
  if (node->namespace_ != nullptr) {
    // TODO: 
    identifierDecl(node->namespace_);
  }

  current_scope = scopeCreate(current_scope);

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
  Symbol* func_sym = identifierResolve(node->identifier);

  if (func_sym->type != SymbolType::FUNCTION) {
    assert(false && "Calling a symbol which is not a function");
  }
  
  for (int i = 0; i < node->arguments_count; i++) {
    Symbol expr_sym = expr(node->arguments[i]);
    if (func_sym->function.parameter_vector->size <= i ||
      !matchTypes((Symbol*)vecGet(func_sym->function.parameter_vector, i), expr_sym)) {
      assert(false && "Function call parameter types don't match definition");
    }
  }

  return *func_sym;
}

Symbol unary(Unary* node) {
  Symbol symbol = expr(node->expr);
  switch (node->type) {
    case ASTType::UNARY_NOT:
      break;
    case ASTType::UNARY_PLUS:
      break;
    case ASTType::UNARY_MINUS:
      break;
    default:
      assert(false && "Unary");
  }
}

Symbol binary(Binary* node) {
  expr(node->first);
  expr(node->second);
}

Symbol expr(Expr* node) {
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

void functionParam(FunctionParam* node) {
  Name id = identifierDecl(node->identifier);
  Symbol type_sym = type(node->decl_type);
  Symbol* symbol = createSymbol(id, type_sym);

  scopeDeclare(current_scope, symbol);
}

void function(Function* node) {
  Name id = identifierDecl(node->header->identifier);
  Symbol return_type = type(node->header->return_type);
  Symbol* symbol = symbolCreateFunction(id.start, id.end, current_scope, return_type);

  current_scope = symbol->function.scope;

  for (int i = 0; i < node->header->parameter_count; i++) {
    functionParam(node->header->parameter_list[i]);
  }

  if (node->expr != nullptr) {
    expr(node->expr);
  }

  if (node->block != nullptr) {
    block(node->block);
  }

  current_scope = current_scope->parent;
}

void struct_(Struct* node) {
  Name id = identifierDecl(node->identifier);
  Symbol* symbol = symbolCreateStruct(id.start, id.end, current_scope);

  current_scope = symbol->struct_.members_table;

  for (int i = 0; i < node->declarations_count; i++) {
    Name decl_id = identifierDecl(node->declarations[i]->identifier);
    symbolAddStructChild(symbol, decl_id.start, decl_id.end);

    declaration(node->declarations[i]);
  }

  current_scope = symbol->struct_.members_table->parent;
}

void enum_(Enum* node) {
  Name id = identifierDecl(node->identifier);
  Symbol* symbol = symbolCreateEnum(id.start, id.end);

  for (int i = 0; i < node->members_count; i++) {
    symbolAddEnumChild(symbol, node->members[i]->start, node->members[i]->end);
  }

  scopeDeclare(current_scope, symbol);
}

void primaryTag(PrimaryTag* node) {
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
  for (int i = 0; i < node->primary_tags_count; i++) {
    primaryTag(node->primary_tags[i]);
  }
}


void visitDefRef(Primary* node) {
  scopeStackCreate();
  symbolStackCreate();

  current_scope = scopeCreate(nullptr);
  
  primary(node);
}

void defref_destroy() {
  symbolStackDestroy();
  scopeStackDestroy();
}
