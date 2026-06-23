cmake_minimum_required(VERSION 3.14)
project(CStyleInterpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Interpreter library (all source files except main.cpp)
add_library(interpreter_lib
    ConsoleApplication1/CStyleInterpreter.cpp
    ConsoleApplication1/MathParser.cpp
    ConsoleApplication1/MathFunctions.cpp
    ConsoleApplication1/StringFunctions.cpp
    ConsoleApplication1/VariableOperations.cpp
    ConsoleApplication1/InterpreterCommands.cpp
)
target_include_directories(interpreter_lib PUBLIC ConsoleApplication1)

# Tests
enable_testing()
find_package(GTest REQUIRED)

add_executable(interpreter_tests
    tests/test_math_functions.cpp
    tests/test_math_parser.cpp
    tests/test_string_functions.cpp
    tests/test_variable_operations.cpp
    tests/test_interpreter_commands.cpp
    tests/test_core.cpp
)
target_link_libraries(interpreter_tests interpreter_lib GTest::GTest GTest::Main pthread)
target_include_directories(interpreter_tests PRIVATE ConsoleApplication1 tests)

add_test(NAME InterpreterTests COMMAND interpreter_tests)
