#include "stack.h"

#include <cstdlib>
#include <cstring>

#ifdef STACK_DEBUG
#define STACK_DEBUG_ASSERT
#define STACK_DEBUG_PRINT
#endif

#ifdef STACK_DEBUG_PRINT
#define DEBUG_PRINT(...) printf(__VA_ARGS__);
#else 
#define DEBUG_PRINT(...)
#endif

#ifdef STACK_DEBUG_ASSERT
#define DEBUG_ASSERT(...) assert(__VA_ARGS__);
#else
#define DEBUG_ASSERT(...)
#endif


Stack* stackCreate(int capacity, bool zero_initalize) {
  Stack* output = (Stack*) malloc(sizeof(Stack));
  output->base = (uint8_t*) malloc(capacity);
  
  DEBUG_ASSERT(output->base != nullptr && "stackCreate out of memory");

  output->current = output->base;
  output->capacity = capacity;
  output->zero_initialize = zero_initalize;
  DEBUG_PRINT("stackCreate: capacity = %i\n", capacity);

  return output;
}

void stackDestroy(Stack* stack) {
  DEBUG_ASSERT(stack != nullptr && "stackDestroy nullptr");

  free(stack->base);
  free(stack);

  DEBUG_PRINT("stackDestroy: %p\n", stack);
}

void* stackPush(Stack* stack, int size) {
  if (stack->current - stack->base + size >= stack->capacity) {

  }

  void* output = stack->current;
  stack->current += size;

  if (stack->zero_initialize) memset(output, 0, size);

  DEBUG_PRINT("stackPush: stack: %p size = %i\n", stack, size);
  return output;
}

void stackPop(Stack* stack, int size) {
  DEBUG_ASSERT(stack->current - size >= stack->base && "stackPop underflow");
  stack->current -= size;
  DEBUG_PRINT("stackPop: stack: %p size = %i\n", stack, size);
}

void stackPop(Stack* stack, void *reset_ptr) {
  DEBUG_ASSERT(reset_ptr < stack->current && reset_ptr > stack->base);
  stack->current = (uint8_t*) reset_ptr;
  DEBUG_PRINT("stackReset: stack: %p reset_ptr = %p\n", stack, reset_ptr);
}

