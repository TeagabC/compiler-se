#include "codegen.h"
#include "ast_types.h"
#include "scope.h"
#include "symbol.h"

#include <cassert>

namespace codegen {

void simpleType(SimpleType* node) {
  switch (node->type) {
    case ASTType::SIMPLE_TYPE_I8:
      break;
    case ASTType::SIMPLE_TYPE_U8:
      break;
    case ASTType::SIMPLE_TYPE_I32:
      break;
    case ASTType::SIMPLE_TYPE_U32:
      break;
    case ASTType::SIMPLE_TYPE_F32:
      break;
    default:
      assert(false && "Simple Type");
  }
}
    
void identifier(Identifier* node) {
  if (node->next != nullptr) {
    identifier(node->next);
  }
}

void type(Type* node) {
  switch (node->type) {
    case ASTType::TYPE_SIMPLE: 
      simpleType(node->simple_type);
      break;
    case ASTType::TYPE_ID:
      identifier(node->identifier);
      break;
    default:
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

void expr(Expr* node);
void block(Block* node);

void declaration(Declaration* node) {
  identifier(node->identifier);
  type(node->decl_type);

  for (int i = 0; i < node->qualifiers_count; i++) {
    qualifier(node->qualifiers[i]);
  }

  if (node->expr != nullptr) {
    expr(node->expr);
  }
}

void assignment(Assignment* node) {
  identifier(node->identifier);
  expr(node->expr);
}

void conditional(Conditional* node) {
  expr(node->condition);
  block(node->block);

  if (node->other != nullptr) {
    block(node->other);
  }
}

void while_(While* node) {
  expr(node->condition);
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
    identifier(node->namespace_);
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
}

void literal(Literal* node) {
  switch (node->type) {
    case ASTType::LITERAL_STRING:
      break;
    case ASTType::LITERAL_INT:
      break;
    case ASTType::LITERAL_FLOAT:
      break;
    case ASTType::LITERAL_BOOL:
      break;
    default:
      assert(false && "Literal");
  }
}

void call(Call* node) {
  identifier(node->identifier);
  
  for (int i = 0; i < node->arguments_count; i++) {
    expr(node->arguments[i]);
  }
}

void unary(Unary* node) {
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

  expr(node->expr);
}

void binary(Binary* node) {
  expr(node->first);
  expr(node->second);
}

void expr(Expr* node) {
  switch (node->type) {
    case ASTType::EXPRESSION_CALL:
      call(node->call);
      break;
    case ASTType::EXPRESSION_UNARY:
      unary(node->unary);
      break;
    case ASTType::EXPRESSION_BINARY:
      binary(node->binary);
      break;
    case ASTType::EXPRESSION_IDENTIFIER:
      identifier(node->identifier);
      break;
    case ASTType::EXPRESSION_LITERAL:
      literal(node->literal);
      break;
    default:
      assert(false && "Expr");
  }
}

void functionParam(FunctionParam* node) {
  identifier(node->identifier);
  type(node->decl_type);
}

void functionHeader(FunctionHeader* node) {
  identifier(node->identifier);
  type(node->return_type);

  for (int i = 0; i < node->parameter_count; i++) {
    functionParam(node->parameter_list[i]);
  }
}

void function(Function* node) {
  functionHeader(node->header);

  if (node->expr != nullptr) {
    expr(node->expr);
  }

  if (node->block != nullptr) {
    block(node->block);
  }
}

void struct_(Struct* node) {
  identifier(node->identifier);

  for (int i = 0; i < node->declarations_count; i++) {
    declaration(node->declarations[i]);
  }
}

void enum_(Enum* node) {
  identifier(node->identifier);
  
  for (int i = 0; i < node->members_count; i++) {
  }
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

};

void visitCodeGen(Primary* node) {
  codegen::primary(node);
}

