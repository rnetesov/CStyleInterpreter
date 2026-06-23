#include <gtest/gtest.h>
#include "test_helpers.h"
#include <cmath>

class MathParserTest : public ::testing::Test {};

// --- Basic arithmetic ---

TEST_F(MathParserTest, Addition) {
    auto out = splitLines(runAndCapture({"x = 2 + 3;", "print x;"}));
    EXPECT_EQ(out[0], "5");
}

TEST_F(MathParserTest, Subtraction) {
    auto out = splitLines(runAndCapture({"x = 10 - 4;", "print x;"}));
    EXPECT_EQ(out[0], "6");
}

TEST_F(MathParserTest, Multiplication) {
    auto out = splitLines(runAndCapture({"x = 6 * 7;", "print x;"}));
    EXPECT_EQ(out[0], "42");
}

TEST_F(MathParserTest, Division) {
    auto out = splitLines(runAndCapture({"x = 15 / 3;", "print x;"}));
    EXPECT_EQ(out[0], "5");
}

TEST_F(MathParserTest, DivisionByZeroThrows) {
    EXPECT_THROW(
        runAndCapture({"x = 10 / 0;"}),
        std::runtime_error
    );
}

// --- Operator precedence ---

TEST_F(MathParserTest, MultiplicationBeforeAddition) {
    auto out = splitLines(runAndCapture({"x = 2 + 3 * 4;", "print x;"}));
    EXPECT_EQ(out[0], "14");
}

TEST_F(MathParserTest, ParenthesesOverride) {
    auto out = splitLines(runAndCapture({"x = (2 + 3) * 4;", "print x;"}));
    EXPECT_EQ(out[0], "20");
}

TEST_F(MathParserTest, NestedParentheses) {
    auto out = splitLines(runAndCapture({"x = ((2 + 3) * (4 - 1));", "print x;"}));
    EXPECT_EQ(out[0], "15");
}

// --- Power operator ---

TEST_F(MathParserTest, PowerOperator) {
    auto out = splitLines(runAndCapture({"x = 2 ^ 10;", "print x;"}));
    EXPECT_EQ(out[0], "1024");
}

TEST_F(MathParserTest, PowerRightAssociative) {
    // 2^3^2 should be 2^(3^2) = 2^9 = 512
    auto out = splitLines(runAndCapture({"x = 2 ^ 3 ^ 2;", "print x;"}));
    EXPECT_EQ(out[0], "512");
}

TEST_F(MathParserTest, PowerPrecedenceOverMultiplication) {
    // 2 * 3^2 = 2 * 9 = 18
    auto out = splitLines(runAndCapture({"x = 2 * 3 ^ 2;", "print x;"}));
    EXPECT_EQ(out[0], "18");
}

// --- Unary minus ---

TEST_F(MathParserTest, UnaryMinus) {
    auto out = splitLines(runAndCapture({"x = -5;", "print x;"}));
    EXPECT_EQ(out[0], "-5");
}

TEST_F(MathParserTest, UnaryMinusInExpression) {
    auto out = splitLines(runAndCapture({"x = 10 + -3;", "print x;"}));
    EXPECT_EQ(out[0], "7");
}

// --- Floating point ---

TEST_F(MathParserTest, FloatingPointArithmetic) {
    auto out = splitLines(runAndCapture({"x = 1.5 + 2.5;", "print x;"}));
    EXPECT_EQ(out[0], "4");
}

TEST_F(MathParserTest, FloatingPointDivision) {
    auto out = splitLines(runAndCapture({"x = 7 / 2;", "print x;"}));
    EXPECT_EQ(out[0], "3.5");
}

// --- Complex expression ---

TEST_F(MathParserTest, ComplexExpression) {
    // (10 + 2) * 3 - 4 / 2 = 36 - 2 = 34
    auto out = splitLines(runAndCapture({"x = (10 + 2) * 3 - 4 / 2;", "print x;"}));
    EXPECT_EQ(out[0], "34");
}

// --- Variables in expressions ---

TEST_F(MathParserTest, VariablesInExpression) {
    auto out = splitLines(runAndCapture({
        "a = 10;",
        "b = 20;",
        "c = a + b * 2;",
        "print c;"
    }));
    EXPECT_EQ(out[0], "50");
}

TEST_F(MathParserTest, ChainedAssignments) {
    auto out = splitLines(runAndCapture({
        "a = 5;",
        "b = a * 2;",
        "c = b + a;",
        "print c;"
    }));
    EXPECT_EQ(out[0], "15");
}
