#include "scope.h"
#include "hash_table.h"
#include "stack.h"
#include "symbol.h"

#define SCOPE_DEBUG

#ifdef SCOPE_DEBUG
#include <cassert>
#define SCOPE_ASSERT(...) assert(__VA_ARGS__)
#endif 

Stack* scope_stack;

void scopeStackCreate(int capacity) {
  SCOPE_ASSERT(scope_stack == nullptr && "scopeStackCreate: scope stack in not nullptr");
  scopes_stack = stackCreate(capacity * sizeof(Scope));
  SCOPE_ASSERT(scope_stack != nullptr && "scopeStackCreate: out of memory");
}

void scopeStackDestroy() {
  SCOPE_ASSERT(scope_stack != nullptr);

  while (stackSize(scope_stack) > 0) {
    Scope* current = (Scope*) stackPop(scope_stack, sizeof(Scope));

    SCOPE_ASSERT(current != nullptr);
    SCOPE_ASSERT(current->symbols != nullptr);

    htDestroy(current->symbols);
  }

  stackDestroy(scope_stack);
  scope_stack = nullptr;
}

Scope* scopeCreate(Scope* parent, int capacity) {
  SCOPE_ASSERT(scope_stack != nullptr && "scopeCreate: scopes stack is nullptr");
  
  Scope* scope = (Scope*) stackPush(scope_stack, sizeof(Scope));
  scope->parent = parent;
  scope->symbols = htCreate(capacity);

  return scope;
}

void scopeDeclare(Scope* scope, Symbol* symbol) {
  SCOPE_ASSERT(scope != nullptr);
  htSet(scope->symbols, symbol->start, symbol->end, symbol);
}

Symbol* scopeResolve(Scope* scope, const char* start, const char* end) {
  SCOPE_ASSERT(scope != nullptr);
  Symbol* symbol = (Symbol*) htGet(scope->symbols, start, end);
  
  if (symbol != nullptr) {
    return symbol;
  }

  if (scope->parent != nullptr) {
    return scopeResolve(scope->parent, start, end);
  }

  return nullptr;
}

Symbol* scopeResolveMember(Scope* scope, const char* start, const char* end) {
  SCOPE_ASSERT(scope != nullptr);
  Symbol* symbol = (Symbol*) htGet(scope->symbols, start, end);
  
  if (symbol != nullptr) {
    return symbol;
  }
  return nullptr;
}

bool scopeIsDefined(Scope* scope, const char* start, const char* end) {
  SCOPE_ASSERT(scope != nullptr);
  Symbol* symbol = (Symbol*) htGet(scope->symbols, start, end);
  if (symbol != nullptr) {
    return true;
  }
  return false;
}

