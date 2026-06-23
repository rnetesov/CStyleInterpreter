#include "CStyleInterpreter.h"
#include <stdexcept>
#include <vector>

// Чтение имени переменной-аргумента из скобок функции
std::string CStyleInterpreter::parseVarArg() {
    skipSpaces();
    if (current() != '(') throw std::runtime_error("Expected '(' for function argument");
    next(); // Пропускаем '('

    skipSpaces();
    std::string varName;
    while (std::isalnum(current()) || current() == '_') {
        varName += current();
        index++;
    }
    skipSpaces();
    return varName;
}

// Вспомогательный метод для чтения строки в кавычках из аргументов функций
std::string CStyleInterpreter::parseStringLiteralArg() {
    skipSpaces();
    if (current() != '"') throw std::runtime_error("Expected opening double quote for string argument");
    next(); // Пропускаем '"'

    std::string lit;
    while (index < currentLine.length() && currentLine[index] != '"') {
        lit += currentLine[index];
        index++;
    }
    if (index >= currentLine.length()) throw std::runtime_error("Missing closing double quote in string argument");
    next(); // Пропускаем '"'
    skipSpaces();
    return lit;
}

// Реализация функции strlen() через области видимости
double CStyleInterpreter::builtin_strlen() {
    std::string varName = parseVarArg();
    if (current() != ')') throw std::runtime_error("Expected ')' after strlen argument");
    next(); // Пропускаем ')'
    skipSpaces();

    if (!varExists(varName)) {
        throw std::runtime_error("strlen: variable '" + varName + "' is not defined");
    }

    auto& var = getVarRef(varName);
    if (std::holds_alternative<std::string>(var)) {
        return static_cast<double>(std::get<std::string>(var).length());
    }
    if (std::holds_alternative<double>(var)) {
        return static_cast<double>(std::to_string(std::get<double>(var)).length());
    }
    if (std::holds_alternative<std::vector<double>>(var)) {
        return static_cast<double>(std::get<std::vector<double>>(var).size());
    }
    throw std::runtime_error("strlen: unsupported variable type for '" + varName + "'");
}

// Реализация функции empty() через области видимости
double CStyleInterpreter::builtin_empty() {
    std::string varName = parseVarArg();
    if (current() != ')') throw std::runtime_error("Expected ')' after empty argument");
    next(); // Пропускаем ')'
    skipSpaces();

    if (!varExists(varName)) {
        throw std::runtime_error("empty: variable '" + varName + "' is not defined");
    }

    auto& var = getVarRef(varName);
    if (std::holds_alternative<std::string>(var)) {
        return std::get<std::string>(var).empty() ? 1.0 : 0.0;
    }
    if (std::holds_alternative<double>(var)) {
        return std::get<double>(var) == 0.0 ? 1.0 : 0.0;
    }
    if (std::holds_alternative<std::vector<double>>(var)) {
        return std::get<std::vector<double>>(var).empty() ? 1.0 : 0.0;
    }
    throw std::runtime_error("empty: unsupported variable type for '" + varName + "'");
}

// Реализация функции is_string() через области видимости
double CStyleInterpreter::builtin_is_string() {
    std::string varName = parseVarArg();
    if (current() != ')') throw std::runtime_error("Expected ')' after is_string argument");
    next(); // Пропускаем ')'
    skipSpaces();

    if (!varExists(varName)) {
        throw std::runtime_error("is_string: variable '" + varName + "' is not defined");
    }
    return std::holds_alternative<std::string>(getVarRef(varName)) ? 1.0 : 0.0;
}

// Реализация функции str_pos(var, "search") через области видимости
double CStyleInterpreter::builtin_str_pos() {
    std::string varName = parseVarArg();
    if (current() != ',') throw std::runtime_error("Expected ',' between str_pos arguments");
    next(); // Пропускаем ','

    std::string searchStr = parseStringLiteralArg();
    if (current() != ')') throw std::runtime_error("Expected ')' after str_pos arguments");
    next(); // Пропускаем ')'
    skipSpaces();

    if (!varExists(varName) || !std::holds_alternative<std::string>(getVarRef(varName))) {
        throw std::runtime_error("First argument of str_pos must be a valid string variable");
    }

    std::string text = std::get<std::string>(getVarRef(varName));
    size_t pos = text.find(searchStr);

    if (pos == std::string::npos) return -1.0;
    return static_cast<double>(pos);
}

// Реализация функции str_replace(var, "old", "new") через области видимости
double CStyleInterpreter::builtin_str_replace() {
    std::string varName = parseVarArg();
    if (current() != ',') throw std::runtime_error("Expected ',' in str_replace");
    next(); // Пропускаем ','

    std::string oldStr = parseStringLiteralArg();
    if (current() != ',') throw std::runtime_error("Expected second ',' in str_replace");
    next(); // Пропускаем ','

    std::string newStr = parseStringLiteralArg();
    if (current() != ')') throw std::runtime_error("Expected ')' at the end of str_replace");
    next(); // Пропускаем ')'
    skipSpaces();

    if (!varExists(varName) || !std::holds_alternative<std::string>(getVarRef(varName))) {
        throw std::runtime_error("First argument of str_replace must be a valid string variable");
    }

    std::string text = std::get<std::string>(getVarRef(varName));
    size_t pos = 0;
    while ((pos = text.find(oldStr, pos)) != std::string::npos) {
        text.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }

    getVarRef(varName) = text; // Прямое обновление в нужной области видимости
    return 0.0;
}

// Реализация функции trim(var) через области видимости
double CStyleInterpreter::builtin_trim() {
    std::string varName = parseVarArg();
    if (current() != ')') throw std::runtime_error("Expected ')' after trim argument");
    next(); // Пропускаем ')'
    skipSpaces();

    if (!varExists(varName) || !std::holds_alternative<std::string>(getVarRef(varName))) {
        throw std::runtime_error("Argument of trim must be a valid string variable");
    }

    std::string text = std::get<std::string>(getVarRef(varName));

    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        text = "";
    }
    else {
        size_t end = text.find_last_not_of(" \t\r\n");
        text = text.substr(start, end - start + 1);
    }

    getVarRef(varName) = text; // Прямое обновление в нужной области видимости
    return 0.0;
}
