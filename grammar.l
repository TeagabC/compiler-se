type :
  simple_type |
  id;
  
simple_type :
  i8 |
  u8 |
  i32 |
  u32 |
  f32;

id :
  IDENTIFIER (DOT IDENTIFIER)*;

/*
id_tag :
  id_tag DOT IDENTIFIER |
  ;
*/
qualifier :
  CONST |
  MUT |
  EXPORT;

decl :
  (qualifier)* id COLON type (SET expr)? SEMI

/*
decl2 :
  decl_tag id COLON type SEMI |
  decl_tag id COLON SET expr SEMI;

decl_tag : 
  decl_tag qualifier |
  ;
*/

assign :
  id SET expr SEMI;

conditional :
  IF expr block (ELSE block)?;

/*
conditional2 :
  IF expr block |
  IF expr block ELSE block;
*/

loop :
  WHILE expr block;

break :
  BREAK SEMI;

continue :
  CONTINUE SEMI;

return :
  RETURN expr SEMI |
  RETURN SEMI;

call :
  id LEFT_BRACKET (expr (COMMA expr)*)? RIGHT_BRACKET;
/*
call2 :
  id LEFT_BRACKET RIGHT_BRACKET |
  id LEFT_BRACKET expr call_tag RIGHT_BRACKET;

call_tag :
  call_tag COMMA expr |
  ;
*/

statement :
  assign |
  conditional |
  loop |
  break |
  continue |
  return;

block :
  LEFT_BRACE COLON IDENTIFER SEMI (decl*)? (statement | block)* RIGHT_BRACE |
  LEFT_BRACE decl* (statement | block)* RIGHT_BRACE |
  statement;

/*
block2 :
  LEFT_BRACE COLON IDENTIFIER SEMI block_decl_tag block_statement_tag RIGHT_BRACE |
  LEFT_BRACE block_decl_tag block_statement_tag RIGHT_BRACE |
  statement;

block_decl_tag :
  block_decl_tag decl |
  ;

block_statement_tag :
  block_statement_tag statement |
  block_statement_tag block |
  ;
*/
literal :
  INT_LITERAL |
  STRING_LITERAL |
  FLOAT_LITERAL |
  TRUE |
  FALSE;

unary :
  ADD |
  SUB |
  NOT;

binary_mul :
  expr MUL expr |
  expr DIV expr |
  expr MOD expr;

binary_add :
  expr ADD expr |
  expr SUB expr;

comp_direction :
  expr LT expr |
  expr GT expr |
  expr LE expr |
  expr GE expr;

comp_equality :
  expr EQ expr |
  expr NE expr;

bitwise_and :
  expr AND expr;

bitwise_or :
  expr OR expr |
  expr XOR expr;

binary_expr :
  binary_mul |
  binary_add |
  comp_direction |
  comp_equality |
  bitwise_and |
  bitwise_or;

expr :
  LEFT_BRACKET expr RIGHT_BRACKET |
  call |
  <assoc=right> unary expr |
  binary_expr |
  id |
  literal;

function_param :
  i COLON type |
  type;

function_header :
  (EXPORT)? FUNC id LEFT_BRACKET (function_param (COMMA function_param)*)? RIGHT_BRACKET COLON type;

/*
function_header :
  FUNC id LEFT_BRACKET RIGHT_BRACKET COLON type |
  FUNC id LEFT_BRACKET function_header_params_tag RIGHT_BRACKET COLON type |
  EXPORT FUNC id LEFT_BRACKET RIGHT_BRACKET COLON type |
  EXPORT FUNC id LEFT_BRACKET function_header_params_tag RIGHT_BRACKET COLON type |

function_header_tag :
  function_header_tag COMMA function_param |
  function_param |
  ;
*/
function :
  function_header SEMI |
  function_header SET expr SEMI |
  function_header block;

struct_body :
  decl* ;

/*
struct_body2 :
  struct_body_tag;

struct_body_tag :
  struct_body_tag decl |
  ;
*/

struct :
  STRUCT id RIGHT_BRACE struct_body SEMI;

enum : 
  ENUM id RIGHT_BRACE LEFT_BRACE SEMI |
  ENUM id RIGHT_BRACE IDENTIFIER (COMMA IDENTIFIER)* RIGHT_BRACE SEMI;

primary :
  (decl |
  function)*;
/*
primary_tag :
  primary_tag decl |
  primary_tag function |
  ;
*/



