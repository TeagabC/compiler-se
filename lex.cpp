#include "lex.h"
#include "ht.h"

#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <stdio.h>
#include <type_traits>


const char * TokenTypes[] = {
  "NONE",
  "END",

  "LEFT_PAREN",
  "RIGHT_PAREN",
  "LEFT_BRACE",
  "RIGHT_BRACE",
  "LEFT_SQUARE",
  "RIGHT_SQUARE",
  "COLON",
  "SEMI",
  "COMMA",
  "DOT",

  "ADD",
  "SUB",
  "MUL",
  "DIV",
  "MOD",


  "SET",
  "EQ",
  "NOT",
  "NE",
  "LT",
  "LE",
  "GT",
  "GE",

  "STRING_LITERAL",
  "INT_LITERAL",
  "FLOAT_LITERAL",
  "IDENTIFIER",
  
  "I8",
  "U8",
  "I32",
  "U32",
  "F32",

  "AND",
  "BREAK",
  "CONST",
  "CONTINUE",
  "ELSE",
  "ENUM",
  "EXPORT",
  "FALSE",
  "FUNC",
  "IF",
  "MUT",
  "OR",
  "RETURN",
  "STRUCT",
  "TRUE",
  "WHILE",
  "XOR",
};


#include <cassert>
void printToken(const Token& token) {
  printf("%s: \"", TokenTypes[static_cast<std::underlying_type<TokenType>::type>(token.type)]);
  if (token.type == TokenType::END) {
    printf("\"\n");
    return;
  }

  assert(token.type != TokenType::NONE);
  assert(token.start != nullptr);
  assert(token.end != nullptr);
  assert(token.start < token.end);

  char* current = token.start;
  while (current != token.end) {
    printf("%c", *current++);
  }
  printf("\"\n");
}

struct KeywordPair {
  const char* keyword;
  TokenType type;
};

HashTable* keyword_table = nullptr;
KeywordPair keywords[] = {
  {"i8", TokenType::I8},
  {"u8", TokenType::U8},
  {"i32", TokenType::I32},
  {"u32", TokenType::U32},
  {"f32", TokenType::F32},
  {"and", TokenType::AND},
  {"break", TokenType::BREAK},
  {"const", TokenType::CONST},
  {"continue", TokenType::CONTINUE},
  {"else", TokenType::ELSE},
  {"enum", TokenType::ENUM },
  {"export", TokenType::EXPORT },
  {"false", TokenType::FALSE },
  {"func", TokenType::FUNC },
  {"if", TokenType::IF },
  {"mut", TokenType::MUT},
  {"or", TokenType::OR},
  {"return", TokenType::RETURN},
  {"struct", TokenType::STRUCT},
  {"true", TokenType::TRUE},
  {"while", TokenType::WHILE},
  {"xor", TokenType::XOR},
};

HashTable* createKeywordsTable() {
  assert(keyword_table == nullptr);
  keyword_table = htCreate(64);

  int size = sizeof(keywords) / sizeof(KeywordPair);
  for (int i = 0; i < size; i++) {
    const char* start = keywords[i].keyword;
    const char* end = start + strlen(start);
    htSet(keyword_table, start, end, &keywords[i].type);
  }
  return keyword_table;
}

void destroyKeywordsTable() {
  assert(keyword_table != nullptr);

  htDestroy(keyword_table);
  keyword_table = nullptr;
}

// end is a sentinel past string end
Token nextKeyword(char* start, char* end) {
  // check hashmap for word
  TokenType* p = (TokenType*) htGet(keyword_table, start, end);
  if (p != nullptr) {
    return {*p, start, end};
  }
  /*
  std::string_view view = {start, (size_t)(end - start)};
  if (keyword_map.contains(view)) {
    return {keyword_map[view], start, end};
  }
  */
  return {TokenType::NONE, start};
}

Token nextAlnum(char*& input) {
  if (*input == '\0') return {TokenType::END, NULL, NULL};
  if (!isalnum(*input)) return {TokenType::NONE, NULL, NULL};

  char* start = input;
  if (isdigit(*input)) {
    while (isdigit(*++input)) {}
    if (!(*input == '.' && isdigit(*(input + 1)))) 
      return {TokenType::INT_LITERAL, start, input}; 

    input++;
    while (isdigit(*++input)) {}
    return {TokenType::FLOAT_LITERAL, start, input};

  }
  else {
    while (isalnum(*++input)){}

    Token token = nextKeyword(start, input);
    if (token.type != TokenType::NONE) return token; 

    return {TokenType::IDENTIFIER, start, input};
  }
}

// Moves input to the last character of token
Token nextToken(char*& input) {
  while (*input == ' ' || *input == '\n' || *input == '\t') {
    if (*input == '\0') return {TokenType::END, NULL};
    input++;
  }

  char* start = input;
  switch(*input) {
    case '(': return {TokenType::LEFT_PAREN, input, ++input};
    case ')': return {TokenType::RIGHT_PAREN, input, ++input};
    case '{': return {TokenType::LEFT_BRACE, input, ++input};
    case '}': return {TokenType::RIGHT_BRACE, input, ++input};
    case '[': return {TokenType::LEFT_SQUARE, input, ++input};
    case ']': return {TokenType::RIGHT_SQUARE, input, ++input};
    case ':': return {TokenType::COLON, input, ++input};
    case ';': return {TokenType::SEMI, input, ++input};
    case ',': return {TokenType::COMMA, input, ++input};
    case '.': return {TokenType::DOT, input, ++input};

    case '+': return {TokenType::ADD, input, ++input}; 
    case '-': return {TokenType::SUB, input, ++input};
    case '*': return {TokenType::MUL, input, ++input};
    case '/': return {TokenType::DIV, input, ++input};
    case '%': return {TokenType::MOD, input, ++input};

    case '=': 
      if (*++input != '=') return {TokenType::SET, start, input};
      else return {TokenType::EQ, start, ++input};
    case '!':
      input++;
      if (*++input != '=') return {TokenType::NOT, start, input};
      else return {TokenType::NE, start, ++input};
    case '<':
      if (*++input != '=') return {TokenType::LT, start, input};
      else return {TokenType::LE, start, ++input};
    case '>':
      if (*++input != '=') return {TokenType::GT, start, input};
      else return {TokenType::GE, start, ++input};


    case '"': 
      while (*++input != '\0' && *input != '"'){}
      return *input == '\0' ?
        Token {TokenType::NONE, NULL, NULL} :
        Token {TokenType::STRING_LITERAL, start, ++input};

    default:
      return nextAlnum(input);
  }
}

Token* lex(char* input, int max_token_count) {
  Token* tokens = (Token*) malloc(max_token_count * sizeof(Token));
  createKeywordsTable();

  int i = 0;
  Token token = nextToken(input);
  while (token.type != TokenType::NONE && token.type != TokenType::END) {
    tokens[i] = token;
    token = nextToken(input);
    i++;
  }

  if (token.type == TokenType::NONE) {
    destroyKeywordsTable();
    free(tokens);
    return nullptr;
  }

  tokens[i] = token;
  destroyKeywordsTable();
  return tokens;
}

void lex_destroy(Token* tokens) {
  free(tokens);
}
