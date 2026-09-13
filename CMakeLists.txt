cmake_minimum_required(VERSION 3.15)
project(jaguar VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Runtime Support Library
set(RUNTIME_SRCS
    src/runtime/runtime.c
    src/filesystem/fs.c
    src/runtime/net.c
)

add_library(jaguar_runtime STATIC ${RUNTIME_SRCS})

# Compiler Core Library
set(COMPILER_SRCS
    src/lexer/lexer.c
    src/parser/parser.c
    src/ast/ast.c
    src/types/types.c
    src/semantic/symbol.c
    src/semantic/diagnostics.c
    src/semantic/semantic.c
    src/ir/ir.c
    src/optimizer/optimizer.c
    src/codegen/codegen.c
    src/codegen/driver.c
    src/live/supervisor.c
    src/live/live.c
    src/platform/watch.c
)

add_library(jaguar_core STATIC ${COMPILER_SRCS})
target_link_libraries(jaguar_core PUBLIC jaguar_runtime)

# Main Executable
add_executable(jag src/cli/main.c)
target_link_libraries(jag PRIVATE jaguar_core)

# Testing
enable_testing()

add_executable(test_lexer tests/test_lexer.c)
target_link_libraries(test_lexer PRIVATE jaguar_core)
add_test(NAME test_lexer COMMAND test_lexer)

add_executable(test_parser tests/test_parser.c)
target_link_libraries(test_parser PRIVATE jaguar_core)
add_test(NAME test_parser COMMAND test_parser)

add_executable(test_semantic tests/test_semantic.c)
target_link_libraries(test_semantic PRIVATE jaguar_core)
add_test(NAME test_semantic COMMAND test_semantic)

add_executable(test_ir tests/test_ir.c)
target_link_libraries(test_ir PRIVATE jaguar_core)
add_test(NAME test_ir COMMAND test_ir)

add_executable(test_codegen tests/test_codegen.c)
target_link_libraries(test_codegen PRIVATE jaguar_core)
add_test(NAME test_codegen COMMAND test_codegen)

add_executable(test_runtime tests/test_runtime.c)
target_link_libraries(test_runtime PRIVATE jaguar_runtime)
add_test(NAME test_runtime COMMAND test_runtime)

add_executable(test_live tests/test_live.c)
target_link_libraries(test_live PRIVATE jaguar_core)
add_test(NAME test_live COMMAND test_live)

include(GNUInstallDirs)
install(TARGETS jag DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS jaguar_runtime DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/jag DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_executable(test_cli tests/test_cli.c)
add_test(NAME test_cli COMMAND test_cli)
