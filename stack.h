#pragma once

#include <cstdint>

struct Stack {
  uint8_t* base;
  uint8_t* current;
  int capacity;
  bool zero_initialize;
};

Stack* stackCreate(int capacity, bool zero_initalize = false);
void stackDestroy(Stack* stack);

void* stackPush(Stack* stack, int size);
void stackPop(Stack* stack, int size);
void stackPop(Stack* stack, void *reset_ptr);
