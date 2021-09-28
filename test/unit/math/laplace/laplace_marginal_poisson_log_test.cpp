#include <stan/math.hpp>
#include <stan/math/laplace/laplace.hpp>

#include <test/unit/math/laplace/laplace_utility.hpp>
#include <test/unit/math/rev/fun/util.hpp>

#include <gtest/gtest.h>
#include <iostream>
#include <istream>
#include <fstream>
#include <vector>


TEST(laplace, likelihood_differentiation) {
  using stan::math::diff_poisson_log;
  using stan::math::to_vector;

  Eigen::VectorXd theta(2);
  theta << 1, 1;
  std::vector<int> n_samples = {1, 1};
  std::vector<int> sums = {1, 0};
  Eigen::VectorXd eta_dummy;

  diff_poisson_log diff_functor(to_vector(n_samples),
                                to_vector(sums));
  double log_density = diff_functor.log_likelihood(theta, eta_dummy);
  Eigen::VectorXd gradient;
  Eigen::SparseMatrix<double> hessian;
  diff_functor.diff(theta, eta_dummy, gradient, hessian);
  Eigen::VectorXd third_tensor = diff_functor.third_diff(theta, eta_dummy);

  EXPECT_FLOAT_EQ(-4.436564, log_density);
  EXPECT_FLOAT_EQ(-1.718282, gradient(0));
  EXPECT_FLOAT_EQ(-2.718282, gradient(1));
  EXPECT_FLOAT_EQ(-2.718282, hessian.coeff(0, 0));
  EXPECT_FLOAT_EQ(-2.718282, hessian.coeff(1, 1));
  EXPECT_FLOAT_EQ(-2.718282, third_tensor(0));
  EXPECT_FLOAT_EQ(-2.718282, third_tensor(1));
}


TEST(laplace, likelihood_differentiation2) {
  // Test exposure argument
  using stan::math::diff_poisson_log;
  using stan::math::to_vector;

  Eigen::VectorXd theta(2);
  theta << 1, 1;
  std::vector<int> n_samples = {1, 1};
  std::vector<int> sums = {1, 0};
  std::vector<double> log_exposure = {log(0.5), log(2)};
  Eigen::VectorXd eta_dummy;

  diff_poisson_log diff_functor(to_vector(n_samples),
                                to_vector(sums),
                                to_vector(log_exposure));

  double log_density = diff_functor.log_likelihood(theta, eta_dummy);
  Eigen::VectorXd gradient;
  Eigen::SparseMatrix<double> hessian;
  diff_functor.diff(theta, eta_dummy, gradient, hessian);
  Eigen::VectorXd third_tensor = diff_functor.third_diff(theta, eta_dummy);

  EXPECT_FLOAT_EQ(-6.488852, log_density);
  EXPECT_FLOAT_EQ(-0.3591409, gradient(0));
  EXPECT_FLOAT_EQ(-5.4365637, gradient(1));
  EXPECT_FLOAT_EQ(-1.359141, hessian.coeff(0, 0));
  EXPECT_FLOAT_EQ(-5.436564, hessian.coeff(1, 1));
  EXPECT_FLOAT_EQ(-1.359141, third_tensor(0));
  EXPECT_FLOAT_EQ(-5.436564, third_tensor(1));

}

TEST(laplace, poisson_lgm_dim2) {
  using stan::math::laplace_marginal_poisson_log_lpmf;
  using stan::math::var;
  using stan::math::to_vector;
  using stan::math::value_of;

  int dim_phi = 2;
  Eigen::Matrix<var, Eigen::Dynamic, 1> phi(dim_phi);
  phi << 1.6, 0.45;

  int dim_theta = 2;
  Eigen::VectorXd theta_0(dim_theta);
  theta_0 << 0, 0;

  int dim_x = 2;
  std::vector<Eigen::VectorXd> x(dim_theta);
  Eigen::VectorXd x_0(2);
  x_0 <<  0.05100797, 0.16086164;
  Eigen::VectorXd x_1(2);
  x_1 << -0.59823393, 0.98701425;
  x[0] = x_0;
  x[1] = x_1;

  std::vector<double> delta;
  std::vector<int> delta_int;

  std::vector<int> n_samples = {1, 1};
  std::vector<int> sums = {1, 0};

  stan::math::test::squared_kernel_functor K;
  var target
    = laplace_marginal_poisson_log_lpmf(sums, n_samples, K, phi, x, delta,
                                        delta_int, theta_0);

  // Test with exposure argument
  Eigen::VectorXd ye(2);
  ye << 1, 1;
  target = laplace_marginal_poisson_log_lpmf(sums, n_samples, ye, K, phi, x, delta,
                                        delta_int, theta_0);

  // How to test this? The best way would be to generate a few
  // benchmarks using gpstuff.
  std::vector<double> g;
  std::vector<stan::math::var> parm_vec{phi(0), phi(1)};
  target.grad(parm_vec, g);

  // finite diff test
  double diff = 1e-7;
  Eigen::VectorXd phi_dbl = value_of(phi);
  Eigen::VectorXd phi_1l = phi_dbl, phi_1u = phi_dbl,
    phi_2l = phi_dbl, phi_2u = phi_dbl;
  phi_1l(0) -= diff;
  phi_1u(0) += diff;
  phi_2l(1) -= diff;
  phi_2u(1) += diff;

  double target_1u = laplace_marginal_poisson_log_lpmf(sums, n_samples, K, phi_1u, x,
                                                  delta, delta_int, theta_0),
         target_1l = laplace_marginal_poisson_log_lpmf(sums, n_samples, K, phi_1l, x,
                                              delta, delta_int, theta_0),
         target_2u = laplace_marginal_poisson_log_lpmf(sums, n_samples, K, phi_2u, x,
                                              delta, delta_int, theta_0),
         target_2l = laplace_marginal_poisson_log_lpmf(sums, n_samples, K, phi_2l, x,
                                              delta, delta_int, theta_0);

  std::vector<double>g_finite(dim_phi);
  g_finite[0] = (target_1u - target_1l) / (2 * diff);
  g_finite[1] = (target_2u - target_2l) / (2 * diff);

  double tol = 1.1e-4;
  EXPECT_NEAR(g_finite[0], g[0], tol);
  EXPECT_NEAR(g_finite[1], g[1], tol);
}
