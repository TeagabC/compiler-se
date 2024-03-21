#pragma once

struct HashTableEntry {
  const char* key;
  void* value;
};

struct HashTable {
  HashTableEntry* entries;
  int capacity;
  int length;
};



HashTable* htCreate(int initial_capacity);
void htDestroy(HashTable* table);

void* htGet(HashTable* table, const char* key_start, const char* key_end);
const char* htSet(HashTable* table, const char* key_start, const char* key_end, void* value);
