#include "CStyleInterpreter.h"
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <climits>

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

std::string CStyleInterpreter::parseIdentifier() {
    std::string name;
    while (std::isalnum(current()) || current() == '_') {
        name += current();
        index++;
    }
    skipSpaces();
    return name;
}

std::string CStyleInterpreter::parseStringLiteral() {
    if (current() != '"') throw std::runtime_error("Expected opening double quote");
    index++;
    std::string lit;
    while (index < currentLine.length() && currentLine[index] != '"') {
        lit += currentLine[index];
        index++;
    }
    if (index >= currentLine.length()) throw std::runtime_error("Missing closing double quote");
    index++;
    skipSpaces();
    return lit;
}

double CStyleInterpreter::callBuiltinMathFunc(double (CStyleInterpreter::*func)(double)) {
    next();
    double arg = parseExpression();
    if (current() != ')') throw std::runtime_error("Expected ')'");
    next();
    return (this->*func)(arg);
}

bool CStyleInterpreter::parseCondition() {
    if (current() != '(') throw std::runtime_error("Expected '(' for condition");
    next();

    double left = parseExpression();
    skipSpaces();

    std::string op;
    while (current() == '<' || current() == '>' || current() == '=' || current() == '!') {
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
    if (op == "!=") return left != right;
    if (op == "<=") return left <= right;
    if (op == ">=") return left >= right;

    throw std::runtime_error("Unknown comparison operator: " + op);
}

bool CStyleInterpreter::parseConditionFromString(const std::string& condLine) {
    ParserStateGuard guard(*this);

    currentLine = condLine;

    size_t openParenIdx = condLine.find("(");
    if (openParenIdx == std::string::npos) {
        throw std::runtime_error("Expected '(' in condition: " + condLine);
    }
    index = openParenIdx;

    skipSpaces();
    return parseCondition();
}

void CStyleInterpreter::skipBlock() {
    // Handle '{' on its own line (Allman brace style):
    // only skip if the line contains nothing but whitespace and '{'
    if (ip < lines.size()) {
        const std::string& firstLine = lines[ip];
        bool isStandaloneBrace = false;
        for (char c : firstLine) {
            if (c == '{') { isStandaloneBrace = true; break; }
            else if (!std::isspace(c)) break;
        }
        if (isStandaloneBrace) ip++;
    }
    int depth = 0;
    while (ip < lines.size()) {
        std::string l = lines[ip];
        ip++;
        if (l.find("}") != std::string::npos) {
            if (depth == 0) return;
            depth--;
        }
        if (l.find("{") != std::string::npos) depth++;
    }
    throw std::runtime_error("Unmatched '{': reached end of source without finding closing '}'");
}

void CStyleInterpreter::executeSingleLineFromString(const std::string& lineText) {
    ParserStateGuard guard(*this);

    currentLine = lineText;
    index = 0;
    skipSpaces();

    std::string firstWord;
    if (std::isalpha(current()) || current() == '_') {
        firstWord = parseIdentifier();
    }

    if (!firstWord.empty()) {
        processVariableAssignment(firstWord);
    }
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

void CStyleInterpreter::executeBlock(size_t endLineIdx, bool breakOnReturn) {
    std::vector<size_t> loopStack;
    std::vector<bool> isForStack;

    while (ip <= endLineIdx && ip < lines.size()) {
        std::string line = lines[ip];

        if (line.find("while") != std::string::npos && line.find("#") == std::string::npos) {
            loopStack.push_back(ip);
            isForStack.push_back(false);
        }
        if (line.find("for") != std::string::npos && line.find("#") == std::string::npos) {
            loopStack.push_back(ip);
            isForStack.push_back(true);
        }

        ip++;
        executeLine(ip - 1);

        if (breakOnReturn && line.find("return") != std::string::npos && line.find("#") == std::string::npos) {
            break;
        }

        // Handle break: skip to end of current loop block
        if (breakFlag && !loopStack.empty()) {
            breakFlag = false;
            size_t startLoop = loopStack.back();
            bool isFor = isForStack.back();
            // Skip forward to find the closing '}' of the loop
            int depth = 0;
            while (ip < lines.size()) {
                std::string l = lines[ip];
                ip++;
                if (l.find("{") != std::string::npos) depth++;
                if (l.find("}") != std::string::npos) {
                    if (depth == 0) break;
                    depth--;
                }
            }
            loopStack.pop_back();
            isForStack.pop_back();
            if (isFor && !forStepStack.empty()) {
                forStepStack.pop_back();
            }
            continue;
        }

        // Handle continue: jump to loop re-evaluation
        if (continueFlag && !loopStack.empty()) {
            continueFlag = false;
            size_t startLoop = loopStack.back();
            bool isFor = isForStack.back();
            std::string loopLine = lines[startLoop];

            // For 'for' loops, execute the step expression first
            if (isFor && !forStepStack.empty()) {
                executeSingleLineFromString(forStepStack.back());
            }

            // Re-evaluate the loop condition
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
                // Skip to body start (right after the loop header)
                ip = startLoop + 1;
            }
            else {
                // Condition false, exit the loop
                // Skip forward to closing '}'
                int depth = 0;
                while (ip < lines.size()) {
                    std::string l = lines[ip];
                    ip++;
                    if (l.find("{") != std::string::npos) depth++;
                    if (l.find("}") != std::string::npos) {
                        if (depth == 0) break;
                        depth--;
                    }
                }
                loopStack.pop_back();
                isForStack.pop_back();
                if (isFor && !forStepStack.empty()) {
                    forStepStack.pop_back();
                }
            }
            continue;
        }

        if (line.find("}") != std::string::npos && !loopStack.empty()) {
            size_t startLoop = loopStack.back();
            bool isFor = isForStack.back();
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
                loopStack.pop_back();
                isForStack.pop_back();
                if (isFor && !forStepStack.empty()) {
                    forStepStack.pop_back();
                }
            }
        }
    }
}

void CStyleInterpreter::run(const std::vector<std::string>& sourceLines) {
    lines = sourceLines;
    ip = 0;
    executeBlock(SIZE_MAX, false);
}
