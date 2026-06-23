#include <iostream>
#include "CStyleInterpreter.h"
#include <stdexcept>
#include <vector>

void CStyleInterpreter::processVariableAssignment(const std::string& varName) {
    if (current() == '[') {
        next();
        double idxVal = parseExpression();
        if (current() != ']') throw std::runtime_error("Expected ']'");
        next();
        skipSpaces();
        if (current() != '=') throw std::runtime_error("Expected '=' for array assignment");
        next();
        double value = parseExpression();

        if (varExists(varName)) {
            auto& var = getVarRef(varName);
            if (std::holds_alternative<std::vector<double>>(var)) {
                auto& arr = std::get<std::vector<double>>(var);
                size_t i = static_cast<size_t>(idxVal);
                if (i >= arr.size()) throw std::runtime_error("Array index out of bounds");
                arr[i] = value;
                return;
            }
            throw std::runtime_error("Variable '" + varName + "' is not an array");
        }
        throw std::runtime_error("Variable '" + varName + "' is not initialized");
    }

    if (current() == '=') {
        next();
        skipSpaces();

        if (current() == '"') {
            index++;
            std::string strLit;
            while (index < currentLine.length() && currentLine[index] != '"') {
                strLit += currentLine[index];
                index++;
            }
            if (index >= currentLine.length()) {
                throw std::runtime_error("Missing closing double quote in string assignment to '" + varName + "'");
            }
            index++;
            skipSpaces();
            setVariable(varName, strLit);
            return;
        }

        if (currentLine.find("array(") != std::string::npos) {
            double size = parseExpression();
            std::vector<double> newArr(static_cast<size_t>(size), 0.0);
            setVariable(varName, newArr);
            return;
        }

        std::string funcCheck;
        size_t checkIdx = index;
        while (checkIdx < currentLine.length() && std::isalnum(currentLine[checkIdx])) {
            funcCheck += currentLine[checkIdx];
            checkIdx++;
        }

        if (funcCheck == "strlen" || funcCheck == "empty" || funcCheck == "is_string" || funcCheck == "str_pos") {
            index = checkIdx;
            double funcResult = 0;
            if (funcCheck == "strlen")      funcResult = builtin_strlen();
            if (funcCheck == "empty")       funcResult = builtin_empty();
            if (funcCheck == "is_string")   funcResult = builtin_is_string();
            if (funcCheck == "str_pos")     funcResult = builtin_str_pos();

            skipSpaces();
            setVariable(varName, funcResult);
            return;
        }

        double value = parseExpression();
        setVariable(varName, value);
        return;
    }
    else if (current() == '+') {
        next();
        if (current() == '+') {
            next();
            if (varExists(varName)) {
                auto& var = getVarRef(varName);
                if (std::holds_alternative<double>(var)) {
                    var = std::get<double>(var) + 1;
                }
                else throw std::runtime_error("Cannot increment this variable type");
                return;
            }
            throw std::runtime_error("Variable '" + varName + "' is not initialized");
        }
        else if (current() == '=') {
            next();
            double value = parseExpression();
            if (varExists(varName)) {
                auto& var = getVarRef(varName);
                if (std::holds_alternative<double>(var)) {
                    var = std::get<double>(var) + value;
                }
                else throw std::runtime_error("Cannot use += on this variable type");
                return;
            }
            throw std::runtime_error("Variable '" + varName + "' is not initialized");
        }
    }

    throw std::runtime_error("Unknown syntax after variable name: " + varName);
}
