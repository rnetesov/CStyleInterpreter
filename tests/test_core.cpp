#include <gtest/gtest.h>
#include "test_helpers.h"

class CoreTest : public ::testing::Test {};

// --- Scope management ---

TEST_F(CoreTest, GlobalVariablePersists) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "y = 20;",
        "z = x + y;",
        "print z;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "30");
}

TEST_F(CoreTest, FunctionScopeIsolation) {
    // Variables defined inside a function should not be visible outside
    auto out = splitLines(runAndCapture({
        "global_val = 100;",
        "func test_scope(n) {",
        "local_var = n * 2;",
        "return local_var;",
        "}",
        "r = test_scope(5);",
        "print global_val;",
        "print r;"
    }));
    ASSERT_EQ(out.size(), 2);
    EXPECT_EQ(out[0], "100");
    EXPECT_EQ(out[1], "10");
}

TEST_F(CoreTest, NestedFunctionScopeIsolation) {
    // Calling a function that uses the same variable names as the caller
    auto out = splitLines(runAndCapture({
        "func inner(x) {",
        "return x + 1;",
        "}",
        "func outer(x) {",
        "y = inner(x);",
        "return y + x;",
        "}",
        "r = outer(10);",
        "print r;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "21");
}

// --- Condition parsing ---

TEST_F(CoreTest, ConditionLessThan) {
    auto out = splitLines(runAndCapture({
        "a = 5;",
        "if (a < 10) {",
        R"(print "lt";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "lt");
}

TEST_F(CoreTest, ConditionGreaterThan) {
    auto out = splitLines(runAndCapture({
        "a = 15;",
        "if (a > 10) {",
        R"(print "gt";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "gt");
}

TEST_F(CoreTest, ConditionEqual) {
    auto out = splitLines(runAndCapture({
        "a = 10;",
        "if (a == 10) {",
        R"(print "eq";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "eq");
}

TEST_F(CoreTest, ConditionLessOrEqual) {
    auto out = splitLines(runAndCapture({
        "a = 10;",
        "if (a <= 10) {",
        R"(print "le";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "le");
}

TEST_F(CoreTest, ConditionGreaterOrEqual) {
    auto out = splitLines(runAndCapture({
        "a = 10;",
        "if (a >= 10) {",
        R"(print "ge";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "ge");
}

TEST_F(CoreTest, ConditionWithExpressions) {
    auto out = splitLines(runAndCapture({
        "a = 5;",
        "b = 3;",
        "if (a + b == 8) {",
        R"(print "correct";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "correct");
}

// --- skipBlock ---

TEST_F(CoreTest, SkipBlockOnFalseCondition) {
    auto out = splitLines(runAndCapture({
        "x = 0;",
        "if (x > 100) {",
        "x = 999;",
        R"(print "should not appear";)",
        "}",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

TEST_F(CoreTest, SkipNestedBlock) {
    auto out = splitLines(runAndCapture({
        "x = 0;",
        "if (x > 100) {",
        "if (x > 200) {",
        R"(print "inner";)",
        "}",
        R"(print "outer";)",
        "}",
        R"(print "after";)"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "after");
}

// --- Empty program ---

TEST_F(CoreTest, EmptyProgram) {
    auto out = runAndCapture({});
    EXPECT_EQ(out, "");
}

TEST_F(CoreTest, OnlyComments) {
    auto out = runAndCapture({
        "# comment 1",
        "# comment 2"
    });
    EXPECT_EQ(out, "");
}

TEST_F(CoreTest, EmptyLines) {
    auto out = runAndCapture({"", "", ""});
    EXPECT_EQ(out, "");
}

// --- Error handling ---

TEST_F(CoreTest, UnknownVariableInExpressionThrows) {
    EXPECT_THROW(
        runAndCapture({"x = undefined_var;"}),
        std::runtime_error
    );
}

TEST_F(CoreTest, UnknownFunctionCallThrows) {
    EXPECT_THROW(
        runAndCapture({"x = nonexistent_func(1);"}),
        std::runtime_error
    );
}

// --- Integration: while loop with function ---

TEST_F(CoreTest, WhileLoopWithFunctionCall) {
    auto out = splitLines(runAndCapture({
        "func double(x) {",
        "return x * 2;",
        "}",
        "sum = 0;",
        "i = 1;",
        "while (i <= 3) {",
        "sum += double(i);",
        "i++;",
        "}",
        "print sum;"
    }));
    ASSERT_EQ(out.size(), 1);
    // double(1) + double(2) + double(3) = 2 + 4 + 6 = 12
    EXPECT_EQ(out[0], "12");
}

// --- Integration: for loop filling array, then summing ---

TEST_F(CoreTest, ForLoopArrayFillAndSum) {
    auto out = splitLines(runAndCapture({
        "arr = array(5);",
        "for (i = 0; i < 5; i++) {",
        "arr[i] = (i + 1) * 10;",
        "}",
        "total = 0;",
        "for (j = 0; j < 5; j++) {",
        "total += arr[j];",
        "}",
        "print total;"
    }));
    ASSERT_EQ(out.size(), 1);
    // 10 + 20 + 30 + 40 + 50 = 150
    EXPECT_EQ(out[0], "150");
}

// --- Integration: math functions in conditions ---

TEST_F(CoreTest, MathFunctionInCondition) {
    auto out = splitLines(runAndCapture({
        "x = floor(3.9);",
        "if (x == 3) {",
        R"(print "correct";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "correct");
}

// --- Integration: string ops then conditional ---

TEST_F(CoreTest, StringOpsWithConditional) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello world";)",
        R"(pos = str_pos(s, "world");)",
        "if (pos > 0) {",
        R"(print "found";)",
        "}"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "found");
}
