#include "parser_types.h"
#include "lex.h"
#include <cassert>
#include <stdio.h>
#include <type_traits>

const char * ASTTypes[] = {
  "NONE",
  "TYPE_SIMPLE",
  "TYPE_ID",
  "SIMPLE_TYPE_I8",
  "SIMPLE_TYPE_U8",
  "SIMPLE_TYPE_I32",
  "SIMPLE_TYPE_U32",
  "SIMPLE_TYPE_F32",
  "IDENTIFIER",
  "QUALIFIER_CONST",
  "QUALIFIER_MUT",
  "QUALIFIER_EXPORT",
  "DECLARATION",
  "ASSIGNMENT",
  "CONDITIONAL",
  "WHILE",
  "BREAK",
  "CONTINUE",
  "RETURN",
  "STATEMENT_CONDITION",
  "STATEMENT_WHILE",
  "STATEMENT_BREAK",
  "STATEMENT_CONTINUE",
  "STATEMENT_RETURN",
  "STATEMENT_ASSIGN",
  "STATEMENT_EXPR",
  "BLOCK_TAG_STATEMENT",
  "BLOCK_TAG_BLOCK",
  "BLOCK",
  "LITERAL_STRING",
  "LITERAL_INT",
  "LITERAL_FLOAT",
  "LITERAL_BOOL",
  "CALL",
  "UNARY_PLUS",
  "UNARY_MINUS",
  "UNARY_NOT",
  "BINARY_ADD",
  "BINARY_SUB",
  "BINARY_MUL",
  "BINARY_DIV",
  "BINARY_MOD",
  "BINARY_LT",
  "BINARY_GT",
  "BINARY_LE",
  "BINARY_GE",
  "BINARY_EQ",
  "BINARY_NE",
  "BINARY_AND",
  "BINARY_OR",
  "BINARY_XOR",
  "EXPRESSION_CALL",
  "EXPRESSION_UNARY",
  "EXPRESSION_BINARY",
  "EXPRESSION_IDENTIFIER",
  "EXPRESSION_LITERAL",
  "FUNC_PARAM",
  "FUNC_HEADER",
  "FUNC_FORWARD",
  "FUNCTION",
  "STRUCT",
  "ENUM",
  "PRIMARY_TAG_DECL",
  "PRIMARY_TAG_STRUCT",
  "PRIMARY_TAG_ENUM",
  "PRIMARY_TAG_FUNC",
  "PRIMARY",
};

const int binary_operator_precidence[] = {
  70, // BINARY_ADD,
  70, // BINARY_SUB,
  60, // BINARY_MUL,
  60, // BINARY_DIV,
  60, // BINARY_MOD,
  50, // BINARY_LT,
  50, // BINARY_GT,
  50, // BINARY_LE,
  50, // BINARY_GE,
  40, // BINARY_EQ,
  40, // BINARY_NE,
  30, // BINARY_AND,
  20, // BINARY_OR,
  20, // BINARY_XOR,
};

int getBinaryPrecidence(ASTType type) {
  return binary_operator_precidence [
    static_cast<std::underlying_type<ASTType>::type>(type) - 
    static_cast<std::underlying_type<ASTType>::type>(ASTType::BINARY_ADD)];
}

void printASTNode(void* node) {
  Break* n = (Break*) node;
  assert(n != nullptr);
  assert(n->type != ASTType::NONE);
  assert(n->start != nullptr);
  assert(n->end != nullptr);
  assert(n->start < n->end);

  Token* current = n->start;
  printf("%s: \n", ASTTypes[static_cast<std::underlying_type<ASTType>::type>(n->type)]);
  while (current != n->end) {
    printToken(*current);
    current++;
  }
}

