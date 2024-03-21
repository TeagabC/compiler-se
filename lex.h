#pragma once

extern const char * TokenTypes[];

enum class TokenType {
  NONE,
  END,

  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  LEFT_SQUARE,
  RIGHT_SQUARE,
  COLON,
  SEMI,
  COMMA,
  DOT,

  ADD,
  SUB,
  MUL,
  DIV,
  MOD,


  SET,
  EQ,
  NOT,
  NE,
  LT,
  LE,
  GT,
  GE,

  STRING_LITERAL,
  INT_LITERAL,
  FLOAT_LITERAL,
  IDENTIFIER,

  I8,
  U8,
  I32,
  U32,
  F32,


  AND,
  BREAK,
  CONST,
  CONTINUE,
  ELSE,
  ENUM,
  EXPORT,
  FALSE,
  FUNC,
  IF,
  MUT,
  OR,
  RETURN,
  STRUCT,
  TRUE,
  WHILE,
  XOR,
  
};

struct Token {
  TokenType type;
  char* start;
  char* end;
};

void printToken(const Token& token);

Token nextToken(char*& input);

Token* lex(char* input, int max_token_count);
void lex_destroy(Token* tokens);
