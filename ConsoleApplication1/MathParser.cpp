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

    next();

    // Поочередно считываем все переданные аргументы через запятую
    std::vector<double> arguments;
    if (current() != ')') {
        while (true) {
            arguments.push_back(parseExpression());
            skipSpaces();
            if (current() == ',') {
                next();
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
    next();

    UserFunction func = functions[name];

    if (arguments.size() != func.paramNames.size()) {
        throw std::runtime_error("Function '" + name + "' expected " +
            std::to_string(func.paramNames.size()) + " arguments, but got " +
            std::to_string(arguments.size()));
    }

    scopeStack.push_back({});
    returnStack.push_back(ip);

    for (size_t i = 0; i < func.paramNames.size(); ++i) {
        scopeStack.back()[func.paramNames[i]] = arguments[i];
    }

    ip = func.bodyLineIdx;
    ParserStateGuard guard(*this);

    lastReturnValue = 0.0;

    size_t scanIp = func.bodyLineIdx;
    int bracketDepth = 0;
    size_t functionEndLineIdx = lines.size();

    // Handle '{' on its own line (Allman brace style):
    // only skip if the line contains nothing but whitespace and '{'
    if (scanIp < lines.size()) {
        bool isStandaloneBrace = false;
        for (char c : lines[scanIp]) {
            if (c == '{') { isStandaloneBrace = true; break; }
            else if (!std::isspace(c)) break;
        }
        if (isStandaloneBrace) scanIp++;
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

    executeBlock(functionEndLineIdx, true);

    ip = returnStack.back();
    returnStack.pop_back();

    scopeStack.pop_back();
    return lastReturnValue;
}

double CStyleInterpreter::parseFactor() {
    skipSpaces();
    char c = current();

    if (std::isalpha(c) || c == '_') {
        std::string name = parseIdentifier();

        if (name == "strlen")      return builtin_strlen();
        if (name == "empty")       return builtin_empty();
        if (name == "is_string")   return builtin_is_string();
        if (name == "str_pos")     return builtin_str_pos();

        if (current() == '(') {
            if (name == "sin")   return callBuiltinMathFunc(&CStyleInterpreter::builtin_sin);
            if (name == "cos")   return callBuiltinMathFunc(&CStyleInterpreter::builtin_cos);
            if (name == "sqrt")  return callBuiltinMathFunc(&CStyleInterpreter::builtin_sqrt);
            if (name == "rand")  return callBuiltinMathFunc(&CStyleInterpreter::builtin_rand);
            if (name == "round") return callBuiltinMathFunc(&CStyleInterpreter::builtin_round);
            if (name == "floor") return callBuiltinMathFunc(&CStyleInterpreter::builtin_floor);
            if (name == "ceil")  return callBuiltinMathFunc(&CStyleInterpreter::builtin_ceil);

            if (name == "array") {
                next();
                double arg = parseExpression();
                if (current() != ')') throw std::runtime_error("Expected ')'");
                next();
                return arg;
            }

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
                    if (idxVal < 0)
                        throw std::runtime_error("Negative array index");
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
