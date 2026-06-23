#include <gtest/gtest.h>
#include "test_helpers.h"

class VariableOperationsTest : public ::testing::Test {};

// --- Simple assignment ---

TEST_F(VariableOperationsTest, AssignDouble) {
    auto out = splitLines(runAndCapture({"x = 42;", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "42");
}

TEST_F(VariableOperationsTest, AssignExpression) {
    auto out = splitLines(runAndCapture({"x = 3 + 4 * 2;", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "11");
}

TEST_F(VariableOperationsTest, AssignString) {
    auto out = splitLines(runAndCapture({
        R"(name = "John";)",
        "print name;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "John");
}

TEST_F(VariableOperationsTest, ReassignVariable) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "x = 20;",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "20");
}

// --- Increment (++) ---

TEST_F(VariableOperationsTest, Increment) {
    auto out = splitLines(runAndCapture({
        "x = 5;",
        "x++;",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "6");
}

TEST_F(VariableOperationsTest, IncrementMultiple) {
    auto out = splitLines(runAndCapture({
        "x = 0;",
        "x++;",
        "x++;",
        "x++;",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "3");
}

TEST_F(VariableOperationsTest, IncrementUninitializedThrows) {
    EXPECT_THROW(
        runAndCapture({"y++;"}),
        std::runtime_error
    );
}

// --- Plus-equals (+=) ---

TEST_F(VariableOperationsTest, PlusEquals) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "x += 5;",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "15");
}

TEST_F(VariableOperationsTest, PlusEqualsExpression) {
    auto out = splitLines(runAndCapture({
        "x = 10;",
        "x += 3 * 2;",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "16");
}

TEST_F(VariableOperationsTest, PlusEqualsUninitializedThrows) {
    EXPECT_THROW(
        runAndCapture({"z += 5;"}),
        std::runtime_error
    );
}

// --- Array operations ---

TEST_F(VariableOperationsTest, ArrayCreation) {
    auto out = splitLines(runAndCapture({
        "arr = array(3);",
        "print arr;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "[0, 0, 0]");
}

TEST_F(VariableOperationsTest, ArrayAssignAndRead) {
    auto out = splitLines(runAndCapture({
        "arr = array(3);",
        "arr[0] = 10;",
        "arr[1] = 20;",
        "arr[2] = 30;",
        "print arr;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "[10, 20, 30]");
}

TEST_F(VariableOperationsTest, ArrayIndexRead) {
    auto out = splitLines(runAndCapture({
        "arr = array(3);",
        "arr[0] = 100;",
        "arr[1] = 200;",
        "arr[2] = 300;",
        "x = arr[1];",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "200");
}

TEST_F(VariableOperationsTest, ArrayOutOfBoundsThrows) {
    EXPECT_THROW(
        runAndCapture({
            "arr = array(2);",
            "arr[5] = 10;"
        }),
        std::runtime_error
    );
}

TEST_F(VariableOperationsTest, ArrayInExpression) {
    auto out = splitLines(runAndCapture({
        "arr = array(2);",
        "arr[0] = 3;",
        "arr[1] = 7;",
        "x = arr[0] + arr[1];",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "10");
}

// --- String variable operations ---

TEST_F(VariableOperationsTest, AssignStrlenResult) {
    auto out = splitLines(runAndCapture({
        R"(s = "test";)",
        "n = strlen(s);",
        "print n;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "4");
}

TEST_F(VariableOperationsTest, AssignEmptyResult) {
    auto out = splitLines(runAndCapture({
        R"(s = "test";)",
        "n = empty(s);",
        "print n;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

TEST_F(VariableOperationsTest, AssignIsStringResult) {
    auto out = splitLines(runAndCapture({
        R"(s = "test";)",
        "n = is_string(s);",
        "print n;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "1");
}
