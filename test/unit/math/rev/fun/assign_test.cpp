#include <stan/math/rev.hpp>
#include <gtest/gtest.h>
#include <test/unit/math/rev/fun/util.hpp>

TEST(MathMatrixRevMat, getAssignRowVar) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::assign;
  using stan::math::get_base1_lhs;
  using stan::math::var;

  Matrix<var, Dynamic, Dynamic> m(2, 3);
  m << 1, 2, 3, 4, 5, 6;

  Matrix<double, 1, Dynamic> rv(3);
  rv << 10, 100, 1000;

  assign(get_base1_lhs(m, 1, "m", 1), rv);
  EXPECT_FLOAT_EQ(10.0, m(0, 0).val());
  EXPECT_FLOAT_EQ(100.0, m(0, 1).val());
  EXPECT_FLOAT_EQ(1000.0, m(0, 2).val());
}

TEST(AgradRevMatrix, assign) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::assign;
  using std::vector;

  stan::math::var x;
  assign(x, 2.0);
  EXPECT_FLOAT_EQ(2.0, x.val());

  assign(x, 2);
  EXPECT_FLOAT_EQ(2.0, x.val());

  stan::math::var y(3.0);
  assign(x, y);
  EXPECT_FLOAT_EQ(3.0, x.val());

  double xd;
  assign(xd, 2.0);
  EXPECT_FLOAT_EQ(2.0, xd);

  assign(xd, 2);
  EXPECT_FLOAT_EQ(2.0, xd);

  int iii;
  assign(iii, 2);
  EXPECT_EQ(2, iii);

  unsigned int j = 12;
  assign(iii, j);
  EXPECT_EQ(12U, j);

  std::vector<double> y_dbl(2);
  y_dbl[0] = 2.0;
  y_dbl[1] = 3.0;

  std::vector<stan::math::var> y_var(2);
  assign(y_var, y_dbl);
  EXPECT_FLOAT_EQ(2.0, y_var[0].val());
  EXPECT_FLOAT_EQ(3.0, y_var[1].val());

  Matrix<double, Dynamic, 1> v_dbl(6);
  v_dbl << 1, 2, 3, 4, 5, 6;
  Matrix<stan::math::var, Dynamic, 1> v_var(6);
  assign(v_var, v_dbl);
  EXPECT_FLOAT_EQ(1, v_var(0).val());
  EXPECT_FLOAT_EQ(6, v_var(5).val());

  Matrix<double, 1, Dynamic> rv_dbl(3);
  rv_dbl << 2, 4, 6;
  Matrix<stan::math::var, 1, Dynamic> rv_var(3);
  assign(rv_var, rv_dbl);
  EXPECT_FLOAT_EQ(2, rv_var(0).val());
  EXPECT_FLOAT_EQ(4, rv_var(1).val());
  EXPECT_FLOAT_EQ(6, rv_var(2).val());

  Matrix<double, Dynamic, Dynamic> m_dbl(2, 3);
  m_dbl << 2, 4, 6, 100, 200, 300;
  Matrix<stan::math::var, Dynamic, Dynamic> m_var(2, 3);
  assign(m_var, m_dbl);
  EXPECT_EQ(2, m_var.rows());
  EXPECT_EQ(3, m_var.cols());
  EXPECT_FLOAT_EQ(2, m_var(0, 0).val());
  EXPECT_FLOAT_EQ(100, m_var(1, 0).val());
  EXPECT_FLOAT_EQ(300, m_var(1, 2).val());
}
TEST(AgradRevMatrix, assign_error) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::assign;
  using std::vector;

  std::vector<double> y_dbl(2);
  y_dbl[0] = 2.0;
  y_dbl[1] = 3.0;

  std::vector<stan::math::var> y_var(3);
  EXPECT_THROW(assign(y_var, y_dbl), std::invalid_argument);

  Matrix<double, Dynamic, 1> v_dbl(6);
  v_dbl << 1, 2, 3, 4, 5, 6;
  Matrix<stan::math::var, Dynamic, 1> v_var(5);
  EXPECT_THROW(assign(v_var, v_dbl), std::invalid_argument);

  Matrix<double, 1, Dynamic> rv_dbl(3);
  rv_dbl << 2, 4, 6;
  Matrix<stan::math::var, 1, Dynamic> rv_var(6);
  EXPECT_THROW(assign(rv_var, rv_dbl), std::invalid_argument);

  Matrix<double, Dynamic, Dynamic> m_dbl(2, 3);
  m_dbl << 2, 4, 6, 100, 200, 300;
  Matrix<stan::math::var, Dynamic, Dynamic> m_var(1, 1);
  EXPECT_THROW(assign(m_var, m_dbl), std::invalid_argument);
}

TEST(MathAssign, VarDouble) {
  using stan::math::assign;
  using stan::math::var;
  var x;
  double y = 10.1;
  assign(x, y);
  EXPECT_FLOAT_EQ(10.1, x.val());
}
