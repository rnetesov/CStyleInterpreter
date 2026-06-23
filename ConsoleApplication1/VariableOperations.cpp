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
                if (idxVal < 0)
                    throw std::runtime_error("Negative array index");
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
            std::string result = parseStringLiteral();
            skipSpaces();
            // Support string concatenation with +
            while (current() == '+') {
                next();
                skipSpaces();
                if (current() == '"') {
                    result += parseStringLiteral();
                } else {
                    // Concatenate with a string variable
                    std::string concatVar;
                    while (std::isalnum(current()) || current() == '_') {
                        concatVar += current();
                        index++;
                    }
                    skipSpaces();
                    if (!concatVar.empty() && varExists(concatVar)) {
                        auto& var = getVarRef(concatVar);
                        if (std::holds_alternative<std::string>(var)) {
                            result += std::get<std::string>(var);
                        } else if (std::holds_alternative<double>(var)) {
                            double d = std::get<double>(var);
                            if (d == (int)d) {
                                result += std::to_string((int)d);
                            } else {
                                result += std::to_string(d);
                            }
                        } else {
                            throw std::runtime_error("Cannot concatenate variable '" + concatVar + "' to string");
                        }
                    } else if (!concatVar.empty()) {
                        throw std::runtime_error("Variable '" + concatVar + "' is not initialized");
                    } else {
                        throw std::runtime_error("Expected string literal or variable after '+' in string concatenation");
                    }
                }
                skipSpaces();
            }
            setVariable(varName, result);
            return;
        }

        if (currentLine.find("array(") != std::string::npos) {
            double size = parseExpression();
            if (size < 0)
                throw std::runtime_error("Array size cannot be negative");
            if (size > 1000000)
                throw std::runtime_error("Array size exceeds maximum allowed (1000000)");
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

        // Check if the RHS is a string variable (for concatenation like: a = name + " world")
        if (!funcCheck.empty() && varExists(funcCheck)) {
            auto& rhsVar = getVarRef(funcCheck);
            if (std::holds_alternative<std::string>(rhsVar)) {
                index = checkIdx;
                skipSpaces();
                std::string result = std::get<std::string>(rhsVar);
                while (current() == '+') {
                    next();
                    skipSpaces();
                    if (current() == '"') {
                        result += parseStringLiteral();
                    } else {
                        std::string concatVar;
                        while (std::isalnum(current()) || current() == '_') {
                            concatVar += current();
                            index++;
                        }
                        skipSpaces();
                        if (!concatVar.empty() && varExists(concatVar)) {
                            auto& cv = getVarRef(concatVar);
                            if (std::holds_alternative<std::string>(cv)) {
                                result += std::get<std::string>(cv);
                            } else if (std::holds_alternative<double>(cv)) {
                                double d = std::get<double>(cv);
                                if (d == (int)d) {
                                    result += std::to_string((int)d);
                                } else {
                                    result += std::to_string(d);
                                }
                            } else {
                                throw std::runtime_error("Cannot concatenate variable '" + concatVar + "' to string");
                            }
                        } else if (!concatVar.empty()) {
                            throw std::runtime_error("Variable '" + concatVar + "' is not initialized");
                        }
                    }
                    skipSpaces();
                }
                setVariable(varName, result);
                return;
            }
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
    else if (current() == '-') {
        next();
        if (current() == '-') {
            next();
            if (varExists(varName)) {
                auto& var = getVarRef(varName);
                if (std::holds_alternative<double>(var)) {
                    var = std::get<double>(var) - 1;
                }
                else throw std::runtime_error("Cannot decrement this variable type");
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
                    var = std::get<double>(var) - value;
                }
                else throw std::runtime_error("Cannot use -= on this variable type");
                return;
            }
            throw std::runtime_error("Variable '" + varName + "' is not initialized");
        }
    }
    else if (current() == '*') {
        next();
        if (current() == '=') {
            next();
            double value = parseExpression();
            if (varExists(varName)) {
                auto& var = getVarRef(varName);
                if (std::holds_alternative<double>(var)) {
                    var = std::get<double>(var) * value;
                }
                else throw std::runtime_error("Cannot use *= on this variable type");
                return;
            }
            throw std::runtime_error("Variable '" + varName + "' is not initialized");
        }
    }
    else if (current() == '/') {
        next();
        if (current() == '=') {
            next();
            double value = parseExpression();
            if (value == 0) throw std::runtime_error("Division by zero in /= for variable '" + varName + "'");
            if (varExists(varName)) {
                auto& var = getVarRef(varName);
                if (std::holds_alternative<double>(var)) {
                    var = std::get<double>(var) / value;
                }
                else throw std::runtime_error("Cannot use /= on this variable type");
                return;
            }
            throw std::runtime_error("Variable '" + varName + "' is not initialized");
        }
    }

    throw std::runtime_error("Unknown syntax after variable name: " + varName);
}
