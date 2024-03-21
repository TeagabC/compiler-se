#pragma once 

#include <cstdint>
#include "lex.h"

extern const char * ASTTypes[];
void printASTNode(void* node);

enum class ASTType;
extern const int binary_operator_precidence[];
int getBinaryPrecidence(ASTType type);

enum class ASTType {
  NONE,
  TYPE_SIMPLE,
  TYPE_ID,
  SIMPLE_TYPE_I8,
  SIMPLE_TYPE_U8,
  SIMPLE_TYPE_I32,
  SIMPLE_TYPE_U32,
  SIMPLE_TYPE_F32,
  IDENTIFIER,
  QUALIFIER_CONST,
  QUALIFIER_MUT,
  QUALIFIER_EXPORT,
  DECLARATION,
  ASSIGNMENT,
  CONDITIONAL,
  WHILE,
  BREAK,
  CONTINUE,
  RETURN,
  STATEMENT_CONDITION,
  STATEMENT_WHILE,
  STATEMENT_BREAK,
  STATEMENT_CONTINUE,
  STATEMENT_RETURN,
  STATEMENT_ASSIGN,
  STATEMENT_EXPR,
  BLOCK_TAG_STATEMENT,
  BLOCK_TAG_BLOCK,
  BLOCK,
  LITERAL_STRING,
  LITERAL_INT,
  LITERAL_FLOAT,
  LITERAL_BOOL,
  CALL,
  UNARY_PLUS,
  UNARY_MINUS,
  UNARY_NOT,
  BINARY_ADD,
  BINARY_SUB,
  BINARY_MUL,
  BINARY_DIV,
  BINARY_MOD,
  BINARY_LT,
  BINARY_GT,
  BINARY_LE,
  BINARY_GE,
  BINARY_EQ,
  BINARY_NE,
  BINARY_AND,
  BINARY_OR,
  BINARY_XOR,
  EXPRESSION_CALL,
  EXPRESSION_UNARY,
  EXPRESSION_BINARY,
  EXPRESSION_IDENTIFIER,
  EXPRESSION_LITERAL,
  FUNC_PARAM,
  FUNC_HEADER,
  FUNC_FORWARD,
  FUNCTION,
  STRUCT,
  ENUM,
  PRIMARY_TAG_DECL,
  PRIMARY_TAG_STRUCT,
  PRIMARY_TAG_ENUM,
  PRIMARY_TAG_FUNC,
  PRIMARY,
};


struct ASTNode {
  ASTType type;
  Token* start;
  Token* end;
};

struct SimpleType {
  ASTType type;
  Token* start;
  Token* end;
  union {
    int8_t int8;
    uint8_t uint8;
    int32_t int32;
    uint32_t uint32;
    float f32;
  };
};

struct Identifier {
  ASTType type;
  Token* start;
  Token* end;
  Token* identifier;
  Identifier* next;
};

struct Type {
  ASTType type;
  Token* start;
  Token* end;
  union {
    SimpleType *simple_type;
    Identifier *identifier;
  };
};

struct Qualifier {
  ASTType type;
  Token* start;
  Token* end;
};
// TODO: maybe QualifierList ?

struct Expr;
struct Block;

struct Declaration {
  ASTType type;
  Token* start;
  Token* end;
  Qualifier** qualifiers;
  int qualifiers_count;
  Identifier* identifier;
  Type* decl_type;
  Expr* expr;
};

struct Assignment {
  ASTType type;
  Token* start;
  Token* end;
  Identifier* identifier;
  Expr* expr;
};

struct Conditional {
  ASTType type;
  Token* start;
  Token* end;
  Expr* condition;
  Block* block;
  Block* other;
};

struct While {
  ASTType type;
  Token* start;
  Token* end;
  Expr* condition;
  Block* block;
};

struct Break {
  ASTType type;
  Token* start;
  Token* end;
};

struct Continue {
  ASTType type;
  Token* start;
  Token* end;
};

struct Return {
  ASTType type;
  Token* start;
  Token* end;
  Expr* expr;
};

struct ExprStatement {
  ASTType type;
  Token* start;
  Token* end;
  Expr* expr;
};

struct Statement {
  ASTType type;
  Token* start;
  Token* end;
  union {
    Conditional* conditional;
    While* while_;
    Break* break_;
    Continue* continue_;
    Return* return_;
    Assignment* assignment;
    Expr* expr;
  };
};

struct BlockTag {
  ASTType type;
  Token* start;
  Token* end;
  union {
    Statement* statement;
    Block* block;
  };
};

struct Block {
  ASTType type;
  Token* start;
  Token* end;
  Identifier* namespace_;
  Statement* statement;
  Declaration** declarations;
  int declarations_count;
  BlockTag** block_tags;
  int block_tags_count;
};

struct Literal {
  ASTType type;
  Token* start;
  Token* end;
  union {
    char* string_;
    int int_;
    float float_;
    bool bool_;
  };
};

struct Call {
  ASTType type;
  Token* start;
  Token* end;
  Identifier* identifier;
  Expr** arguments;
  int arguments_count;
};


struct Unary {
  ASTType type;
  Token* start;
  Token* end;
  Expr* expr;
};

struct Binary {
  ASTType type;
  Token* start;
  Token* end;
  Expr* first;
  Expr* second;
};

struct Expr {
  ASTType type;
  Token* start;
  Token* end;
  union {
    Call* call;
    Unary* unary;
    Binary* binary;
    Identifier* identifier;
    Literal* literal;
  };
};

struct FunctionParam {
  ASTType type;
  Token* start;
  Token* end;
  Identifier* identifier;
  Type* decl_type;
};

struct FunctionHeader {
  ASTType type;
  Token* start;
  Token* end;
  Identifier* identifier;
  Type* return_type;
  FunctionParam** parameter_list;
  int parameter_count;
  bool export_;
};

struct Function {
  ASTType type;
  Token* start;
  Token* end;
  FunctionHeader* header;
  Expr* expr;
  Block* block;
};

struct Struct {
  ASTType type;
  Token* start;
  Token* end;
  Identifier* identifier;
  Declaration** declarations;
  int declarations_count;
};

struct Enum {
  ASTType type;
  Token* start;
  Token* end;
  Identifier* identifier;
  Token** members;
  int members_count;
};

struct PrimaryTag {
  ASTType type;
  Token* start;
  Token* end;
  union {
    Declaration* decl;
    Struct* struct_;
    Enum* enum_;
    Function* func;
  };
};

struct Primary {
  ASTType type;
  Token* start;
  Token* end;
  PrimaryTag** primary_tags;
  int primary_tags_count;
};

