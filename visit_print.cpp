#include "parser_types.h"
#include <cassert>
#include <cstdio>


int tabs = 0;
#define printTab(...) for (int i = 0; i < tabs; i++) printf("  "); printf(__VA_ARGS__)

#define printPtrTab(start, end) for (int i = 0; i < tabs; i++) printf("  "); while(start != end) printf("%c", *start++); 

#define printPtr(start, end) while(start != end) printf("%c", *start++); 

void simpleType(SimpleType* node) {
  switch (node->type) {
    case ASTType::SIMPLE_TYPE_I8:
      printTab("I8");
      break;
    case ASTType::SIMPLE_TYPE_U8:
      printTab("U8");
      break;
    case ASTType::SIMPLE_TYPE_I32:
      printTab("I32");
      break;
    case ASTType::SIMPLE_TYPE_U32:
      printTab("U32");
      break;
    case ASTType::SIMPLE_TYPE_F32:
      printTab("F32");
      break;
    default:
      assert(false && "Simple Type");
  }
}
    
void identifier(Identifier* node) {
  printTab("Identifier: ");
  tabs++;
  printPtr(node->identifier->start, node->identifier->end);
  if (node->next != nullptr) {
    printf("\n");
    printTab("DOT:\n");
    identifier(node->next);
  }
  tabs--;
}

void type(Type* node) {
  printTab("Type:\n");
  tabs++;
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
  printf("\n");
  tabs--;
}

void qualifier(Qualifier* node) {
  switch (node->type) {
    case ASTType::QUALIFIER_CONST:
      printTab("Qualifier: const");
      break;
    case ASTType::QUALIFIER_EXPORT:
      printTab("Qualifier: export");
      break;
    case ASTType::QUALIFIER_MUT:
      printTab("Qualifier: mut");
      break;
    default:
      assert(false && "Qualifier");
  }
}

void expr(Expr* node);
void block(Block* node);

void declaration(Declaration* node) {
  printTab("Declaration:\n");
  tabs++;
  
  identifier(node->identifier);
  
  type(node->decl_type);

  for (int i = 0; i < node->qualifiers_count; i++) {
    qualifier(node->qualifiers[i]);
    printf("\n");
  }


  if (node->expr != nullptr) {
    printTab( "Set Expr:\n");
    expr(node->expr);
  }

  tabs--;

}

void assignment(Assignment* node) {
  printTab("Assignment:\n");
  tabs++;

  identifier(node->identifier);
  printf("\n");
  expr(node->expr);
  printf("\n");

  tabs--;
}

void conditional(Conditional* node) {
  printTab("Conditional:\n");
  tabs++;

  expr(node->condition);
  block(node->block);

  if (node->other != nullptr) {
    printTab("Else:\n");
    block(node->other);
  }
  tabs--;
}

void while_(While* node) {
  printTab("While:\n");
  tabs++;

  expr(node->condition);
  block(node->block);

  tabs--;
}

void break_(Break* node) {
  printTab("Break\n");
}

void continue_(Continue* node) {
  printTab("Continue\n");
}

void return_(Return* node) {
  printTab("Return\n");

  if (node->expr != nullptr) {
    tabs++;
    expr(node->expr);
    tabs--;
  }
}

void statement(Statement* node) {
  printTab("Statement:\n");
  tabs++;
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
  tabs--;
}

void blockTag(BlockTag* node) {
  printTab("Block Tag:\n");
  tabs++;

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

  tabs--;
}

void block(Block* node) {
  printTab("Block:\n");
  tabs++;

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
  tabs--;
}

void literal(Literal* node) {
  // TODO: implement
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
  printTab("Call:\n");
  tabs++;
  
  identifier(node->identifier);
  
  for (int i = 0; i < node->arguments_count; i++) {
    expr(node->arguments[i]);
  }

  tabs--;
}

void unary(Unary* node) {
  printTab("Unary:\n");
  tabs++;
  
  switch (node->type) {
    case ASTType::UNARY_NOT:
      printTab("Operator : NOT\n");
      break;
    case ASTType::UNARY_PLUS:
      printTab("Operator : PLUS\n");
      break;
    case ASTType::UNARY_MINUS:
      printTab("Operator : MINUS\n");
      break;
    default:
      assert(false && "Unary");
  }

  tabs--;
}

void binary(Binary* node) {
  printTab("Binary:\n");
  tabs++;

  printTab("Operator: %s\n", ASTTypes[(int)node->type]);

  expr(node->first);
  expr(node->second);

  tabs--;
}

void expr(Expr* node) {
  printTab("Expr:\n");
  tabs++;

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
  printf("\n");

  tabs--;
}

void functionParam(FunctionParam* node) {
  printTab("Param:\n");
  tabs++;
  identifier(node->identifier);
  printf("\n");
  type(node->decl_type);
  tabs--;
}

void functionHeader(FunctionHeader* node) {
  printTab("Function Header:\n");
  tabs++;
  identifier(node->identifier);
  printf("\n");

  printTab("Export: %i\n", node->export_);

  printTab("Return Type:\n");
  type(node->return_type);

  for (int i = 0; i < node->parameter_count; i++) {
    functionParam(node->parameter_list[i]);
  }
  tabs--;
}

void function(Function* node) {
  printTab("Function:\n");
  tabs++;

  functionHeader(node->header);

  if (node->expr != nullptr) {
    expr(node->expr);
  }

  if (node->block != nullptr) {
    block(node->block);
  }

  tabs--;
}

void struct_(Struct* node) {
  printTab("Struct:\n");
  tabs++;

  identifier(node->identifier);

  for (int i = 0; i < node->declarations_count; i++) {
    declaration(node->declarations[i]);
  }

  printf("\n");

  tabs--;
}

void enum_(Enum* node) {
  printTab("Enum:\n");
  tabs++;

  identifier(node->identifier);
  
  for (int i = 0; i < node->members_count; i++) {
    printPtrTab(node->members[i]->start, node->members[i]->end);
    printf("\n");
  }
  printf("\n");
  tabs--;
}

void primaryTag(PrimaryTag* node) {
  printTab("Primary Tag:\n");
  tabs++;
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
  tabs--;
}

void primary(Primary* node) {
  printTab("Primary:\n");
  tabs++;
  for (int i = 0; i < node->primary_tags_count; i++) {
    primaryTag(node->primary_tags[i]);
  }
  tabs--;
}


void visitPrint(Primary* node) {
  primary(node);
}

