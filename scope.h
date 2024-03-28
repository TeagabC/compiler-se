#pragma once 
#include "hash_table.h"
#include "stack.h"

extern Stack* scopes_stack;

struct Symbol;

struct Scope {
  Scope* parent;
  HashTable* symbols;
};

void scopeStackCreate(int capacity = 4096);
void scopeStackDestroy();

Scope* scopeCreate(Scope* parent, void* node, int capacity = 16);
Scope* scopeGet(void* index);

void scopeDeclare(Scope* scope, Symbol* symbol);

Symbol* scopeResolve(Scope* scope, const char* start, const char* end);
Symbol* scopeResolveMember(Scope* scope, const char* start, const char* end);

bool scopeIsDefined(Scope* scope, const char* start, const char* end);
