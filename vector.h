#pragma once

struct Vector {
  void* data;
  int item_size;
  int size;
  int capacity;
};

Vector* vecCreate(int initial_capacity, int item_size);
void vecDestroy(Vector* vector);

void* vecGet(Vector* vector, int index);
void vecSet(Vector* vector, int index, void* item);

void vecPush(Vector* vector, void* item);
