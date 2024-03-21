#pragma once 
#include "ht.h"

extern HashTable* scopes;

struct Symbol;

struct Scope {
  Scope* parent;

  HashTable* symbols;
};

void scopeDeclare(Scope* scope, const char* start, const char* end, Symbol symbol);

Symbol scopeResolve(Scope* scope, const char* start, const char* end);
Symbol scopeResolveMember(Scope* scope, const char* start, const char* end);

bool scopeIsDefined(Scope* scope, const char* start, const char* end);
;
