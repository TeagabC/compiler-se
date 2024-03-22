#include "vector.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

Vector* vecCreate(int initial_capacity, int item_size) {
  Vector* vector = (Vector*) malloc(sizeof(Vector));
  if (initial_capacity >= 0) {
    vector->data = malloc(initial_capacity * item_size);
  }
  else {
    vector->data = nullptr;
  }

  vector->item_size = item_size;
  vector->size = 0;
  vector->capacity = initial_capacity;

  return vector;
}

void vecDestroy(Vector*& vector) {
  if (vector->data != nullptr) {
    free(vector->data);
  }
  free(vector);
  vector = nullptr;
}

void* vecGet(Vector* vector, int index) {
  return ((uint8_t*) vector->data) + index * vector->item_size;

}

void vecSet(Vector* vector, int index, void* item) {
  memcpy(vecGet(vector, index), item, vector->item_size);
}

void vecPush(Vector* vector, void* item) {
  if (vector->size == vector->capacity) {
    int byte_count = std::max(1, 2 * vector->capacity * vector->item_size);
    void* data = malloc(byte_count);
    memcpy(data, vector->data, vector->capacity * vector->item_size);
    free(vector->data);
    vector->data = data;
  }
  
  vecSet(vector, vector->size++, item);
}
