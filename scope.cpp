#include "scope.h"
#include "hash_table.h"
#include "stack.h"
#include "symbol.h"

#define SCOPE_DEBUG

#ifdef SCOPE_DEBUG
#define SCOPE_DEBUG_ASSERT
#define SCOPE_DEBUG_PRINT
#endif 

#ifdef SCOPE_DEBUG_ASSERT
#include <cassert>
#define DEBUG_ASSERT(...) assert(__VA_ARGS__)
#else 
#define DEBUG_ASSERT(...)
#endif

#ifdef SCOPE_DEBUG_PRINT
#include <cstdio>
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else 
#define DEBUG_PRINT(...) 
#endif

Stack* scopes_stack;

void scopeStackCreate(int capacity) {
  DEBUG_ASSERT(scopes_stack == nullptr && "scopeStackCreate: scope stack in not nullptr");
  scopes_stack = stackCreate(capacity * sizeof(Scope));
  DEBUG_ASSERT(scopes_stack != nullptr && "scopeStackCreate: out of memory");
}

void scopeStackDestroy() {
  DEBUG_ASSERT(scopes_stack != nullptr);

  while (stackSize(scopes_stack) > 0) {
    Scope* current = (Scope*) stackPop(scopes_stack, sizeof(Scope));

    DEBUG_ASSERT(current != nullptr);
    DEBUG_ASSERT(current->symbols != nullptr);

    htDestroy(current->symbols);
  }

  stackDestroy(scopes_stack);
  scopes_stack = nullptr;
}

Scope* scopeCreate(Scope* parent, int capacity) {
  DEBUG_ASSERT(scopes_stack != nullptr && "scopeCreate: scopes stack is nullptr");
  
  Scope* scope = (Scope*) stackPush(scopes_stack, sizeof(Scope));
  scope->parent = parent;
  scope->symbols = htCreate(capacity);

  DEBUG_PRINT("Create scope: %p\n", scope);

  return scope;
}

void scopeDeclare(Scope* scope, Symbol* symbol) {
  DEBUG_ASSERT(scope != nullptr);
  htSet(scope->symbols, symbol->start, symbol->end, symbol);
  DEBUG_PRINT("Scope declare: %p\n", scope);
}

Symbol* scopeResolve(Scope* scope, const char* start, const char* end) {
  DEBUG_ASSERT(scope != nullptr);
  Symbol* symbol = (Symbol*) htGet(scope->symbols, start, end);
  
  if (symbol != nullptr) {
    DEBUG_PRINT("Scope resolve: %p\n", scope);
    return symbol;
  }

  if (scope->parent != nullptr) {
    return scopeResolve(scope->parent, start, end);
  }

  return nullptr;
}

Symbol* scopeResolveMember(Scope* scope, const char* start, const char* end) {
  DEBUG_ASSERT(scope != nullptr);
  Symbol* symbol = (Symbol*) htGet(scope->symbols, start, end);
  
  if (symbol != nullptr) {
    DEBUG_PRINT("Scope resolve member: %p\n", scope);
    return symbol;
  }
  return nullptr;
}

bool scopeIsDefined(Scope* scope, const char* start, const char* end) {
  DEBUG_ASSERT(scope != nullptr);
  Symbol* symbol = (Symbol*) htGet(scope->symbols, start, end);
  if (symbol != nullptr) {
    return true;
  }
  return false;
}

