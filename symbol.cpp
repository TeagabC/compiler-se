#include "symbol.h"
#include "hash_table.h"
#include "scope.h"
#include "stack.h"
#include "vector.h"

#ifdef SYMBOL_DEBUG
#include <cassert>
#define SYMBOL_ASSERT(...) assert(__VA_ARGS__)
#else
#define SYMBOL_ASSERT(...)
#endif

#define ENUM_INITIAL_CAPACITY 8
#define STRUCT_INITIAL_CAPACITY 8
#define FUNCTION_PARAM_INITIAL_CAPACITY 8

Stack* symbol_stack = nullptr;

void symbolDestroyPointer(Symbol* symbol);
void symbolDestroyEnum(Symbol* symbol);
void symbolDestroyStruct(Symbol* symbol);

void printSymbol(Symbol* symbol) {

}

void symbolStackCreate(int capacity) {
  SYMBOL_ASSERT(symbol_stack == nullptr && "symbolStackCreate: symbol stack is not nullptr");
  symbol_stack = stackCreate(capacity * sizeof(Symbol));
  SYMBOL_ASSERT(symbol_stack != nullptr && "symbolStackCreate: out of memory");
}

void symbolStackDestroy() {
  SYMBOL_ASSERT(symbol_stack != nullptr);

  while (stackSize(symbol_stack) > 0) {
    Symbol* symbol = (Symbol*) stackPop(symbol_stack, sizeof(Symbol));
    switch (symbol->type) {
      case SymbolType::POINTER:
        symbolDestroyPointer(symbol);
        break;
      case SymbolType::FUNCTION:
        // Scopes are managed by scope_stack
        break;
      case SymbolType::ENUM:
        symbolDestroyEnum(symbol);
        break;
      case SymbolType::STRUCT:
        symbolDestroyStruct(symbol);
        break;
      default:
        break;
    }
  }

  stackDestroy(symbol_stack);
  symbol_stack = nullptr;
}

Symbol* symbolCreateVariable(SymbolType type, const char* start, const char* end) {
  SYMBOL_ASSERT(symbol_stack != nullptr);

  Symbol* symbol = (Symbol*) stackPush(symbol_stack, sizeof(Symbol));
  symbol->type = type;
  symbol->start = start;
  symbol->end = end;

  return symbol;
}

Symbol* symbolCreatePointer(const char* start, const char* end) {
  SYMBOL_ASSERT(symbol_stack != nullptr);

  Symbol* symbol = (Symbol*) stackPush(symbol_stack, sizeof(Symbol));
  symbol->type = SymbolType::POINTER;
  symbol->start = start;
  symbol->end = end;

  // TODO: get actual type of pointer and make linked list

  return symbol;
}

Symbol* symbolCreateEnum(const char* start, const char* end) {
  SYMBOL_ASSERT(symbol_stack != nullptr);

  Symbol* symbol = (Symbol*) stackPush(symbol_stack, sizeof(Symbol));
  symbol->type = SymbolType::ENUM;
  symbol->start = start;
  symbol->end = end;

  symbol->enum_.table = htCreate(ENUM_INITIAL_CAPACITY);

  return symbol;
}

Symbol* symbolCreateEnumInstance(const char* start, const char* end, Symbol enum_decl) {
  SYMBOL_ASSERT(symbol_stack != nullptr);
  SYMBOL_ASSERT(enum_decl != nullptr);
  SYMBOL_ASSERT(enum_decl->type == SymbolType::ENUM);

  Symbol* symbol = (Symbol*) stackPush(symbol_stack, sizeof(Symbol));
  symbol->type = SymbolType::ENUM_INSTANCE;
  symbol->start = start;
  symbol->end = end;

  symbol->enum_instance.enum_decl = &enum_decl.enum_;

  return symbol;
}

Symbol* symbolCreateStruct(const char* start, const char* end, Scope* parent) {
  SYMBOL_ASSERT(symbol_stack != nullptr);

  Symbol* symbol = (Symbol*) stackPush(symbol_stack, sizeof(Symbol));
  symbol->type = SymbolType::STRUCT;
  symbol->start = start;
  symbol->end = end;

  symbol->struct_.members_table = scopeCreate(parent);
  symbol->struct_.members_vector = vecCreate(STRUCT_INITIAL_CAPACITY, 2 * sizeof(char*));
  symbol->struct_.size = 0;

  return symbol;
}

Symbol* symbolCreateStructInstance(const char* start, const char* end, Symbol* struct_decl) {
  SYMBOL_ASSERT(symbol_stack != nullptr);
  SYMBOL_ASSERT(struct_decl != nullptr);
  SYMBOL_ASSERT(struct_decl->type == SymbolType::STRUCT);

  Symbol* symbol = (Symbol*) stackPush(symbol_stack, sizeof(Symbol));
  symbol->type = SymbolType::STRUCT_INSTANCE;
  symbol->start = start;
  symbol->end = end;

  symbol->struct_instance.struct_decl = &struct_decl->struct_;

  return symbol;
}

Symbol* symbolCreateFunction(const char* start, const char* end, Scope* parent, Symbol return_type) {
  SYMBOL_ASSERT(symbol_stack != nullptr);

  Symbol* symbol = (Symbol*) stackPush(symbol_stack, sizeof(Symbol));
  symbol->type = SymbolType::FUNCTION;
  symbol->start = start;
  symbol->end = end;

  symbol->function.scope = scopeCreate(parent);
  symbol->function.parameter_vector = vecCreate(FUNCTION_PARAM_INITIAL_CAPACITY, sizeof(Symbol*));

  if (return_type.type == SymbolType::STRUCT) {
    symbol->function.struct_return_type = &return_type.struct_;
  }
  else {
    symbol->function.simple_return_type = return_type.type;
  }

  return symbol;
}

void symbolDestroyPointer(Symbol *symbol) {
  // TODO: implement this
}

void symbolDestroyEnum(Symbol *symbol) {
  htDestroy(symbol->enum_.table);
}

void symbolDestroyStruct(Symbol *symbol) {
  // scope is managed by scope stack
  vecDestroy(symbol->struct_.members_vector);
}

void symbolAddEnumChild(Symbol* symbol, const char* start, const char* end) {
  SYMBOL_ASSERT(symbol != nullptr);
  SYMBOL_ASSERT(symbol->type == SymbolType::ENUM);

  // TODO: maybe this should hold a value* and a void* which is actually not a pointer
  htSet(symbol->enum_.table, start, end, (void*) (unsigned long) symbol->enum_.table->length);
}

void symbolAddStructChild(Symbol* symbol, const char* start, const char* end) {
  SYMBOL_ASSERT(symbol != nullptr);
  SYMBOL_ASSERT(symbol->type == SymbolType::STRUCT);
  SYMBOL_ASSERT(child != nullptr);

  const char* pair[2] = {start, end};
  vecPush(symbol->struct_.members_vector, pair);
}

void symbolAddFunctionParamChild(Symbol* symbol, const char* start, const char* end, Symbol* child) {
  SYMBOL_ASSERT(symbol != nullptr);
  SYMBOL_ASSERT(symbol->type == SymbolType::FUNCTION);

  scopeDeclare(symbol->function.scope, start, end, child);
  vecPush(symbol->function.parameter_vector, child);
}

