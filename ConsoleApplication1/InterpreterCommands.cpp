#include "CStyleInterpreter.h"
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <fstream>
#include <limits>

void CStyleInterpreter::executeLine(size_t lineIdx) {
	ParserStateGuard guard(*this);

	currentLine = lines[lineIdx];
	index = 0;
	skipSpaces();

	if (currentLine.empty() || current() == '#' || current() == '\0' || current() == '{') {
		return;
	}

	if (current() == '}') {
		// Skip all remaining else-if / else branches (the taken branch already executed)
		bool hasElse = false;
		if (currentLine.find("else") != std::string::npos) {
			hasElse = true;
		} else if (ip < lines.size() && lines[ip].find("else") != std::string::npos) {
			ip++;
			hasElse = true;
		}
		while (hasElse) {
			skipBlock();
			hasElse = false;
			if (ip > 0 && ip - 1 < lines.size() && lines[ip - 1].find("else") != std::string::npos) {
				hasElse = true;
			} else if (ip < lines.size() && lines[ip].find("else") != std::string::npos) {
				ip++;
				hasElse = true;
			}
		}
		return;
	}

	std::string firstWord;
	if (std::isalpha(current()) || current() == '_') {
		firstWord = parseIdentifier();
	}

	if (firstWord.empty()) {
		return;
	}

	// --- КОМАНДА: ЦИКЛ FOR ---
	if (firstWord == "for") {
		if (current() != '(') throw std::runtime_error("Expected '(' after for");
		next();

		std::string initCmd;
		while (index < currentLine.length() && currentLine[index] != ';') {
			initCmd += currentLine[index];
			index++;
		}
		if (current() != ';') throw std::runtime_error("Expected ';' after for initialization");
		next();

		executeSingleLineFromString(initCmd + ";");

		std::string conditionPart = "(";
		while (index < currentLine.length() && currentLine[index] != ';') {
			conditionPart += currentLine[index];
			index++;
		}
		conditionPart += ")";
		if (current() != ';') throw std::runtime_error("Expected ';' after for condition");
		next();

		std::string stepCmd;
		while (index < currentLine.length() && currentLine[index] != ')') {
			stepCmd += currentLine[index];
			index++;
		}
		if (current() != ')') throw std::runtime_error("Expected ')' after for step");
		next();
		skipSpaces();

		bool cond = parseConditionFromString("while " + conditionPart);
		if (!cond) {
			skipBlock();
		}
		else {
			forStepStack.push_back(stepCmd + ";");
		}

		return;
	}

	// Команда INCLUDE
	if (firstWord == "include") {
		std::string incFileName = parseStringLiteral();
		if (current() != ';') throw std::runtime_error("Missing ';' at the end of include command");

		std::ifstream incFile(incFileName);
		if (!incFile.is_open()) throw std::runtime_error("Could not open include file: " + incFileName);

		std::vector<std::string> newLines;
		std::string l;
		while (std::getline(incFile, l)) {
			newLines.push_back(l);
		}
		incFile.close();
		lines.insert(lines.begin() + ip, newLines.begin(), newLines.end());

		return;
	}

	// Команда RETURN
	if (firstWord == "return") {
		double val = parseExpression();
		if (current() != ';') throw std::runtime_error("Missing ';' at the end of return command");
		lastReturnValue = val;
		return;
	}

	// Команда FUNC
	if (firstWord == "func") {
		std::string funcName = parseIdentifier();
		if (current() != '(') throw std::runtime_error("Expected '(' after function name");
		next();
		skipSpaces();

		UserFunction newFunc;
		newFunc.bodyLineIdx = ip;

		if (current() != ')') {
			while (true) {
				std::string paramName = parseIdentifier();
				if (paramName.empty()) throw std::runtime_error("Expected parameter name in function definition");

				newFunc.paramNames.push_back(paramName);

				if (current() == ',') {
					next();
					skipSpaces();
				}
				else if (current() == ')') {
					break;
				}
				else {
					throw std::runtime_error("Expected ',' or ')' in function parameter list");
				}
			}
		}

		if (current() != ')') throw std::runtime_error("Expected ')' after parameter list");
		next();
		skipSpaces();

		functions[funcName] = newFunc;
		skipBlock();
		return;
	}

	if (firstWord == "print") {
		if (current() == '"') {
			std::string literalText = parseStringLiteral();
			if (current() != ';') throw std::runtime_error("Missing ';' at the end of print command");
			std::cout << literalText << std::endl;
		}
		else {
			std::string varCheck;
			size_t checkIdx = index;
			while (checkIdx < currentLine.length() && (std::isalnum(currentLine[checkIdx]) || currentLine[checkIdx] == '_')) {
				varCheck += currentLine[checkIdx];
				checkIdx++;
			}

			if (!varCheck.empty() && varExists(varCheck)) {
				// Peek ahead: if next non-space char is '[', this is array element access (arr[i]),
				// so fall through to parseExpression() instead of printing the whole variable.
				size_t peekIdx = checkIdx;
				while (peekIdx < currentLine.length() && std::isspace(currentLine[peekIdx])) peekIdx++;
				bool isIndexAccess = (peekIdx < currentLine.length() && currentLine[peekIdx] == '[');

				if (!isIndexAccess) {
				auto& var = getVarRef(varCheck);
				if (std::holds_alternative<std::string>(var)) {
					index = checkIdx;
					skipSpaces();
					if (current() != ';') throw std::runtime_error("Missing ';' after print command");
					std::cout << std::get<std::string>(var) << std::endl;
					return;
				}
				else if (std::holds_alternative<std::vector<double>>(var)) {
					index = checkIdx;
					skipSpaces();
					if (current() != ';') throw std::runtime_error("Missing ';' after print command");
					const auto& arr = std::get<std::vector<double>>(var);
					std::cout << "[";
					for (size_t i = 0; i < arr.size(); ++i) {
						std::cout << arr[i];
						if (i + 1 < arr.size()) std::cout << ", ";
					}
					std::cout << "]" << std::endl;
					return;
				}
				}
			}

			double value = parseExpression();
			if (current() != ';') throw std::runtime_error("Missing ';' at the end of print command");
			std::cout << value << std::endl;
		}

		return;
	}

	// Команда INPUT
	if (firstWord == "input") {
		std::string varName = parseIdentifier();
		if (varName.empty()) throw std::runtime_error("Expected variable name after 'input'");
		if (current() != ';') throw std::runtime_error("Missing ';' at the end of input command");

		double userVal;
		std::cout << "[INPUT FOR " << varName << "]: ";
		std::cin >> userVal;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			throw std::runtime_error("Invalid input: expected a numeric value for variable '" + varName + "'");
		}

		setVariable(varName, userVal);
		return;
	}

	// Команда IF
	if (firstWord == "if") {
		bool condition = parseCondition();
		skipSpaces();
		if (!condition) {
			skipBlock();
			// Handle else-if / else chain
			while (true) {
				std::string elseLineText;

				// Check closing '}' line for inline else (e.g., "} else if (...) {")
				if (ip > 0 && ip - 1 < lines.size() && lines[ip - 1].find("else") != std::string::npos) {
					elseLineText = lines[ip - 1];
				}
				// Check next line for separate else/else-if
				else if (ip < lines.size() && lines[ip].find("else") != std::string::npos) {
					elseLineText = lines[ip];
					ip++;
				}

				if (elseLineText.empty()) break;

				// Determine if it's "else if" or plain "else"
				size_t elsePos = elseLineText.find("else");
				std::string afterElse = elseLineText.substr(elsePos + 4);
				size_t nonSpace = afterElse.find_first_not_of(" \t");
				bool isElseIf = (nonSpace != std::string::npos && afterElse.substr(nonSpace, 2) == "if");

				if (isElseIf) {
					bool elseIfCond = parseConditionFromString(elseLineText);
					if (elseIfCond) {
						break; // Take this branch
					} else {
						skipBlock();
						continue; // Check next branch
					}
				} else {
					// Plain else — let body execute
					break;
				}
			}
		}
		return;
	}

	if (firstWord == "else") {
		return;
	}

	// Команда WHILE
	if (firstWord == "while") {
		bool condition = parseCondition();
		if (!condition) {
			skipBlock();
		}
		return;
	}

	if (firstWord == "trim" && current() == '(') {
		builtin_trim();
		return;
	}
	if (firstWord == "str_replace" && current() == '(') {
		builtin_str_replace();
		return;
	}

	if (functions.find(firstWord) != functions.end() && current() == '(') {
		index = 0;
		parseFactor();
		if (current() != ';') throw std::runtime_error("Missing ';' after function call");
		return;
	}

	if (!firstWord.empty()) {
		processVariableAssignment(firstWord);
		return;
	}
	throw std::runtime_error("Unknown syntax: " + firstWord);
}