#include "parser.h"
#include "lex.h"
#include "parser_types.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <stdio.h>

#if true
#define DEBUG(s, t, t2) fprintf(stderr, "%s -> Line: %u : \n\t", s, __LINE__); printToken(*t); fprintf(stderr, "\t"); printToken(*t2);
#else
#define DEBUG(s, t, t2)
#endif

#if false
#define DEBUG_PRINT(s) fprintf(stderr, "Line: %u -> %s\n", __LINE__, s);
#else 
#define DEBUG_PRINT(s)
#endif

#define MAX_BUFFER_COUNT 64

bool check(Token* token, TokenType type) {
  if (token->type != type) return false;
  return true;
}

int stack_capacity = 65536;
uint8_t* stack_base = nullptr;
uint8_t* stack_current = nullptr;

void create_stack(int capacity = 65536) {
  assert(stack_base == nullptr);
  stack_capacity = capacity;
  stack_base = (uint8_t*) malloc(stack_capacity);
  stack_current = stack_base;
  assert(stack_base != nullptr);
}

void destroy_stack() {
  if (stack_base == nullptr) return;
  stack_capacity = 0;
  free(stack_base);
  stack_base = nullptr;
  stack_current = nullptr;
}


void* push_stack(int size) {
  assert(stack_current - stack_base + size < stack_capacity);
  void* ret = stack_current;
  stack_current += size;
  memset(ret, 0, size);
  //fprintf(stderr, "Push stack: %li\n", stack_current - stack_base);
  return ret;
}

void* pop_stack(int size) {
  assert(stack_current - size >= stack_base);
  stack_current -= size;
  //fprintf(stderr, "Pop stack: %li\n", stack_current - stack_base);
  return nullptr;
}

void* pop_stack(void* ptr) {
  assert(ptr < stack_current && ptr > stack_base);
  stack_current = (uint8_t*) ptr;
  //fprintf(stderr, "Pop stack: %li\n", stack_current - stack_base);
  return nullptr;
}

int buffer_pool_capacity = 65536;
uint8_t* buffer_pool_base = nullptr;
uint8_t* buffer_pool_current = nullptr;

void create_buffer_pool(int capacity = 65536) {
  assert(buffer_pool_base == nullptr);
  buffer_pool_capacity = capacity;
  buffer_pool_base = (uint8_t*) malloc(buffer_pool_capacity);
  buffer_pool_current = buffer_pool_base;
  assert(buffer_pool_base != nullptr);
}

void destroy_buffer_pool() {
  if (buffer_pool_base == nullptr) return;
  buffer_pool_capacity = 0;
  free(buffer_pool_base);
  buffer_pool_base = nullptr;
  buffer_pool_current = nullptr;
}


void* push_buffer_pool(int size) {
  assert(buffer_pool_current - buffer_pool_base + size < buffer_pool_capacity);
  void* ret = buffer_pool_current;
  buffer_pool_current += size;
  memset(ret, 0, size);
  //fprintf(stderr, "Push buffer_pool: %li\n", buffer_pool_current - buffer_pool_base);
  return ret;
}

void* pop_buffer_pool(int size) {
  assert(buffer_pool_current - size >= buffer_pool_base);
  buffer_pool_current -= size;
  //fprintf(stderr, "Pop buffer_pool: %li\n", buffer_pool_current - buffer_pool_base);
  return nullptr;
}

void* pop_buffer_pool(void* ptr) {
  assert(ptr < buffer_pool_current && ptr > buffer_pool_base);
  buffer_pool_current = (uint8_t*) ptr;
  //fprintf(stderr, "Pop buffer_pool: %li\n", buffer_pool_current - buffer_pool_base);
  return nullptr;
}

SimpleType* simpleType(Token*& tokens) {
  SimpleType* node = (SimpleType*) push_stack(sizeof(SimpleType));
  node->start = tokens;

  switch (tokens->type) {
    case TokenType::I8:
      node->type = ASTType::SIMPLE_TYPE_I8;
      break;
    case TokenType::U8:
      node->type = ASTType::SIMPLE_TYPE_U8;
      break;
    case TokenType::I32:
      node->type = ASTType::SIMPLE_TYPE_I32;
      break;
    case TokenType::U32:
      node->type = ASTType::SIMPLE_TYPE_U32;
      break;
    case TokenType::F32:
      node->type = ASTType::SIMPLE_TYPE_F32;
      break;
    default:
      return (SimpleType*) pop_stack(node);
  }

  node->end = ++tokens;
  DEBUG("Match Simple Type", node->start, node->end);
  return node;
}

Identifier* identifier(Token*& tokens) {
  Identifier* node = (Identifier*) push_stack(sizeof(Identifier));

  Token* current = tokens;
  node->start = tokens;

  if (!check(current++, TokenType::IDENTIFIER)) return (Identifier*) pop_stack(node);

  node->identifier = tokens;
  if (check(current, TokenType::DOT)) {
    node->next = identifier(++current);
    if (node->next == nullptr) current--; // TODO: idk if we fail here or return identifier 
  }

  tokens = current;
  node->type = ASTType::ENUM;
  node->end = tokens;
  DEBUG("Match Identifier", node->start, node->end);
  return node;
}

Type* type(Token*& tokens) {
  Type* node = (Type*) push_stack(sizeof(Type));
  node->start = tokens;
  
  node->simple_type = simpleType(tokens);
  if (node->simple_type != nullptr) {
    node->type = ASTType::TYPE_SIMPLE;
    node->end = tokens;
    return node;
  }

  node->identifier = identifier(tokens);
  if (node->identifier != nullptr) {
    node->type = ASTType::TYPE_ID;
    node->end = tokens;
    return node;
  }

  return (Type*) pop_stack(node);
}

Qualifier* qualifier(Token*& tokens) {
  Qualifier* node = (Qualifier*) push_stack(sizeof(Qualifier));
  node->start = tokens;

  switch(tokens->type) {
    case TokenType::CONST:
      node->type = ASTType::QUALIFIER_CONST;
      break;

    case TokenType::EXPORT:
      node->type = ASTType::QUALIFIER_EXPORT;
      break;

    case TokenType::MUT:
      node->type = ASTType::QUALIFIER_MUT;
      break;

    default:
      return (Qualifier*) pop_stack(node);
  }

  node->end = ++tokens;
  DEBUG("Match Qualifier", node->start, node->end);
  return node;
}

Expr* expr(Token*& tokens, bool check_binary = true);
Block* block(Token*& tokens);

Declaration* declaration(Token*& tokens) {
  Declaration* node = (Declaration*) push_stack(sizeof(Declaration));
  
  unsigned byte_count;
  Qualifier** qualifier_buffer = (Qualifier**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(Qualifier*));

  Token* current = tokens;

  node->start = tokens;

  DEBUG_PRINT("Try Decl");
  qualifier_buffer[node->qualifiers_count] = qualifier(current);
  while(qualifier_buffer[node->qualifiers_count] != nullptr) {
    qualifier_buffer[++node->qualifiers_count] = qualifier(current);
  }

  node->identifier = identifier(current);
  if (node->identifier == nullptr) return (Declaration*) pop_stack(node);

  if(!check(current++, TokenType::COLON)) return (Declaration*) pop_stack(node);

  node->decl_type = type(current);
  if (node->decl_type == nullptr) return (Declaration*) pop_stack(node);

  DEBUG_PRINT("Try Decl SET");

  if (check(current, TokenType::SET)) {
    DEBUG_PRINT("Start Decl SET");
    node->expr = expr(++current);
    DEBUG("Decl Expr", node->expr->start, node->expr->end);
    if (node->expr == nullptr) return (Declaration*) pop_stack(node);
    DEBUG_PRINT("End Decl SET");
  }

  DEBUG_PRINT("Try Decl SEMI");
  if(!check(current++, TokenType::SEMI)) return (Declaration*) pop_stack(node);
  DEBUG_PRINT("SUCESS Decl SEMI");

  byte_count = node->qualifiers_count * sizeof(Qualifier*);
  node->qualifiers = (Qualifier**) push_stack(byte_count); 
  memcpy(node->qualifiers, qualifier_buffer, byte_count);

  tokens = current;
  node->type = ASTType::DECLARATION;
  node->end = current;
  DEBUG("Match Declaration", node->start, node->end);
  return node;
}

Assignment* assignment(Token*& tokens) {
  Assignment* node = (Assignment*) push_stack(sizeof(Assignment));
  Token* current = tokens;
  node->start = tokens;

  node->identifier = identifier(current);
  if (node->identifier == nullptr) return (Assignment*) pop_stack(node);
  if (!check(current++, TokenType::SET)) return (Assignment*) pop_stack(node);

  node->expr = expr(current);
  if (node->expr == nullptr) return (Assignment*) pop_stack(node);

  if (!check(current++, TokenType::SEMI)) return (Assignment*) pop_stack(node);

  tokens = current;
  node->type = ASTType::ASSIGNMENT;
  node->end = current;
  DEBUG("Match Assignment", node->start, node->end);
  return node;
}

Conditional* conditional(Token*& tokens) {
  Conditional* node = (Conditional*) push_stack(sizeof(Conditional));
  Token* current = tokens;
  node->start = tokens;

  if (!check(current++, TokenType::IF)) return (Conditional*) pop_stack(node);

  node->condition = expr(current);
  if (node->condition == nullptr) return (Conditional*) pop_stack(node);

  node->block = block(current);
  if (node->block == nullptr) return (Conditional*) pop_stack(node);

  if (check(current++, TokenType::ELSE)) {
    node->other = block(current);
    if (node->other == nullptr) return (Conditional*) pop_stack(node);
  }

  tokens = current;
  node->type = ASTType::CONDITIONAL;
  node->end = current;
  DEBUG("Match Conditional", node->start, node->end);
  return node;
}

While* while_(Token*& tokens) {
  While* node = (While*) push_stack(sizeof(While));
  Token* current = tokens;
  node->start = tokens;

  if (!check(current++, TokenType::WHILE)) return (While*) pop_stack(node);

  node->condition = expr(current);
  if (node->condition == nullptr) return (While*) pop_stack(node);

  node->block = block(current);
  if (node->block == nullptr) return (While*) pop_stack(node);

  tokens = current;
  node->type = ASTType::WHILE;
  node->end = current;
  DEBUG("Match While", node->start, node->end);
  return node;
}

Break* break_(Token*& tokens) {
  Break* node = (Break*) push_stack(sizeof(Break));
  Token* current = tokens;
  node->start = tokens;

  if (!check(current++, TokenType::BREAK)) return (Break*) pop_stack(node);
  if (!check(current++, TokenType::SEMI)) return (Break*) pop_stack(node);

  tokens = current;
  node->type = ASTType::BREAK;
  node->end = current;
  DEBUG("Match Break", node->start, node->end);
  return node;
}

Continue* continue_(Token*& tokens) {
  Continue* node = (Continue*) push_stack(sizeof(Continue));
  Token* current = tokens;
  node->start = tokens;

  if (!check(current++, TokenType::CONTINUE)) return (Continue*) pop_stack(node);
  if (!check(current++, TokenType::SEMI)) return (Continue*) pop_stack(node);

  tokens = current;
  node->type = ASTType::CONTINUE;
  node->end = current;
  DEBUG("Match Continue", node->start, node->end);
  return node;
}

Return* return_(Token*& tokens) {
  Return* node = (Return*) push_stack(sizeof(Return));
  Token* current = tokens;

  node->start = tokens;

  if (!check(current++, TokenType::RETURN)) return (Return*) pop_stack(node);

  node->expr = expr(current);

  if (!check(current++, TokenType::SEMI)) return (Return*) pop_stack(node);

  tokens = current;
  node->type = ASTType::RETURN;
  node->end = current;
  DEBUG("Match Return", node->start, node->end);
  return node;
}

Statement* statement(Token*& tokens) {
  Statement* node = (Statement*) push_stack(sizeof(Statement));
  Token* current = tokens;

  node->start = tokens;

  DEBUG_PRINT("Try Statement");
  switch (current->type) {
    case TokenType::IF:
      node->type = ASTType::STATEMENT_CONDITION;
      node->conditional = conditional(current);
      if (node->conditional == nullptr) return (Statement*) pop_stack(node); 
      tokens = current;
      node->end = current;
      DEBUG("Match Conditional Statement", node->start, node->end);
      return node;

    case TokenType::WHILE:
      node->type = ASTType::STATEMENT_WHILE;
      node->while_ = while_(current); 
      if (node->while_ == nullptr) return (Statement*) pop_stack(node); 
      tokens = current;
      node->end = current;
      DEBUG("Match While Statement", node->start, node->end);
      return node;

    case TokenType::BREAK:
      node->type = ASTType::STATEMENT_BREAK;
      node->break_ = break_(current);
      if (node->break_ == nullptr) return (Statement*) pop_stack(node); 
      tokens = current;
      node->end = current;
      DEBUG("Match Break Statement", node->start, node->end);
      return node;

    case TokenType::CONTINUE:
      node->type = ASTType::STATEMENT_CONTINUE;
      node->continue_ = continue_(current);
      if (node->continue_ == nullptr) return (Statement*) pop_stack(node); 
      tokens = current;
      node->end = current;
      DEBUG("Match Continue Statement", node->start, node->end);
      return node;

    case TokenType::RETURN:
      node->type = ASTType::STATEMENT_RETURN;
      node->return_ = return_(current);
      if (node->return_ == nullptr) return (Statement*) pop_stack(node); 
      tokens = current;
      node->end = current;
      DEBUG("Match Return Statement", node->start, node->end);
      return node;

    default:
      node->type = ASTType::STATEMENT_ASSIGN;
      node->assignment = assignment(current);
      if (node->assignment != nullptr) {
        tokens = current;
        node->end = current;
        DEBUG("Match Assignment Statement", node->start, node->end);
        return node;
      }

      node->type = ASTType::STATEMENT_EXPR;
      node->expr = expr(current);
      if (node->expr == nullptr) return (Statement*) pop_stack(node); 
      if (!check(current++, TokenType::SEMI)) return (Statement*) pop_stack(node);
      
      tokens = current;
      node->end = current;
      DEBUG("Match Expr Statement", node->start, node->end);
      return node;
  }
}

BlockTag* blockTag(Token*& tokens) {
  BlockTag* node = (BlockTag*) push_stack(sizeof(BlockTag));
  Token* current = tokens;
  node->start = tokens;

  node->statement = statement(current);
  if (node->statement != nullptr){
    tokens = current;
    node->type = ASTType::BLOCK_TAG_STATEMENT;
    node->end = tokens;
    DEBUG("Match Block Tag Statement", node->start, node->end);
    return node;
  }

  node->block = block(current);
  if (node->block != nullptr) {
    tokens = current;
    node->type = ASTType::BLOCK_TAG_BLOCK;
    node->end = tokens;
    DEBUG("Match Block Tag Block", node->start, node->end);
    return node;
  }

  return (BlockTag*) pop_stack(node);
}

Block* block(Token*& tokens) {
  Block* node = (Block*) push_stack(sizeof(Block));

  unsigned byte_count_declarations;
  Declaration** declarations_buffer = (Declaration**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(Declaration*));

  unsigned byte_count_tags;
  BlockTag** block_tag_buffer = (BlockTag**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(BlockTag*));

  Token* current = tokens;
  node->start = tokens;

  DEBUG_PRINT("Try Block");
  if (!check(current, TokenType::LEFT_BRACE)){
    node->statement = statement(current);
    if (node->statement == nullptr) return (Block*) pop_stack(node);
   
    tokens = current;
    node->type = ASTType::BLOCK;
    node->end = current;
    DEBUG("Match Single Statement Block", node->start, node->end);
    return node;
  }

  current++;

  if (check(current, TokenType::COLON)) {
    node->namespace_ = identifier(++current);
    if (node->namespace_ == nullptr) return (Block*) pop_stack(node);
    
    if (!check(current, TokenType::SEMI)) return (Block*) pop_stack(node);
  }

  DEBUG_PRINT("Try Block Decls");
  declarations_buffer[node->declarations_count] = declaration(current);
  while (declarations_buffer[node->declarations_count] != nullptr) {
    declarations_buffer[++node->declarations_count] = declaration(current);
  }

  DEBUG_PRINT("Try Block Tags");
  block_tag_buffer[node->block_tags_count] = blockTag(current);
  while (block_tag_buffer[node->block_tags_count] != nullptr) {
    block_tag_buffer[++node->block_tags_count] = blockTag(current);
  }

  DEBUG_PRINT("Try Block Right Brace");
  if (!check(current++, TokenType::RIGHT_BRACE)) return (Block*) pop_stack(node);

  byte_count_declarations = node->declarations_count * sizeof(Declaration*);
  node->declarations = (Declaration**) push_stack(byte_count_declarations); 
  memcpy(node->declarations, declarations_buffer, byte_count_declarations);

  byte_count_tags = node->block_tags_count * sizeof(BlockTag*);
  node->block_tags = (BlockTag**) push_stack(byte_count_tags); 
  memcpy(node->block_tags, block_tag_buffer, byte_count_tags);

  tokens = current;
  node->type = ASTType::BLOCK;
  node->end = tokens;
  DEBUG("Match Block", node->start, node->end);
  return node;
}

Literal* literal(Token*& tokens) {
  Literal* node = (Literal*) push_stack(sizeof(Literal));
  node->start = tokens;

  // TODO: parse the actual literal
  switch (tokens->type) {
    case TokenType::STRING_LITERAL:
      node->type = ASTType::LITERAL_STRING;
      node->end = ++tokens;
      DEBUG("Match Literal String", node->start, node->end);
      return node;
    case TokenType::INT_LITERAL:
      node->type = ASTType::LITERAL_INT;
      node->end = ++tokens;
      DEBUG("Match Literal Int", node->start, node->end);
      return node;
    case TokenType::FLOAT_LITERAL:
      node->type = ASTType::LITERAL_FLOAT;
      node->end = ++tokens;
      DEBUG("Match Literal Float", node->start, node->end);
      return node;
    case TokenType::TRUE:
      node->type = ASTType::LITERAL_BOOL;
      node->bool_ = true;
      node->end = ++tokens;
      DEBUG("Match Literal Bool", node->start, node->end);
      return node;
    case TokenType::FALSE:
      node->type = ASTType::LITERAL_BOOL;
      node->bool_ = false;
      node->end = ++tokens;
      DEBUG("Match Literal Bool", node->start, node->end);
      return node;
    default:
      return (Literal*) pop_stack(node);
  }
}

Call* call(Token*& tokens) {
  Call* node = (Call*) push_stack(sizeof(Call));
  Token* current = tokens;

  unsigned byte_count;
  Expr** args_buffer = (Expr**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(Expr*));

  node->start = tokens;

  node->identifier = identifier(current);
  if (node->identifier == nullptr) return (Call*) pop_stack(node);

  if (!check(current++, TokenType::LEFT_PAREN)) return (Call*) pop_stack(node);

  args_buffer[node->arguments_count] = expr(current);
  while (args_buffer[node->arguments_count] != nullptr) {
    if (check(current, TokenType::COMMA)) current++;
    args_buffer[++node->arguments_count] = expr(current);
  }
  
  if (!check(current++, TokenType::RIGHT_PAREN)) return (Call*) pop_stack(node);

  byte_count = node->arguments_count * sizeof(Expr*);
  node->arguments = (Expr**) push_stack(byte_count); 
  memcpy(node->arguments, args_buffer, byte_count);

  tokens = current;
  node->type = ASTType::CALL;
  node->end = current;
  DEBUG("Match Call", node->start, node->end);
  return node;
}

Unary* unary(Token*& tokens) {
  Unary* node = (Unary*) push_stack(sizeof(Unary));
  Token* current = tokens;

  node->start = tokens;

  switch (current->type) {
    case TokenType::NOT:
      node->type = ASTType::UNARY_NOT;

    case TokenType::ADD:
      node->type = ASTType::UNARY_PLUS;

    case TokenType::SUB:
      node->type = ASTType::UNARY_MINUS;

    default:
      return (Unary*) pop_stack(node);
  }

  node->expr = expr(++current, false);
  if (node->expr == nullptr) return (Unary*) pop_stack(node);

  node->end = current;
  DEBUG("Match Unary", node->start, node->end);
  return node;
}

Expr* makeBinaryExpr(Token* start, Token* end, Expr* first, Expr* second, ASTType type) {
  /*
   * WARNING: Make sure you understand how binary() works before changing this.
   * binary() depends on this allocating the Binary before the Expr.
   */

  Expr* node;
  Binary* binary;

  binary = (Binary*) push_stack(sizeof(Binary));
  binary->type = type;
  binary->start = start;
  binary->end = end;
  binary->first = first;
  binary->second = second;

  node = (Expr*) push_stack(sizeof(Expr));
  node->type = ASTType::EXPRESSION_BINARY;
  node->binary = binary;
  node->start = start;
  node->end = end;

  return node;
}

Binary* binary(Token*& tokens) {
  Binary* node = (Binary*) push_stack(sizeof(Binary));
  Token* current = tokens;
  ASTType type; 
  
  unsigned expr_count = 0;
  Expr** expr_buffer = (Expr**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(Expr*));
  unsigned op_count = 0;
  ASTType* op_buffer = (ASTType*) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(ASTType));

  expr_buffer[expr_count] = expr(current, false);
  if (expr_buffer[expr_count++] == nullptr) return (Binary*) pop_stack(node);

  bool is_binary_expr = false;
  do {
    Expr* expr_ptr;
    ASTType op;

    switch(current->type) {
      case TokenType::MUL:
        op = ASTType::BINARY_MUL;
        break;

      case TokenType::DIV:
        op = ASTType::BINARY_DIV;
        break;

      case TokenType::MOD:
        op = ASTType::BINARY_MOD;
        break;

      case TokenType::ADD:
        op = ASTType::BINARY_ADD;
        break;

      case TokenType::SUB:
        op = ASTType::BINARY_SUB;
        break;

      case TokenType::LT:
        op = ASTType::BINARY_LT;
        break;

      case TokenType::GT:
        op = ASTType::BINARY_GT;
        break;

      case TokenType::LE:
        op = ASTType::BINARY_LE;
        break;

      case TokenType::GE:
        op = ASTType::BINARY_GE;
        break;

      case TokenType::EQ:
        op = ASTType::BINARY_EQ;
        break;

      case TokenType::NE:
        op = ASTType::BINARY_NE;
        break;

      case TokenType::AND:
        op = ASTType::BINARY_AND;
        break;

      case TokenType::OR:
        op = ASTType::BINARY_OR;
        break;

      case TokenType::XOR:
        op = ASTType::BINARY_XOR;
        break;

      default:
        op = ASTType::NONE;
    }
    
    if (op == ASTType::NONE) break;

    current++;
    is_binary_expr = true;
    expr_ptr = expr(current, false);
    if (expr_ptr == nullptr) break; 

    while (op_count > 0 && getBinaryPrecidence(op) > getBinaryPrecidence(op_buffer[op_count - 1])) {
      Expr* first = expr_buffer[expr_count - 1];
      Expr* second = expr_buffer[expr_count - 2];
      ASTType top_op = op_buffer[op_count - 1];

      expr_buffer[expr_count - 2] = makeBinaryExpr(first->start, second->end, first, second, top_op);
      expr_count--;
      op_count--;
    }

    expr_buffer[expr_count++] = expr_ptr;
    op_buffer[op_count++] = op;
  } while(true);

  while (op_count > 0) {
    Expr* first = expr_buffer[expr_count - 1];
    Expr* second = expr_buffer[expr_count - 2];
    ASTType top_op = op_buffer[op_count - 1];

    expr_buffer[expr_count - 2] = makeBinaryExpr(first->start, second->end, first, second, top_op);
    expr_count--;
    op_count--;
  }

  if (!is_binary_expr) return (Binary*) pop_stack(node);

  /*
   * WARNING: this is fragile. we know that expr_buffer contains
   * exprs but we need a binary. If it is a binary expression then 
   * expr_buffer[0] ends up being a binary expression. Since we always
   * allocate these with the helper makeBinaryExpr we know that Expr 
   * is allocated after its binary on the stack so we delete the extra
   * Expr by poping the stack to it. This leaves its Binary in tact.
   */

  node->start = tokens;
  tokens = current;
  node = expr_buffer[0]->binary;
  node->end = current;
  //pop_stack(expr_buffer[0]);
  DEBUG("Match Binary Expression", node->start, node->end);
  return node;
}

Expr* expr(Token*& tokens, bool check_binary) {
  Expr* node;
  Token* current = tokens;
  
  DEBUG_PRINT("Try Expr Left Paren");
  if (check(current, TokenType::LEFT_PAREN)) {
    current++;
    node = expr(current, check_binary);
    if (node != nullptr) {
      if (check(current++, TokenType::LEFT_PAREN)) {
          tokens = current;
          DEBUG("Match Expr Paren", node->start, node->end);
          return node;
      }
    }
  }
  DEBUG_PRINT("Fail Expr Left Paren");

  node = (Expr*) push_stack(sizeof(Expr));
  node->start = tokens;

  DEBUG_PRINT("Try Expr Call");
  node->call = call(current);
  if (node->call != nullptr) {
    tokens = current;
    node->type = ASTType::EXPRESSION_CALL;
    node->end = current;
    DEBUG("Match Expr Call", node->start, node->end);
    return node;
  }

  DEBUG_PRINT("Try Expr Unary");
  node->unary = unary(current);
  if (node->unary != nullptr) {
    tokens = current;
    node->type = ASTType::EXPRESSION_UNARY;
    node->end = current;
    DEBUG("Match Expr Unary", node->start, node->end);
    return node;
  }

  if (check_binary) {
    DEBUG_PRINT("Try Expr Binary");
    node->binary = binary(current);
    if (node->binary != nullptr) {
      tokens = current;
      node->type = ASTType::EXPRESSION_BINARY;
      node->end = current;
      DEBUG("Match Expr Binary", node->start, node->end);
      return node;
    }
  }
    
  DEBUG_PRINT("Try Expr Identifier");
  node->identifier = identifier(current);
  if (node->identifier != nullptr) {
    tokens = current;
    node->type = ASTType::EXPRESSION_IDENTIFIER;
    node->end = current;
    DEBUG("Match Expr Identifier", node->start, node->end);
    return node;
  }
     
  DEBUG_PRINT("Try Expr Literal");
  node->literal = literal(current);
  if (node->literal != nullptr) {
    tokens = current;
    node->type = ASTType::EXPRESSION_LITERAL;
    node->end = current;
    DEBUG("Match Expr Literal", node->start, node->end);
    return node;
  }
    
  DEBUG_PRINT("Fail Expr");
  return (Expr*) pop_stack(node);
}

Expr* expr(Token*& tokens) {
  return expr(tokens, true);
}

FunctionParam* functionParam(Token*& tokens) {
  FunctionParam* node = (FunctionParam*) push_stack(sizeof(FunctionParam));

  Token* current = tokens;
  node->start = tokens;
  
  node->identifier = identifier(current);
  if (node->identifier != nullptr) { 
    if (!check(current++, TokenType::COLON)) return (FunctionParam*) pop_stack(node);
  }

  node->decl_type = type(current);
  if (node->decl_type != nullptr) {
    tokens = current;
    node->type = ASTType::FUNC_PARAM;
    node->end = tokens;
    DEBUG("Match Function Param", node->start, node->end);
    return node;
  }
  
  return (FunctionParam*) pop_stack(node);
}

FunctionHeader* functionHeader(Token*& tokens) {
  FunctionHeader* node = (FunctionHeader*) push_stack(sizeof(FunctionHeader));

  unsigned byte_count;
  FunctionParam** param_buffer = (FunctionParam**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(FunctionParam*));

  Token* current = tokens;
  node->start = tokens;

  if (check(current, TokenType::EXPORT)) {
    node->export_ = true;
    current++;
  }

  if (!check(current++, TokenType::FUNC)) return (FunctionHeader*) pop_stack(node);

  node->identifier = identifier(current);
  if (node->identifier == nullptr) return (FunctionHeader*) pop_stack(node);

  if(!check(current++, TokenType::LEFT_PAREN)) return (FunctionHeader*) pop_stack(node);

  param_buffer[node->parameter_count] = functionParam(current);
  while(param_buffer[node->parameter_count] != nullptr) {
    if (check(current, TokenType::COMMA)) current++;
    param_buffer[++node->parameter_count] = functionParam(current);
  }

  if(!check(current++, TokenType::RIGHT_PAREN)) return (FunctionHeader*) pop_stack(node);
  if(!check(current++, TokenType::COLON)) return (FunctionHeader*) pop_stack(node);

  node->return_type = type(current);
  if (node->return_type == nullptr) return (FunctionHeader*) pop_stack(node);

  byte_count = node->parameter_count * sizeof(FunctionParam*);
  node->parameter_list = (FunctionParam**) push_stack(byte_count); 
  memcpy(node->parameter_list, param_buffer, byte_count);

  tokens = current;
  node->type = ASTType::FUNC_HEADER;
  node->end = current;
  DEBUG("Match Function Header", node->start, node->end);
  return node;
}

Function* function(Token*& tokens) {
  Function* node = (Function*) push_stack(sizeof(Function));

  Token* current = tokens;
  node->start = tokens;

  node->header = functionHeader(current);
  if (node->header == nullptr) return (Function*) pop_stack(sizeof(Function));

  if (check(current, TokenType::SEMI)) {
    tokens = ++current;
    node->type = ASTType::FUNC_FORWARD;
    node->end = current;
    DEBUG("Match Function Forward Declaration", node->start, node->end);
    return node;
  }

  node->block = block(current);
  if (node->block == nullptr) return (Function*) pop_stack(sizeof(Function));

  tokens = current;
  node->type = ASTType::FUNCTION;
  node->end = current;
  DEBUG("Match Function Definition", node->start, node->end);
  return node;
}

Struct* struct_(Token*& tokens) {
  Struct* node = (Struct*) push_stack(sizeof(Struct));

  unsigned byte_count;
  Declaration** decl_buffer = (Declaration**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(Declaration*));

  Token* current = tokens;
  node->start = tokens;

  if (!check(current++, TokenType::STRUCT)) return (Struct*) pop_stack(node);
  
  node->identifier = identifier(current);
  if (node->identifier == nullptr) return (Struct*) pop_stack(node);

  if (!check(current++, TokenType::LEFT_BRACE)) return (Struct*) pop_stack(node);

  decl_buffer[node->declarations_count] = declaration(current);
  while (decl_buffer[node->declarations_count] != nullptr) {
    decl_buffer[++node->declarations_count] = declaration(current);
  }

  if (!check(current++, TokenType::RIGHT_BRACE)) return (Struct*) pop_stack(node);
  if (!check(current++, TokenType::SEMI)) return (Struct*) pop_stack(node);

  byte_count = node->declarations_count * sizeof(Declaration*);
  node->declarations = (Declaration**) push_stack(byte_count); 
  memcpy(node->declarations, decl_buffer, byte_count);

  tokens = current;
  node->type = ASTType::STRUCT;
  node->end = tokens;
  DEBUG("Match Struct", node->start, node->end);
  return node;
}

Enum* enum_(Token*& tokens) {
  Enum* node = (Enum*) push_stack(sizeof(Enum));

  unsigned byte_count;
  Token** token_buffer = (Token**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(Token*));

  Token* current = tokens;
  node->start = tokens;

  if (!check(current++, TokenType::ENUM)) return (Enum*) pop_stack(node);

  node->identifier = identifier(current);
  if (node->identifier == nullptr) return (Enum*) pop_stack(node);

  if (!check(current++, TokenType::LEFT_BRACE)) return (Enum*) pop_stack(node);

  while (check(current, TokenType::IDENTIFIER)) {
    token_buffer[node->members_count++] = current++;
    if (check(current, TokenType::COMMA)) current++;
  }

  if (!check(current++, TokenType::RIGHT_BRACE)) return (Enum*) pop_stack(node);
  if (!check(current++, TokenType::SEMI)) return (Enum*) pop_stack(node);

  byte_count = node->members_count * sizeof(Token*);
  node->members = (Token**) push_stack(byte_count);
  memcpy(node->members, token_buffer, byte_count);

  tokens = current;
  node->type = ASTType::ENUM;
  node->end = tokens;
  DEBUG("Match Enum", node->start, node->end);
  return node;

}

PrimaryTag* primary_tag(Token*& tokens) {
  PrimaryTag* node = (PrimaryTag*) push_stack(sizeof(PrimaryTag));
  node->start = tokens;
  
  DEBUG_PRINT("Try Primary Decl");
  node->decl = declaration(tokens);
  if (node->decl != nullptr) {
    node->type = ASTType::PRIMARY_TAG_DECL;
    node->end = tokens;
    return node;
  }

  DEBUG_PRINT("Try Primary Struct");
  node->struct_ = struct_(tokens);
  if (node->struct_ != nullptr) {
    node->type = ASTType::PRIMARY_TAG_STRUCT;
    node->end = tokens;
    return node;
  }

  DEBUG_PRINT("Try Primary Enum");
  node->enum_ = enum_(tokens);
  if (node->enum_ != nullptr) {
    node->type = ASTType::PRIMARY_TAG_ENUM;
    node->end = tokens;
    return node;
  }

  DEBUG_PRINT("Try Primary Function");
  node->func = function(tokens);
  if (node->func != nullptr) {
    node->type = ASTType::PRIMARY_TAG_FUNC;
    node->end = tokens;
    return node;
  }

  pop_stack(node);
  assert(false && "Primary tag");
}

Primary* primary(Token*& tokens) { 
  Primary* node = (Primary*) push_stack(sizeof(Primary));

  unsigned byte_count;
  PrimaryTag** tag_buffer = (PrimaryTag**) push_buffer_pool(MAX_BUFFER_COUNT * sizeof(PrimaryTag*));
  node->start = tokens;

  while (tokens->type != TokenType::END) {
    PrimaryTag* tag = primary_tag(tokens);
    tag_buffer[node->primary_tags_count++] = tag;
  }

  byte_count = node->primary_tags_count * sizeof(PrimaryTag*);
  node->primary_tags = (PrimaryTag**) push_stack(byte_count);
  memcpy(node->primary_tags, tag_buffer, byte_count);

  node->type = ASTType::PRIMARY;
  node->end = tokens;
  return node;
}

Primary* parse(Token* tokens) {
  Token* current = tokens;
  Primary* output;

  create_stack();
  create_buffer_pool();
  output = primary(current);
  destroy_buffer_pool();
  return output;
}

void parse_destroy() {
  destroy_stack();
}
