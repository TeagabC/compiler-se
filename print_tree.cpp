#include "lex.h"
#include "parser.h"
#include "ast_types.h"
#include "visit_print.h"
#include "defref.h"

#include <cstdlib>
#include <fstream>
#include <stdio.h>



int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: print_tree filename\n");
    return -1;
  }

  std::ifstream f(argv[1]);
  f.seekg(0, std::ios::end);
  size_t size = f.tellg();

  char* input = (char*) malloc(size);
  f.seekg(0);
  f.read(input, size);

  Token* tokens = lex(input, 512);

  if (tokens == nullptr) {
    printf("Failed to lex\n");
  }

  printf("\nStarting Parse:\n\n");

  Primary* primary = parse(tokens);

  printf("\nStarting Visit:\n\n");

  visitPrint(primary);

  visitDefRef(primary);


  defref_destroy();
  parse_destroy();
  free(tokens);
  
  return 0;
}
