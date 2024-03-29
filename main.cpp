#include "lex.h"
#include "parser.h"
#include "ast_types.h"
#include "visit_print.h"

#include <cstdlib>
#include <stdio.h>

int main() {
  char data[] = "enum E { }; enum F {}; struct S {}; export func x(y : u32, z : f32) : i32 { x : u32 = 10; continue; } const z : f32;";
  char* input = data;

  Token* tokens = lex(input, 512);

  Primary* primary = parse(tokens);

  printf("Primary: %p\n", primary);
  if (primary != nullptr) {
    printf("Tag count: %i\n", primary->primary_tags_count);
  }

  printASTNode(primary);
  for (int i = 0; i < primary->primary_tags_count; i++) {
    printASTNode(primary->primary_tags[i]);
  }
  printf("\n\n");

  visitPrint(primary);

  parse_destroy();
  lex_destroy(tokens);

  return 0;
}
