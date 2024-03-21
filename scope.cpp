#include "scope.h"
#include "symbol.h"

HashTable* scopes;

void scopeDeclare(Scope* scope, const char* start, const char* end, Symbol symbol) {
  htSet(scope->symbols, start, end, symbol);

}

Symbol scopeResolve(Scope* scope, const char* start, const char* end) {

}

Symbol scopeResolveMember(Scope* scope, const char* start, const char* end) {

}

bool scopeIsDefined(Scope* scope, const char* start, const char* end) {

}

