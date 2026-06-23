#include "CStyleInterpreter.h"
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <cstdlib>   
#include <ctime>     

CStyleInterpreter::CStyleInterpreter() {
    scopeStack.push_back({}); // Глобальный уровень видимости
    setVariable("pi", 3.141592653589793);
    setVariable("e", 2.718281828459045);
    srand(static_cast<unsigned int>(time(0)));
}

char CStyleInterpreter::current() const {
    if (index < currentLine.length()) return currentLine[index];
    return '\0';
}

void CStyleInterpreter::next() {
    if (index < currentLine.length()) index++;
    skipSpaces();
}

void CStyleInterpreter::skipSpaces() {
    while (index < currentLine.length() && std::isspace(currentLine[index])) {
        index++;
    }
}

bool CStyleInterpreter::parseCondition() {
    if (current() != '(') throw std::runtime_error("Expected '(' for condition");
    next();

    double left = parseExpression();
    skipSpaces();

    std::string op;
    while (current() == '<' || current() == '>' || current() == '=') {
        op += current();
        index++;
    }
    skipSpaces();

    double right = parseExpression();

    if (current() != ')') throw std::runtime_error("Expected ')' after condition");
    next();

    if (op == "<") return left < right;
    if (op == ">") return left > right;
    if (op == "==") return left == right;
    if (op == "<=") return left <= right;
    if (op == ">=") return left >= right;

    throw std::runtime_error("Unknown comparison operator: " + op);
}

bool CStyleInterpreter::parseConditionFromString(const std::string& condLine) {
    // Полностью изолируем текстовый контекст класса
    std::string savedLine = currentLine;
    size_t savedIndex = index;

    currentLine = condLine;

    // ИСПРАВЛЕНИЕ: Находим точное начало скобки условия (, игнорируя слова while/if
    size_t openParenIdx = condLine.find("(");
    if (openParenIdx == std::string::npos) {
        currentLine = savedLine;
        index = savedIndex;
        throw std::runtime_error("Expected '(' in condition: " + condLine);
    }
    index = openParenIdx;

    skipSpaces();
    // Вычисляем условие в изолированном текстовом буфере
    bool result = parseCondition();

    // Восстанавливаем глобальный контекст обратно
    currentLine = savedLine;
    index = savedIndex;

    return result;
}

void CStyleInterpreter::skipBlock() {
    int depth = 0;
    while (ip < lines.size()) {
        std::string l = lines[ip];
        ip++;
        if (l.find("{") != std::string::npos) depth++;
        if (l.find("}") != std::string::npos) {
            if (depth == 0) return;
            depth--;
        }
    }
    throw std::runtime_error("Unmatched '{': reached end of source without finding closing '}'");
}

void CStyleInterpreter::executeSingleLineFromString(const std::string& lineText) {
    std::string savedLine = currentLine;
    size_t savedIndex = index;

    currentLine = lineText;
    index = 0;
    skipSpaces();

    std::string firstWord;
    if (std::isalpha(current()) || current() == '_') {
        while (std::isalnum(current()) || current() == '_') {
            firstWord += current();
            index++;
        }
    }
    skipSpaces();

    if (!firstWord.empty()) {
        processVariableAssignment(firstWord);
    }

    currentLine = savedLine;
    index = savedIndex;
}

bool CStyleInterpreter::varExists(const std::string& name) {
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        if (it->find(name) != it->end()) return true;
    }
    return false;
}

std::variant<double, std::string, std::vector<double>>& CStyleInterpreter::getVarRef(const std::string& name) {
    // Бежим снизу вверх по стеку и возвращаем ПРЯМУЮ ссылку на ячейку карты
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        if (it->find(name) != it->end()) {
            return (*it)[name];
        }
    }
    throw std::runtime_error("Variable not found: " + name);
}

void CStyleInterpreter::setVariable(const std::string& name, const std::variant<double, std::string, std::vector<double>>& value) {
    // ЖЕЛЕЗОБЕТОННАЯ ЛОКАЛИЗАЦИЯ:
    // Запись ВСЕГДА идет только в текущую область видимости (самый верхний кадр стека).
    // Это полностью защищает функцию от перезаписи глобальных переменных и гарантирует,
    // что переменная 'i' зафиксируется внутри функции my_pow!
    scopeStack.back()[name] = value;
}

void CStyleInterpreter::run(const std::vector<std::string>& sourceLines) {
    lines = sourceLines;
    ip = 0;
    std::vector<size_t> whileLoopStack;
    std::vector<bool> isForLoopStack;

    while (ip < lines.size()) {
        std::string line = lines[ip];
        if (line.find("while") != std::string::npos && line.find("#") == std::string::npos) {
            whileLoopStack.push_back(ip);
            isForLoopStack.push_back(false);
        }
        if (line.find("for") != std::string::npos && line.find("#") == std::string::npos) {
            whileLoopStack.push_back(ip);
            isForLoopStack.push_back(true);
        }
        ip++;

        executeLine(ip - 1);

        if (line.find("}") != std::string::npos && !whileLoopStack.empty()) {
            size_t startLoop = whileLoopStack.back();
            bool isFor = isForLoopStack.back();
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
                whileLoopStack.pop_back();
                isForLoopStack.pop_back();
                if (isFor && !forStepStack.empty()) {
                    forStepStack.pop_back();
                }
            }
        }
    }
}
