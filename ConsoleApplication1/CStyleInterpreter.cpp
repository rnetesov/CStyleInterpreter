#include "CStyleInterpreter.h"
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <climits>

// Check if keyword appears as a whole word (not part of another identifier)
static bool containsKeyword(const std::string& line, const std::string& keyword) {
    size_t pos = 0;
    while ((pos = line.find(keyword, pos)) != std::string::npos) {
        bool leftOk = (pos == 0) || !std::isalnum(line[pos - 1]) && line[pos - 1] != '_';
        size_t end = pos + keyword.length();
        bool rightOk = (end >= line.length()) || !std::isalnum(line[end]) && line[end] != '_';
        if (leftOk && rightOk) return true;
        pos++;
    }
    return false;
}

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

bool CStyleInterpreter::parseComparison() {
    // Handle ! prefix (logical NOT)
    if (current() == '!') {
        index++;
        skipSpaces();
        if (current() == '(') {
            next();
            bool val = parseLogicalOr();
            if (current() != ')') throw std::runtime_error("Expected ')' after '!'");
            next();
            return !val;
        }
        throw std::runtime_error("Expected '(' after '!'");
    }

    // Handle nested parenthesized sub-condition
    if (current() == '(') {
        next();
        bool val = parseLogicalOr();
        if (current() != ')') throw std::runtime_error("Expected ')' for nested condition");
        next();
        return val;
    }

    // Regular comparison: expr op expr
    double left = parseExpression();
    skipSpaces();

    std::string op;
    while (current() == '<' || current() == '>' || current() == '=' || current() == '!') {
        op += current();
        index++;
    }
    skipSpaces();

    double right = parseExpression();
    skipSpaces();

    if (op == "<") return left < right;
    if (op == ">") return left > right;
    if (op == "==") return left == right;
    if (op == "!=") return left != right;
    if (op == "<=") return left <= right;
    if (op == ">=") return left >= right;

    throw std::runtime_error("Unknown comparison operator: " + op);
}

bool CStyleInterpreter::parseLogicalAnd() {
    bool result = parseComparison();
    skipSpaces();
    while (current() == '&' && index + 1 < currentLine.length() && currentLine[index + 1] == '&') {
        index += 2;
        skipSpaces();
        bool right = parseComparison();
        result = result && right;
        skipSpaces();
    }
    return result;
}

bool CStyleInterpreter::parseLogicalOr() {
    bool result = parseLogicalAnd();
    skipSpaces();
    while (current() == '|' && index + 1 < currentLine.length() && currentLine[index + 1] == '|') {
        index += 2;
        skipSpaces();
        bool right = parseLogicalAnd();
        result = result || right;
        skipSpaces();
    }
    return result;
}

bool CStyleInterpreter::parseCondition() {
    if (current() != '(') throw std::runtime_error("Expected '(' for condition");
    next();

    bool result = parseLogicalOr();

    if (current() != ')') throw std::runtime_error("Expected ')' after condition");
    next();

    return result;
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

        if (containsKeyword(line, "while") && line.find("#") == std::string::npos) {
            loopStack.push_back(ip);
            isForStack.push_back(false);
        }
        if (containsKeyword(line, "for") && line.find("#") == std::string::npos) {
            loopStack.push_back(ip);
            isForStack.push_back(true);
        }

        ip++;
        try {
            executeLine(ip - 1);
        } catch (const std::exception& e) {
            size_t origLine = (ip - 1 < originalLineNumbers.size()) ? originalLineNumbers[ip - 1] : ip;
            throw std::runtime_error("Line " + std::to_string(origLine) + ": " + e.what());
        }

        if (breakOnReturn && containsKeyword(line, "return") && line.find("#") == std::string::npos) {
            break;
        }

        // Handle break: skip to end of current loop block
        if (breakFlag && !loopStack.empty()) {
            breakFlag = false;
            size_t startLoop = loopStack.back();
            bool isFor = isForStack.back();
            // Calculate current nesting depth relative to loop body
            int depth = 0;
            for (size_t k = startLoop + 1; k < ip; k++) {
                for (char c : lines[k]) {
                    if (c == '{') depth++;
                    if (c == '}') depth--;
                }
            }
            // Skip forward until we exit the loop (depth goes negative)
            while (ip < lines.size()) {
                std::string l = lines[ip];
                ip++;
                for (char c : l) {
                    if (c == '{') depth++;
                    if (c == '}') depth--;
                }
                if (depth < 0) break;
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
                int depth = 0;
                for (size_t k = startLoop + 1; k < ip; k++) {
                    for (char c : lines[k]) {
                        if (c == '{') depth++;
                        if (c == '}') depth--;
                    }
                }
                while (ip < lines.size()) {
                    std::string l = lines[ip];
                    ip++;
                    for (char c : l) {
                        if (c == '{') depth++;
                        if (c == '}') depth--;
                    }
                    if (depth < 0) break;
                }
                loopStack.pop_back();
                isForStack.pop_back();
                if (isFor && !forStepStack.empty()) {
                    forStepStack.pop_back();
                }
            }
            continue;
        }

        // Only treat '}' as loop-end when it's the first non-whitespace char
        bool isClosingBrace = false;
        {
            size_t pos = line.find_first_not_of(" \t");
            if (pos != std::string::npos && line[pos] == '}') isClosingBrace = true;
        }
        if (isClosingBrace && !loopStack.empty()) {
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

void CStyleInterpreter::expandInlineBlocks(std::vector<std::string>& src, std::vector<size_t>& lineMap) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < src.size(); i++) {
            const std::string& line = src[i];
            // Find first '{' not inside a string
            int firstOpen = -1;
            bool inStr = false;
            for (size_t j = 0; j < line.size(); j++) {
                if (line[j] == '"') inStr = !inStr;
                if (!inStr && line[j] == '{' && firstOpen < 0) firstOpen = (int)j;
            }
            if (firstOpen < 0) continue;
            // Find matching '}' on the same line
            int depth = 0;
            int matchClose = -1;
            inStr = false;
            for (size_t j = firstOpen; j < line.size(); j++) {
                if (line[j] == '"') inStr = !inStr;
                if (inStr) continue;
                if (line[j] == '{') depth++;
                if (line[j] == '}') {
                    depth--;
                    if (depth == 0) { matchClose = (int)j; break; }
                }
            }
            if (matchClose < 0) continue;
            // Extract content between braces
            std::string content = line.substr(firstOpen + 1, matchClose - firstOpen - 1);
            // Trim content
            size_t cs = content.find_first_not_of(" \t");
            size_t ce = content.find_last_not_of(" \t");
            if (cs == std::string::npos) continue; // empty block
            content = content.substr(cs, ce - cs + 1);
            // Build replacement lines
            std::string before = line.substr(0, firstOpen) + "{";
            std::string after = "}" + line.substr(matchClose + 1);
            // Trim trailing whitespace from 'after'
            size_t ae = after.find_last_not_of(" \t");
            if (ae != std::string::npos) after = after.substr(0, ae + 1);
            // Remember original line number before replacing
            size_t origLine = lineMap[i];
            std::vector<std::string> replacement;
            replacement.push_back(before);
            replacement.push_back("    " + content);
            replacement.push_back(after);
            src.erase(src.begin() + i);
            src.insert(src.begin() + i, replacement.begin(), replacement.end());
            // Update line map: all replacement lines map to same original line
            lineMap.erase(lineMap.begin() + i);
            lineMap.insert(lineMap.begin() + i, 3, origLine);
            changed = true;
            break; // restart scan
        }
    }
}

void CStyleInterpreter::run(const std::vector<std::string>& sourceLines) {
    lines = sourceLines;
    // Initialize original line number mapping (1-based)
    originalLineNumbers.resize(lines.size());
    for (size_t i = 0; i < lines.size(); i++) {
        originalLineNumbers[i] = i + 1;
    }
    expandInlineBlocks(lines, originalLineNumbers);
    ip = 0;
    executeBlock(SIZE_MAX, false);
}
