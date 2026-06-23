#include "CStyleInterpreter.h"
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <fstream>
#include <limits>

void CStyleInterpreter::executeLine(size_t lineIdx) {
	// КРИТИЧЕСКОЕ АРХИТЕКТУРНОЕ ИСПРАВЛЕНИЕ:
	// Сохраняем старое состояние, чтобы вложенные вызовы функций
	// или шаги циклов for никогда не сбивали указатели парсера.
	std::string savedLine = currentLine;
	size_t savedIndex = index;

	currentLine = lines[lineIdx];
	index = 0;
	skipSpaces();

	if (currentLine.empty() || current() == '#' || current() == '\0' || current() == '{') {
		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	if (current() == '}') {
		if (ip < lines.size()) {
			std::string nextLine = lines[ip];
			if (nextLine.find("else") != std::string::npos) {
				ip++;
				skipBlock();
			}
		}
		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	std::string firstWord;
	if (std::isalpha(current()) || current() == '_') {
		while (std::isalnum(current()) || current() == '_') {
			firstWord += current();
			index++;
		}
	}
	skipSpaces();

	if (firstWord.empty()) {
		currentLine = savedLine;
		index = savedIndex;
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

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Команда INCLUDE
	if (firstWord == "include") {
		if (current() != '"') throw std::runtime_error("Expected opening double quote after include");
		next();
		std::string incFileName;
		while (index < currentLine.length() && currentLine[index] != '"') {
			incFileName += currentLine[index];
			index++;
		}
		if (index >= currentLine.length()) throw std::runtime_error("Missing closing double quote in include");
		next();
		skipSpaces();
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

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Команда RETURN
	if (firstWord == "return") {
		double val = parseExpression();
		if (current() != ';') throw std::runtime_error("Missing ';' at the end of return command");
		lastReturnValue = val;

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Команда FUNC
	// --- ОБНОВЛЕННАЯ РЕГИСТРАЦИЯ FUNC: Теперь поддерживает список параметров через запятую! ---
	if (firstWord == "func") {
		std::string funcName;
		while (std::isalnum(current()) || current() == '_') {
			funcName += current();
			index++;
		}
		skipSpaces();
		if (current() != '(') throw std::runtime_error("Expected '(' after function name");
		next(); // Пропускаем '('
		skipSpaces();

		UserFunction newFunc;
		newFunc.bodyLineIdx = ip;

		// Считываем список параметров через запятую
		if (current() != ')') {
			while (true) {
				std::string paramName;
				while (std::isalnum(current()) || current() == '_') {
					paramName += current();
					index++;
				}
				skipSpaces();
				if (paramName.empty()) throw std::runtime_error("Expected parameter name in function definition");

				newFunc.paramNames.push_back(paramName); // Добавляем имя параметра в вектор

				if (current() == ',') {
					next(); // Пропускаем ',' и идем к следующему параметру
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
		next(); // Пропускаем ')'
		skipSpaces();

		functions[funcName] = newFunc;
		skipBlock();

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Умный всеядный PRINT
	if (firstWord == "print") {
		if (current() == '"') {
			index++;
			std::string literalText;
			while (index < currentLine.length() && currentLine[index] != '"') {
				literalText += currentLine[index];
				index++;
			}
			if (index >= currentLine.length()) throw std::runtime_error("Missing closing double quote");
			index++;
			skipSpaces();
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

					currentLine = savedLine;
					index = savedIndex;
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

					currentLine = savedLine;
					index = savedIndex;
					return;
				}
				}
			}

			double value = parseExpression();
			if (current() != ';') throw std::runtime_error("Missing ';' at the end of print command");
			std::cout << value << std::endl;
		}

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Команда INPUT
	if (firstWord == "input") {
		std::string varName;
		while (std::isalnum(current()) || current() == '_') {
			varName += current();
			index++;
		}
		skipSpaces();
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

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Команда IF
	if (firstWord == "if") {
		bool condition = parseCondition();
		skipSpaces();
		if (!condition) {
			skipBlock();
			if (ip < lines.size() && lines[ip].find("else") != std::string::npos) {
				ip++;
			}
		}

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	if (firstWord == "else") {
		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Команда WHILE
	if (firstWord == "while") {
		bool condition = parseCondition();
		if (!condition) {
			skipBlock();
		}

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	// Автономные вызовы строковых утилит
	if (firstWord == "trim" && current() == '(') {
		builtin_trim();
		index = currentLine.length();

		currentLine = savedLine;
		index = savedIndex;
		return;
	}
	if (firstWord == "str_replace" && current() == '(') {
		builtin_str_replace();
		index = currentLine.length();

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	if (functions.find(firstWord) != functions.end() && current() == '(') {
		index = 0;
		parseFactor();
		if (current() != ';') throw std::runtime_error("Missing ';' after function call");

		currentLine = savedLine;
		index = savedIndex;
		return;
	}

	if (!firstWord.empty()) {
		processVariableAssignment(firstWord);
		currentLine = savedLine;
		index = savedIndex;
		return;
	}
	throw std::runtime_error("Unknown syntax: " + firstWord);
}