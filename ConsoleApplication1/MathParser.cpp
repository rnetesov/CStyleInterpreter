#include <iostream>  
#include "CStyleInterpreter.h"
#include <cctype>
#include <cmath>
#include <stdexcept>

// Полноценный локальный интерпретатор тела функции с поддержкой вложенных циклов for и while
double CStyleInterpreter::parseUserFunctionCall(const std::string& name) {
    if (functions.find(name) == functions.end()) {
        throw std::runtime_error("Unknown function: " + name);
    }

    next(); // Пропускаем '('

    // 1. Поочередно считываем все переданные аргументы через запятую
    std::vector<double> arguments;
    if (current() != ')') {
        while (true) {
            arguments.push_back(parseExpression());
            skipSpaces();
            if (current() == ',') {
                next(); // Пропускаем ',' и идем к следующему аргументу
            }
            else if (current() == ')') {
                break;
            }
            else {
                throw std::runtime_error("Expected ',' or ')' in function argument list");
            }
        }
    }

    if (current() != ')') throw std::runtime_error("Expected ')' at the end of function call");
    next(); // Пропускаем ')'

    UserFunction func = functions[name];

    // Проверяем совпадение количества переданных аргументов с объявленными параметрами
    if (arguments.size() != func.paramNames.size()) {
        throw std::runtime_error("Function '" + name + "' expected " +
            std::to_string(func.paramNames.size()) + " arguments, but got " +
            std::to_string(arguments.size()));
    }

    // Создаем изолированную локальную область видимости
    scopeStack.push_back({});
    returnStack.push_back(ip);

    // 2. Записываем ВСЕ аргументы в локальный слой под их именами
    for (size_t i = 0; i < func.paramNames.size(); ++i) {
        scopeStack.back()[func.paramNames[i]] = arguments[i];
    }

    size_t old_ip = ip;
    ip = func.bodyLineIdx;
    std::string savedLine = currentLine;
    size_t savedIndex = index;

    lastReturnValue = 0.0;

    std::vector<size_t> localWhileLoopStack;
    std::vector<bool> localIsForLoopStack;

    // Вычисляем точный конец функции по скобкам
    size_t scanIp = func.bodyLineIdx;
    int bracketDepth = 0;
    size_t functionEndLineIdx = lines.size();

    // Handle '{' on its own line (Allman brace style)
    if (scanIp < lines.size() && lines[scanIp].find("{") != std::string::npos
        && lines[scanIp].find("}") == std::string::npos) {
        scanIp++;
    }

    while (scanIp < lines.size()) {
        std::string l = lines[scanIp];
        if (l.find("{") != std::string::npos) bracketDepth++;
        if (l.find("}") != std::string::npos) {
            if (bracketDepth == 0) {
                functionEndLineIdx = scanIp;
                break;
            }
            bracketDepth--;
        }
        scanIp++;
    }

    if (functionEndLineIdx == lines.size()) {
        throw std::runtime_error("Function '" + name + "' has no closing '}'");
    }

    while (ip <= functionEndLineIdx && ip < lines.size()) {
        std::string line = lines[ip];

        if (line.find("while") != std::string::npos && line.find("#") == std::string::npos) {
            localWhileLoopStack.push_back(ip);
            localIsForLoopStack.push_back(false);
        }
        if (line.find("for") != std::string::npos && line.find("#") == std::string::npos) {
            localWhileLoopStack.push_back(ip);
            localIsForLoopStack.push_back(true);
        }

        size_t currentExecutingLineIdx = ip;
        ip++;

        executeLine(currentExecutingLineIdx);

        if (line.find("return") != std::string::npos && line.find("#") == std::string::npos) {
            break;
        }

        if (line.find("}") != std::string::npos && !localWhileLoopStack.empty()) {
            size_t startLoop = localWhileLoopStack.back();
            bool isFor = localIsForLoopStack.back();
            std::string loopLine = lines[startLoop];

            if (isFor && !forStepStack.empty()) {
                executeSingleLineFromString(forStepStack.back());
            }

            bool cond = false;
            if (isFor) {
                size_t firstSemi = loopLine.find(";");
                size_t secondSemi = loopLine.find(";", firstSemi + 1);
                std::string condPart = "(" + loopLine.substr(firstSemi + 1, secondSemi - firstSemi - 1) + ")";
                cond = parseConditionFromString("while " + condPart);
            }
            else {
                cond = parseConditionFromString(loopLine);
            }

            if (cond) {
                ip = startLoop + 1;
            }
            else {
                localWhileLoopStack.pop_back();
                localIsForLoopStack.pop_back();
                if (isFor && !forStepStack.empty()) {
                    forStepStack.pop_back();
                }
            }
        }
    }

    currentLine = savedLine;
    index = savedIndex;
    ip = returnStack.back();
    returnStack.pop_back();

    scopeStack.pop_back();
    return lastReturnValue;
}

double CStyleInterpreter::parseFactor() {
    skipSpaces();
    char c = current();

    if (std::isalpha(c) || c == '_') {
        std::string name;
        while (std::isalnum(current()) || current() == '_') {
            name += current();
            index++;
        }
        skipSpaces();

        if (name == "strlen")      return builtin_strlen();
        if (name == "empty")       return builtin_empty();
        if (name == "is_string")   return builtin_is_string();
        if (name == "str_pos")     return builtin_str_pos();

        if (current() == '(') {
            if (name == "sin") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return builtin_sin(arg); }
            if (name == "cos") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return builtin_cos(arg); }
            if (name == "sqrt") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return builtin_sqrt(arg); }
            if (name == "rand") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return builtin_rand(arg); }
            if (name == "array") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return arg; }

            if (name == "round") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return builtin_round(arg); }
            if (name == "floor") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return builtin_floor(arg); }
            if (name == "ceil") { next(); double arg = parseExpression(); if (current() != ')') throw std::runtime_error("Expected ')'"); next(); return builtin_ceil(arg); }

            return parseUserFunctionCall(name);
        }

        if (current() == '[') {
            next();
            double idxVal = parseExpression();
            if (current() != ']') throw std::runtime_error("Expected ']' after array index");
            next();

            if (varExists(name)) {
                auto& var = getVarRef(name);
                if (std::holds_alternative<std::vector<double>>(var)) {
                    const auto& arr = std::get<std::vector<double>>(var);
                    size_t i = static_cast<size_t>(idxVal);
                    if (i >= arr.size()) throw std::runtime_error("Array index out of bounds");
                    return arr[i];
                }
                throw std::runtime_error("Variable '" + name + "' is not an array");
            }
            throw std::runtime_error("Unknown variable: " + name);
        }

        if (varExists(name)) {
            auto& var = getVarRef(name);
            if (std::holds_alternative<double>(var)) {
                return std::get<double>(var);
            }
            throw std::runtime_error("Cannot use non-numeric variable '" + name + "' in math expression");
        }

        throw std::runtime_error("Unknown variable: " + name);
    }

    if (c == '(') {
        next();
        double result = parseExpression();
        if (current() != ')') throw std::runtime_error("Expected ')'");
        next();
        return result;
    }

    if (c == '-') {
        next();
        return -parseFactor();
    }

    if (std::isdigit(c) || c == '.') {
        std::string numberStr;
        while (std::isdigit(current()) || current() == '.') {
            numberStr += current();
            index++;
        }
        skipSpaces();
        return std::stod(numberStr);
    }

    if (c == ';' || c == '\0') {
        return 0;
    }

    throw std::runtime_error("Unexpected character");
}

double CStyleInterpreter::parsePower() {
    double result = parseFactor();
    if (current() == '^') {
        next();
        double exponent = parsePower();
        result = std::pow(result, exponent);
    }
    return result;
}

double CStyleInterpreter::parseTerm() {
    double result = parsePower();
    while (current() == '*' || current() == '/') {
        char op = current();
        next();
        double nextPower = parsePower();
        if (op == '*') result *= nextPower;
        else {
            if (nextPower == 0.0) throw std::runtime_error("Division by zero");
            result /= nextPower;
        }
    }
    return result;
}

double CStyleInterpreter::parseExpression() {
    double result = parseTerm();
    while (current() == '+' || current() == '-') {
        char op = current();
        next();
        double nextTerm = parseTerm();
        if (op == '+') result += nextTerm;
        else result -= nextTerm;
    }
    return result;
}
