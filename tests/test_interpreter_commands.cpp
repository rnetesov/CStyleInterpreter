#include <gtest/gtest.h>
#include "test_helpers.h"

class InterpreterCommandsTest : public ::testing::Test {};

// --- Print ---

TEST_F(InterpreterCommandsTest, PrintStringLiteral) {
    auto out = splitLines(runAndCapture({R"(print "hello world";)"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "hello world");
}

TEST_F(InterpreterCommandsTest, PrintNumericExpression) {
    auto out = splitLines(runAndCapture({"print 2 + 3;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "5");
}

TEST_F(InterpreterCommandsTest, PrintVariable) {
    auto out = splitLines(runAndCapture({"x = 42;", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "42");
}

TEST_F(InterpreterCommandsTest, PrintStringVariable) {
    auto out = splitLines(runAndCapture({
        R"(msg = "test message";)",
        "print msg;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "test message");
}

TEST_F(InterpreterCommandsTest, PrintArray) {
    auto out = splitLines(runAndCapture({
        "a = array(3);",
        "a[0] = 1;",
        "a[1] = 2;",
        "a[2] = 3;",
        "print a;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "[1, 2, 3]");
}

// --- If/Else ---

TEST_F(InterpreterCommandsTest, IfTrue) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "if (x > 5) {",
        R"(print "yes";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "yes");
}

TEST_F(InterpreterCommandsTest, IfFalse) {
    auto out = splitLines(runAndCapture({
        "x = 3;",
        "if (x > 5) {",
        R"(print "yes";)",
        "}"
    }));
    EXPECT_TRUE(out.empty());
}

TEST_F(InterpreterCommandsTest, IfElse_TrueBranch) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "if (x > 5) {",
        R"(print "big";)",
        "}",
        "else {",
        R"(print "small";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "big");
}

TEST_F(InterpreterCommandsTest, IfElse_FalseBranch) {
    auto out = splitLines(runAndCapture({
        "x = 3;",
        "if (x > 5) {",
        R"(print "big";)",
        "}",
        "else {",
        R"(print "small";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "small");
}

// --- Comparison operators ---

TEST_F(InterpreterCommandsTest, ComparisonLessThan) {
    auto out = splitLines(runAndCapture({
        "x = 3;",
        "if (x < 10) {",
        R"(print "yes";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "yes");
}

TEST_F(InterpreterCommandsTest, ComparisonEquals) {
    auto out = splitLines(runAndCapture({
        "x = 5;",
        "if (x == 5) {",
        R"(print "equal";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "equal");
}

TEST_F(InterpreterCommandsTest, ComparisonLessOrEqual) {
    auto out = splitLines(runAndCapture({
        "x = 5;",
        "if (x <= 5) {",
        R"(print "yes";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "yes");
}

TEST_F(InterpreterCommandsTest, ComparisonGreaterOrEqual) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "if (x >= 10) {",
        R"(print "yes";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "yes");
}

// --- While loop ---

TEST_F(InterpreterCommandsTest, WhileLoop) {
    auto out = splitLines(runAndCapture({
        "x = 0;",
        "while (x < 5) {",
        "x++;",
        "}",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "5");
}

TEST_F(InterpreterCommandsTest, WhileLoopNeverEnters) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "while (x < 5) {",
        "x++;",
        "}",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "10");
}

TEST_F(InterpreterCommandsTest, WhileLoopAccumulate) {
    auto out = splitLines(runAndCapture({
        "sum = 0;",
        "i = 1;",
        "while (i <= 10) {",
        "sum += i;",
        "i++;",
        "}",
        "print sum;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "55");
}

// --- For loop ---

TEST_F(InterpreterCommandsTest, ForLoop) {
    auto out = splitLines(runAndCapture({
        "sum = 0;",
        "for (i = 1; i <= 5; i++) {",
        "sum += i;",
        "}",
        "print sum;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "15");
}

TEST_F(InterpreterCommandsTest, ForLoopNeverEnters) {
    auto out = splitLines(runAndCapture({
        "sum = 0;",
        "for (i = 10; i < 5; i++) {",
        "sum += i;",
        "}",
        "print sum;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

TEST_F(InterpreterCommandsTest, ForLoopWithArray) {
    auto out = splitLines(runAndCapture({
        "a = array(5);",
        "for (i = 0; i < 5; i++) {",
        "a[i] = i * 10;",
        "}",
        "print a;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "[0, 10, 20, 30, 40]");
}

// --- Functions ---

TEST_F(InterpreterCommandsTest, FunctionDefinitionAndCall) {
    auto out = splitLines(runAndCapture({
        "func square(x) {",
        "return x * x;",
        "}",
        "r = square(5);",
        "print r;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "25");
}

TEST_F(InterpreterCommandsTest, FunctionMultipleParams) {
    auto out = splitLines(runAndCapture({
        "func add(a, b) {",
        "return a + b;",
        "}",
        "r = add(10, 20);",
        "print r;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "30");
}

TEST_F(InterpreterCommandsTest, FunctionThreeParams) {
    auto out = splitLines(runAndCapture({
        "func sum3(a, b, c) {",
        "return a + b + c;",
        "}",
        "r = sum3(1, 2, 3);",
        "print r;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "6");
}

TEST_F(InterpreterCommandsTest, FunctionWrongArgCountThrows) {
    EXPECT_THROW(
        runAndCapture({
            "func f(a, b) {",
            "return a + b;",
            "}",
            "r = f(1);"
        }),
        std::runtime_error
    );
}

TEST_F(InterpreterCommandsTest, FunctionLocalScope) {
    // Function local vars should not leak to global scope
    auto out = splitLines(runAndCapture({
        "x = 100;",
        "func setx(val) {",
        "x = val;",
        "return x;",
        "}",
        "r = setx(999);",
        "print x;",
        "print r;"
    }));
    ASSERT_EQ(out.size(), 2);
    EXPECT_EQ(out[0], "100");
    EXPECT_EQ(out[1], "999");
}

TEST_F(InterpreterCommandsTest, FunctionWithLoop) {
    auto out = splitLines(runAndCapture({
        "func my_pow(base, exp) {",
        "result = 1;",
        "for (i = 0; i < exp; i++) {",
        "result = result * base;",
        "}",
        "return result;",
        "}",
        "r = my_pow(2, 8);",
        "print r;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "256");
}

TEST_F(InterpreterCommandsTest, FunctionCalledMultipleTimes) {
    auto out = splitLines(runAndCapture({
        "func double(x) {",
        "return x * 2;",
        "}",
        "a = double(5);",
        "b = double(10);",
        "c = a + b;",
        "print c;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "30");
}

// --- Return ---

TEST_F(InterpreterCommandsTest, ReturnValue) {
    auto out = splitLines(runAndCapture({
        "func get42() {",
        "return 42;",
        "}",
        "x = get42();",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "42");
}

TEST_F(InterpreterCommandsTest, ReturnExpression) {
    auto out = splitLines(runAndCapture({
        "func calc(a, b) {",
        "return a * b + 10;",
        "}",
        "x = calc(3, 4);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "22");
}

// --- Comments ---

TEST_F(InterpreterCommandsTest, CommentsAreIgnored) {
    auto out = splitLines(runAndCapture({
        "# This is a comment",
        "x = 42;",
        "# Another comment",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "42");
}

// --- Multiple prints ---

TEST_F(InterpreterCommandsTest, MultiplePrints) {
    auto out = splitLines(runAndCapture({
        R"(print "line1";)",
        R"(print "line2";)",
        R"(print "line3";)"
    }));
    ASSERT_EQ(out.size(), 3);
    EXPECT_EQ(out[0], "line1");
    EXPECT_EQ(out[1], "line2");
    EXPECT_EQ(out[2], "line3");
}
