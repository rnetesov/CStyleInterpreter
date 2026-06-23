#pragma once

#include <stdarg.h>  
#include <cstdio>    
#include <string>
#include <vector>
#include <map>
#include <variant>

// Модернизированная структура для поддержки списка параметров
struct UserFunction {
    std::vector<std::string> paramNames; // ИСПРАВЛЕНИЕ: Теперь храним список всех параметров!
    size_t bodyLineIdx = 0;
};

class CStyleInterpreter {
private:
    // --- Данные инфраструктуры интерпретатора (CStyleInterpreter.cpp) ---
    std::vector<std::string> lines;
    size_t ip = 0;

    // Стек областей видимости (Scope Stack)
    std::vector<std::map<std::string, std::variant<double, std::string, std::vector<double>>>> scopeStack;

    std::map<std::string, UserFunction> functions;
    std::vector<size_t> returnStack;
    std::vector<std::string> forStepStack;

    double lastReturnValue = 0.0;
    std::string currentLine;
    size_t index = 0;
    bool breakFlag = false;
    bool continueFlag = false;
    std::vector<size_t> originalLineNumbers;

    // --- RAII-охранник состояния парсера ---
    struct ParserStateGuard {
        CStyleInterpreter& self;
        std::string savedLine;
        size_t savedIndex;
        ParserStateGuard(CStyleInterpreter& interp)
            : self(interp), savedLine(interp.currentLine), savedIndex(interp.index) {}
        ~ParserStateGuard() { self.currentLine = savedLine; self.index = savedIndex; }
        ParserStateGuard(const ParserStateGuard&) = delete;
        ParserStateGuard& operator=(const ParserStateGuard&) = delete;
    };

    // --- Общие утилиты парсинга (CStyleInterpreter.cpp) ---
    std::string parseIdentifier();
    std::string parseStringLiteral();
    double callBuiltinMathFunc(double (CStyleInterpreter::*func)(double));
    void executeBlock(size_t endLineIdx, bool breakOnReturn);
    static void expandInlineBlocks(std::vector<std::string>& src, std::vector<size_t>& lineMap);

    // --- Утилиты сканирования строки (CStyleInterpreter.cpp) ---
    char current() const;
    void next();
    void skipSpaces();
    bool parseComparison();
    bool parseLogicalAnd();
    bool parseLogicalOr();
    bool parseCondition();
    void skipBlock();
    bool parseConditionFromString(const std::string& condLine);

    // --- Математическое ядро (MathParser.cpp) ---
    double parseFactor();
    double parsePower();
    double parseTerm();
    double parseExpression();
    double parseUserFunctionCall(const std::string& name);

    // --- Командный менеджер (InterpreterCommands.cpp) ---
    void executeLine(size_t lineIdx);
    void executeSingleLineFromString(const std::string& lineText);

    // --- Менеджер операций над переменными (VariableOperations.cpp) ---
    void processVariableAssignment(const std::string& varName);

    // --- Вспомогательные методы работы с областями видимости ---
    bool varExists(const std::string& name);
    std::variant<double, std::string, std::vector<double>>& getVarRef(const std::string& name);
    void setVariable(const std::string& name, const std::variant<double, std::string, std::vector<double>>& value);

    // --- Встроенные строковые функции (StringFunctions.cpp) ---
    std::string parseVarArg();
    std::string parseStringLiteralArg();
    double builtin_strlen();
    double builtin_empty();
    double builtin_is_string();
    double builtin_str_pos();
    double builtin_str_replace();
    double builtin_trim();

    // --- Встроенные математические функции (MathFunctions.cpp) ---
    double builtin_sin(double arg);
    double builtin_cos(double arg);
    double builtin_sqrt(double arg);
    double builtin_rand(double arg);
    double builtin_round(double arg);
    double builtin_floor(double arg);
    double builtin_ceil(double arg);

public:
    CStyleInterpreter();
    void run(const std::vector<std::string>& sourceLines);
};
