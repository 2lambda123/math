#ifndef TEST_UNIT_UTIL_HPP
#define TEST_UNIT_UTIL_HPP

#include <boost/typeof/typeof.hpp>
#include <gtest/gtest.h>
#include <type_traits>
#include <string>

#define EXPECT_MATRIX_EQ(A, B)                       \
  {                                                  \
    const auto& A_eval = A.eval();                   \
    const auto& B_eval = B.eval();                   \
    EXPECT_EQ(A_eval.rows(), B_eval.rows());         \
    EXPECT_EQ(A_eval.cols(), B_eval.cols());         \
    for (int i = 0; i < A_eval.rows(); i++)          \
      for (int j = 0; j < A_eval.cols(); j++)        \
        EXPECT_FLOAT_EQ(A_eval(i, j), B_eval(i, j)); \
  }

#define EXPECT_MATRIX_FLOAT_EQ(A, B)                 \
  {                                                  \
    const auto& A_eval = A.eval();                   \
    const auto& B_eval = B.eval();                   \
    EXPECT_EQ(A_eval.rows(), B_eval.rows());         \
    EXPECT_EQ(A_eval.cols(), B_eval.cols());         \
    for (int i = 0; i < A_eval.rows(); i++)          \
      for (int j = 0; j < A_eval.cols(); j++)        \
        EXPECT_FLOAT_EQ(A_eval(i, j), B_eval(i, j)); \
  }

#define EXPECT_STD_VECTOR_FLOAT_EQ(A, B) \
  EXPECT_EQ(A.size(), B.size());         \
  for (int i = 0; i < A.size(); ++i)     \
    EXPECT_FLOAT_EQ(A[i], B[i]);

#define EXPECT_MATRIX_NEAR(A, B, DELTA)              \
  {                                                  \
    auto A_eval = A.eval();                          \
    auto B_eval = B.eval();                          \
    EXPECT_EQ(A_eval.rows(), B_eval.rows());         \
    EXPECT_EQ(A_eval.cols(), B_eval.cols());         \
    for (int i = 0; i < A_eval.rows(); i++)          \
      for (int j = 0; j < A_eval.cols(); j++)        \
        EXPECT_FLOAT_EQ(A_eval(i, j), B_eval(i, j)); \
  }

#define EXPECT_THROW_MSG_WITH_COUNT(expr, T_e, msg, count) \
  EXPECT_THROW(expr, T_e);                                 \
  try {                                                    \
    expr;                                                  \
  } catch (const T_e& e) {                                 \
    EXPECT_EQ(count, count_matches(msg, e.what()))         \
        << "expected message: " << msg << std::endl        \
        << "found message:    " << e.what();               \
  }

#define EXPECT_THROW_MSG(expr, T_e, msg) \
  EXPECT_THROW_MSG_WITH_COUNT(expr, T_e, msg, 1)

int count_matches(const std::string& target, const std::string& s) {
  if (target.size() == 0)
    return -1;  // error
  int count = 0;
  for (size_t pos = 0; (pos = s.find(target, pos)) != std::string::npos;
       pos += target.size())
    ++count;
  return count;
}

namespace test {
template <typename T1, typename T2>
void expect_same_type() {
  bool b = std::is_same<T1, T2>::value;
  EXPECT_TRUE(b);
}
}  // namespace test

#endif
