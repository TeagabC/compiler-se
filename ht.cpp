#include "ht.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

bool cmpPtrStr(const char* s1_start, const char* s1_end, const char* s2) {
  const char* p1 = s1_start;
  const char* p2 = s2;

  while (p1 != s1_end && *p2) {
    if (*p1++ != *p2++) return false;
  }

  return p1 == s1_end && !*p2;
}

const char* dupPtrStr(const char* start, const char* end) {
  char* output = (char*) malloc(end - start + 1);
  const char* p = start;
  char* p_out = output;

  while(p != end) {
    *p_out++ = *p++;
  }
  *p_out = '\0';

  return output;
}   

const uint32_t FNV32_BASIS = 16777619;
const uint32_t FNV32_PRIME = 2166136261;

uint32_t hashKey(const char* key_start, const char* key_end) {
  uint32_t hash = FNV32_BASIS;
  for(const char* p = key_start; p != key_end; p++) {
    hash ^= (uint32_t)(unsigned char)(*p);
    hash *= FNV32_PRIME;
  }

  return hash;
}

uint32_t hashKey(const char* key) {
  uint32_t hash = FNV32_BASIS;
  for(const char* p = key; *p; p++) {
    hash ^= (uint32_t)(unsigned char)(*p);
    hash *= FNV32_PRIME;
  }

  return hash;
}

HashTable* htCreate(int initial_capacity) {
  HashTable* table = (HashTable*) malloc(sizeof(HashTable));
  
  if (table == nullptr) return nullptr;

  table->length = 0;
  table->capacity = initial_capacity;

  table->entries = (HashTableEntry*) calloc(table->capacity, sizeof(HashTableEntry));
  if (table->entries == nullptr) {
    free(table);
    return nullptr;
  }

  return table;
}

void htDestroy(HashTable* table) {
  for (int i = 0; i < table->capacity; i++) {
    free((void*)table->entries[i].key);
  }

  free(table->entries);
  free(table);
}

void* htGet(HashTable* table, const char* key_start, const char* key_end) {
  uint32_t hash = hashKey(key_start, key_end);
  int index = hash & (table->capacity - 1);
  
  while(table->entries[index].key != nullptr) {
    if (cmpPtrStr(key_start, key_end, table->entries[index].key)) {
      return table->entries[index].value;
    }

    index++;

    if (index >= table->capacity) {
      index = 0;
    }
  }
  return nullptr;
}

const char* htSetEntry(HashTable* table, const char* key_start, const char* key_end, void* value) {
  uint32_t hash = hashKey(key_start, key_end);
  int index = hash & (table->capacity - 1);
  const char* key;

  while (table->entries[index].key != nullptr) {
    if (cmpPtrStr(key_start, key_end, table->entries[index].key)){
      table->entries[index].value = value;
      return table->entries[index].key;
    }

    index++;
    if (index >= table->capacity) {
      index = 0;
    }
  }

  key = dupPtrStr(key_start, key_end);
  // TODO: should this assert?
  if (key == nullptr) return nullptr;

  table->length++;
  table->entries[index].key = key;
  table->entries[index].value = value;
  return key;
}

const char* htExpandSetEntry(HashTable* table, const char* key, void* value) {
  uint32_t hash = hashKey(key);
  int index = hash & (table->capacity - 1);
  const char* new_key;

  while (table->entries[index].key != nullptr) {
    if (strcmp(key, table->entries[index].key) == 0){
      table->entries[index].value = value;
      return table->entries[index].key;
    }

    index++;
    if (index >= table->capacity) {
      index = 0;
    }
  }

  table->length++;
  table->entries[index].key = key;
  table->entries[index].value = value;
  return key;
}

void htExpand(HashTable* table) {
  HashTableEntry* old_entries = table->entries;
  int old_length = table->length;
  int old_capacity = table->capacity;
  int new_capacity = 2 * table->capacity;

  if (new_capacity < table->capacity) assert(false && "htExpand new_capacity < old_capacity");

  table->entries = (HashTableEntry*) calloc(new_capacity, sizeof(HashTableEntry));
  if (table->entries == nullptr) {
    table->entries = old_entries;
    assert(false && "htExpand out of memory");
  }

  table->capacity = new_capacity;
  table->length = 0;

  for (int i = 0; i < old_capacity; i++) {
    HashTableEntry entry = old_entries[i];
    if (entry.key != nullptr) {
      htExpandSetEntry(table, entry.key, entry.value);
    }
  }

  free(old_entries);
}

const char* htSet(HashTable* table, const char* key_start, const char* key_end, void* value) {
  if (table->length >= table->capacity / 2) {
    htExpand(table);
  }

  return htSetEntry(table, key_start, key_end, value);
}

  

  

