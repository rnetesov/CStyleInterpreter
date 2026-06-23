#include <gtest/gtest.h>
#include "test_helpers.h"

class StringFunctionsTest : public ::testing::Test {};

// --- strlen ---

TEST_F(StringFunctionsTest, Strlen_BasicString) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello";)",
        "x = strlen(s);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "5");
}

TEST_F(StringFunctionsTest, Strlen_EmptyString) {
    auto out = splitLines(runAndCapture({
        R"(s = "";)",
        "x = strlen(s);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

TEST_F(StringFunctionsTest, Strlen_NonexistentVarReturnsZero) {
    auto out = splitLines(runAndCapture({
        "x = strlen(novar);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

// --- empty ---

TEST_F(StringFunctionsTest, Empty_EmptyStringReturnsOne) {
    auto out = splitLines(runAndCapture({
        R"(s = "";)",
        "x = empty(s);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "1");
}

TEST_F(StringFunctionsTest, Empty_NonEmptyStringReturnsZero) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello";)",
        "x = empty(s);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

TEST_F(StringFunctionsTest, Empty_ZeroDoubleReturnsOne) {
    auto out = splitLines(runAndCapture({
        "n = 0;",
        "x = empty(n);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "1");
}

TEST_F(StringFunctionsTest, Empty_NonZeroDoubleReturnsZero) {
    auto out = splitLines(runAndCapture({
        "n = 42;",
        "x = empty(n);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

TEST_F(StringFunctionsTest, Empty_NonexistentVarReturnsOne) {
    auto out = splitLines(runAndCapture({
        "x = empty(ghost);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "1");
}

TEST_F(StringFunctionsTest, Empty_EmptyArrayReturnsOne) {
    auto out = splitLines(runAndCapture({
        "arr = array(0);",
        "x = empty(arr);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "1");
}

TEST_F(StringFunctionsTest, Empty_NonEmptyArrayReturnsZero) {
    auto out = splitLines(runAndCapture({
        "arr = array(3);",
        "x = empty(arr);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

// --- is_string ---

TEST_F(StringFunctionsTest, IsString_StringReturnsOne) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello";)",
        "x = is_string(s);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "1");
}

TEST_F(StringFunctionsTest, IsString_DoubleReturnsZero) {
    auto out = splitLines(runAndCapture({
        "n = 42;",
        "x = is_string(n);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

TEST_F(StringFunctionsTest, IsString_NonexistentReturnsZero) {
    auto out = splitLines(runAndCapture({
        "x = is_string(nope);",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

// --- str_pos ---

TEST_F(StringFunctionsTest, StrPos_Found) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello world";)",
        R"(x = str_pos(s, "world");)",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "6");
}

TEST_F(StringFunctionsTest, StrPos_NotFound) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello";)",
        R"(x = str_pos(s, "xyz");)",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "-1");
}

TEST_F(StringFunctionsTest, StrPos_AtBeginning) {
    auto out = splitLines(runAndCapture({
        R"(s = "abcdef";)",
        R"(x = str_pos(s, "abc");)",
        "print x;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "0");
}

// --- str_replace ---

TEST_F(StringFunctionsTest, StrReplace_Basic) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello world";)",
        R"(str_replace(s, "world", "there");)",
        "print s;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "hello there");
}

TEST_F(StringFunctionsTest, StrReplace_MultipleOccurrences) {
    auto out = splitLines(runAndCapture({
        R"(s = "aaa";)",
        R"(str_replace(s, "a", "bb");)",
        "print s;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "bbbbbb");
}

TEST_F(StringFunctionsTest, StrReplace_NoMatch) {
    auto out = splitLines(runAndCapture({
        R"(s = "hello";)",
        R"(str_replace(s, "xyz", "replaced");)",
        "print s;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "hello");
}

// --- trim ---

TEST_F(StringFunctionsTest, Trim_LeadingAndTrailing) {
    auto out = splitLines(runAndCapture({
        R"(s = "   hello   ";)",
        "trim(s);",
        "print s;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "hello");
}

TEST_F(StringFunctionsTest, Trim_AllWhitespace) {
    auto out = splitLines(runAndCapture({
        R"(s = "     ";)",
        "trim(s);",
        "print s;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "");
}

TEST_F(StringFunctionsTest, Trim_NoWhitespace) {
    auto out = splitLines(runAndCapture({
        R"(s = "clean";)",
        "trim(s);",
        "print s;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "clean");
}

// --- Print string ---

TEST_F(StringFunctionsTest, PrintStringVariable) {
    auto out = splitLines(runAndCapture({
        R"(greeting = "Hello, World!";)",
        "print greeting;"
    }));
    ASSERT_EQ(out.size(), 1);
    EXPECT_EQ(out[0], "Hello, World!");
}
