#pragma once 
#include "hash_table.h"
#include "stack.h"
#include "vector.h"

extern Stack* symbol_stack;

enum class SymbolType {
  NONE,
  I8,
  U8,
  I32,
  U32,
  F32,
  BOOL,
  STRING,
  POINTER,

  ENUM,
  ENUM_INSTANCE,

  STRUCT,
  STRUCT_INSTANCE,

  FUNCTION,
};

struct Scope;

struct VariableComponent {
  // value
};

struct PointerComponent {
  SymbolType type;
  // TODO: how do we manage this memory?
  union {
    // value
    PointerComponent* pointer;
  };
};

struct EnumComponent {
  HashTable* table;
};

struct EnumInstanceComponent {
  EnumComponent* enum_decl;
  // value
};

struct StructComponent {
  // TODO: should this have a hashmap that matchs members?
  // also maybe Symbol shouldnt hold a name and instead this holds a name symbol pair
  Scope* members_table;
  Vector* members_vector;
  int size;
};

struct StructInstanceComponent {
  StructComponent* struct_decl;
  // value
};

struct FunctionComponent {
  Scope* scope;
  Vector* parameter_vector;
  SymbolType simple_return_type;
  StructComponent* struct_return_type;
};


struct Symbol {
  SymbolType type;
  const char* start;
  const char* end;

  union {
    VariableComponent variable;
    PointerComponent poiner;
    FunctionComponent function;
    EnumComponent enum_;
    EnumInstanceComponent enum_instance;
    StructComponent struct_;
    StructInstanceComponent struct_instance;
  };
};

void printSymbol(Symbol* symbol);

void symbolStackCreate(int capacity = 16384);
void symbolStackDestroy();

Symbol* symbolCreateVariable(SymbolType type, const char* start, const char* end);
Symbol* symbolCreatePointer(const char* start, const char* end);
Symbol* symbolCreateEnum(const char* start, const char* end);
Symbol* symbolCreateEnumInstance(const char* start, const char* end, EnumComponent* enum_decl);
Symbol* symbolCreateStruct(const char* start, const char* end, Scope* parent);
Symbol* symbolCreateStructInstance(const char* start, const char* end, StructComponent* struct_decl);
Symbol* symbolCreateFunction(const char* start, const char* end, Scope* parent, Symbol return_type);

void symbolAddEnumChild(Symbol* symbol, const char* start, const char* end);
// This is a little misleading. This adds a start, end pair to structs member_list
// The assumption is that the structs scope will be pushed when visiting and so
// the member_table will be set by visiting and not this function.
void symbolAddStructChild(Symbol* symbol, const char* start, const char* end, Symbol* child);
void symbolAddFunctionParamChild(Symbol* symbol, const char* start, const char* end, Symbol* child);
