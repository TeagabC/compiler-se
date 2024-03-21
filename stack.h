#pragma once

#include <cstdint>

struct Stack {
  uint8_t* base;
  uint8_t* current;
  int capacity;
};

Stack* stackCreate(int capacity);
void stackDestroy(Stack* stack);

void* stackPush(Stack* stack, int size);
void stackPop(Stack* stack, int size);

void* stackReset(Stack* stack, void *reset_ptr);
