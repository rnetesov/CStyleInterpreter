#pragma once

#include "CStyleInterpreter.h"
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

// Captures stdout produced by interpreter.run(lines)
inline std::string runAndCapture(const std::vector<std::string>& lines) {
    CStyleInterpreter interpreter;
    std::stringstream buffer;
    std::streambuf* oldCout = std::cout.rdbuf(buffer.rdbuf());
    try {
        interpreter.run(lines);
    } catch (...) {
        std::cout.rdbuf(oldCout);
        throw;
    }
    std::cout.rdbuf(oldCout);
    return buffer.str();
}

// Splits a multiline string into individual lines (drops trailing empty)
inline std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream iss(s);
    std::string line;
    while (std::getline(iss, line)) {
        result.push_back(line);
    }
    return result;
}
