# Jaguar Grammar Specification (EBNF)

```ebnf
program             = { declaration } ;

declaration         = live_activation
                    | var_declaration
                    | fun_declaration
                    | struct_declaration
                    | class_declaration
                    | enum_declaration
                    | data_declaration
                    | import_statement
                    | export_declaration
                    | statement ;

live_activation     = "live" "=" "\"1\"" ";" ;

var_declaration     = ( "var" | "fixed" ) IDENTIFIER ":" type_spec [ "=" expression ] ";" ;

type_spec           = ( "num" | "decimal" | "bool" | "string" | "void" | "mixed" | "vector" | "matrix" | IDENTIFIER ) [ "[" "]" ] ;

fun_declaration     = [ "async" ] "fun" IDENTIFIER "(" [ parameter_list ] ")" [ ":" type_spec ] block ;

parameter_list      = parameter { "," parameter } ;
parameter           = IDENTIFIER ":" type_spec ;

struct_declaration  = "struct" IDENTIFIER "{" { IDENTIFIER ":" type_spec ";" } "}" ;

class_declaration   = "class" IDENTIFIER "{" { class_member } "}" ;
class_member        = [ "public" | "private" ] ( fun_declaration | IDENTIFIER ":" type_spec ";" ) ;

enum_declaration    = "enum" IDENTIFIER "{" IDENTIFIER { "," IDENTIFIER } "}" ;

statement           = if_statement
                    | loop_statement
                    | do_loop_statement
                    | for_in_statement
                    | return_statement
                    | block
                    | expression_statement ;

if_statement        = "if" "(" expression ")" block { "elif" "(" expression ")" block } [ "else" block ] ;

loop_statement      = "loop" "(" expression ")" block ;

do_loop_statement   = "do" "loop" block "while" "(" expression ")" ";" ;

for_in_statement    = "for" IDENTIFIER "in" expression block ;

return_statement    = "return" [ expression ] ";" ;

block               = "{" { declaration } "}" ;

expression          = assignment ;

assignment          = ternary [ ( "=" | "+=" | "-=" | "*=" | "/=" | "%=" ) assignment ] ;

ternary             = logical_or [ "?" expression ":" expression ] ;

logical_or          = logical_and { "||" logical_and } ;
logical_and         = bitwise_or { "&&" bitwise_or } ;
bitwise_or          = bitwise_xor { "|" bitwise_xor } ;
bitwise_xor         = bitwise_and { "^" bitwise_and } ;
bitwise_and         = equality { "&" equality } ;
equality            = relational { ( "==" | "!=" ) relational } ;
relational          = shift { ( "<" | "<=" | ">" | ">=" ) shift } ;
shift               = additive { ( "<<" | ">>" ) additive } ;
additive            = multiplicative { ( "+" | "-" ) multiplicative } ;
multiplicative      = exponentiation { ( "*" | "/" | "%" ) exponentiation } ;
exponentiation      = unary [ "**" exponentiation ] ;
unary               = ( "-" | "!" | "~" ) unary | postfix ;
postfix             = primary { member_access | index_access | call_expr } ;

member_access       = "." IDENTIFIER ;
index_access        = "[" expression "]" ;
call_expr           = "(" [ expression { "," expression } ] ")" ;

primary             = INT_LITERAL
                    | FLOAT_LITERAL
                    | STRING_LITERAL
                    | "true" | "false"
                    | IDENTIFIER
                    | "(" expression ")"
                    | array_literal
                    | data_literal
                    | live_deg_expr ;

live_deg_expr       = "live" "." "deg" "(" type_spec "," expression "," expression ")" ;
array_literal       = "[" [ expression { "," expression } ] "]" ;
data_literal        = "{" [ data_entry { "," data_entry } ] "}" ;
data_entry          = ( IDENTIFIER | STRING_LITERAL ) ":" expression ;
```
