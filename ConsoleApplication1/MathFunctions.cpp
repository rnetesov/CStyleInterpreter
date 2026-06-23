#include "CStyleInterpreter.h"
#include <cmath>
#include <stdexcept>
#include <cstdlib>
#include <climits>

double CStyleInterpreter::builtin_sin(double arg) {
    return std::sin(arg);
}

double CStyleInterpreter::builtin_cos(double arg) {
    return std::cos(arg);
}

double CStyleInterpreter::builtin_sqrt(double arg) {
    if (arg < 0) {
        throw std::runtime_error("Square root of negative number");
    }
    return std::sqrt(arg);
}

double CStyleInterpreter::builtin_rand(double arg) {
    if (arg < 1) {
        throw std::runtime_error("Argument for rand() must be 1 or greater");
    }
    if (arg > static_cast<double>(INT_MAX)) {
        throw std::runtime_error("Argument for rand() exceeds maximum integer range");
    }
    return (std::rand() % static_cast<int>(arg)) + 1;
}

double CStyleInterpreter::builtin_round(double arg) {
    return std::round(arg);
}

// Реализация встроенной функции floor()
double CStyleInterpreter::builtin_floor(double arg) {
    return std::floor(arg);
}

// Реализация встроенной функции ceil()
double CStyleInterpreter::builtin_ceil(double arg) {
    return std::ceil(arg);
}
