#include <gtest/gtest.h>
#include "test_helpers.h"
#include <cmath>

class MathFunctionsTest : public ::testing::Test {};

TEST_F(MathFunctionsTest, Sin_Zero) {
    auto out = splitLines(runAndCapture({"x = sin(0);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), 0.0);
}

TEST_F(MathFunctionsTest, Sin_PiOver2) {
    auto out = splitLines(runAndCapture({"x = sin(pi / 2);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_NEAR(std::stod(out[0]), 1.0, 1e-9);
}

TEST_F(MathFunctionsTest, Cos_Zero) {
    auto out = splitLines(runAndCapture({"x = cos(0);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_NEAR(std::stod(out[0]), 1.0, 1e-9);
}

TEST_F(MathFunctionsTest, Cos_Pi) {
    auto out = splitLines(runAndCapture({"x = cos(pi);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_NEAR(std::stod(out[0]), -1.0, 1e-9);
}

TEST_F(MathFunctionsTest, Sqrt_Positive) {
    auto out = splitLines(runAndCapture({"x = sqrt(25);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), 5.0);
}

TEST_F(MathFunctionsTest, Sqrt_Zero) {
    auto out = splitLines(runAndCapture({"x = sqrt(0);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), 0.0);
}

TEST_F(MathFunctionsTest, Sqrt_NegativeThrows) {
    EXPECT_THROW(
        runAndCapture({"x = sqrt(-1);", "print x;"}),
        std::runtime_error
    );
}

TEST_F(MathFunctionsTest, Round_Down) {
    auto out = splitLines(runAndCapture({"x = round(2.3);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), 2.0);
}

TEST_F(MathFunctionsTest, Round_Up) {
    auto out = splitLines(runAndCapture({"x = round(2.7);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), 3.0);
}

TEST_F(MathFunctionsTest, Floor_Positive) {
    auto out = splitLines(runAndCapture({"x = floor(2.9);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), 2.0);
}

TEST_F(MathFunctionsTest, Floor_Negative) {
    auto out = splitLines(runAndCapture({"x = floor(-2.1);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), -3.0);
}

TEST_F(MathFunctionsTest, Ceil_Positive) {
    auto out = splitLines(runAndCapture({"x = ceil(2.1);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), 3.0);
}

TEST_F(MathFunctionsTest, Ceil_Negative) {
    auto out = splitLines(runAndCapture({"x = ceil(-2.9);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_DOUBLE_EQ(std::stod(out[0]), -2.0);
}

TEST_F(MathFunctionsTest, Rand_InRange) {
    // rand(N) returns value in [1, N]
    auto out = splitLines(runAndCapture({"x = rand(10);", "print x;"}));
    ASSERT_EQ(out.size(), 1);
    double val = std::stod(out[0]);
    EXPECT_GE(val, 1.0);
    EXPECT_LE(val, 10.0);
}

TEST_F(MathFunctionsTest, Rand_ArgLessThanOneThrows) {
    EXPECT_THROW(
        runAndCapture({"x = rand(0);", "print x;"}),
        std::runtime_error
    );
}

TEST_F(MathFunctionsTest, BuiltinConstants_Pi) {
    auto out = splitLines(runAndCapture({"print pi;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_NEAR(std::stod(out[0]), 3.14159, 1e-4);
}

TEST_F(MathFunctionsTest, BuiltinConstants_E) {
    auto out = splitLines(runAndCapture({"print e;"}));
    ASSERT_EQ(out.size(), 1);
    EXPECT_NEAR(std::stod(out[0]), 2.71828, 1e-4);
}
