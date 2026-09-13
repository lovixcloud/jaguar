# Jaguar Compiler Architecture

The Jaguar compiler pipeline consists of:
1. Lexer (`src/lexer/lexer.c`)
2. Parser & AST (`src/parser/parser.c`, `src/ast/ast.c`)
3. Scoped Symbol Table & Type Checker (`src/semantic/`)
4. Intermediate Representation & Optimizer (`src/ir/`, `src/optimizer/`)
5. Native C Code Generator & Driver (`src/codegen/`)
6. Process Supervisor Live Engine (`src/live/`)
