#include <stan/math.hpp>
//#include <stan/math/laplace/laplace_likelihood.hpp>
#include <stan/math/laplace/laplace.hpp>
#include <test/unit/util.hpp>
#include <test/unit/math/rev/fun/util.hpp>
#include <test/unit/math/laplace/laplace_utility.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <istream>
#include <fstream>
#include <vector>

TEST(laplace, likelihood_differentiation) {
  using stan::math::diff_neg_binomial_2_log;
  using stan::math::var;

  Eigen::VectorXd theta(2);
  theta << 1, 1;
  int n_theta = theta.size();
  Eigen::VectorXd eta(1);
  eta << 1.2;

  Eigen::VectorXd y(2);
  y << 0, 1;
  std::vector<int> y_index(2);
  y_index[0] = 0;
  y_index[1] = 1;

  // Eigen::Matrix<var, Eigen::Dynamic, 1> theta_v = theta;
  diff_neg_binomial_2_log diff_functor(y, y_index, n_theta);

  double log_density = diff_functor.log_likelihood(theta, eta);

  // benchmark against R
  EXPECT_FLOAT_EQ(-3.023328, log_density);

  Eigen::VectorXd gradient;
  Eigen::SparseMatrix<double> hessian;
  diff_functor.diff(theta, eta, gradient, hessian);

  Eigen::VectorXd third_diff = diff_functor.third_diff(theta, eta);

  // Benchmark against finite diff
  double epsilon = 1e-6;
  Eigen::VectorXd theta_l0 = theta, theta_u0 = theta, theta_l1 = theta,
                  theta_u1 = theta;
  theta_u0(0) += epsilon;
  theta_l0(0) -= epsilon;
  theta_u1(1) += epsilon;
  theta_l1(1) -= epsilon;

  Eigen::VectorXd finite_gradient(2);
  finite_gradient(0) = (diff_functor.log_likelihood(theta_u0, eta)
                        - diff_functor.log_likelihood(theta_l0, eta))
                       / (2 * epsilon);

  finite_gradient(1) = (diff_functor.log_likelihood(theta_u1, eta)
                        - diff_functor.log_likelihood(theta_l1, eta))
                       / (2 * epsilon);

  Eigen::VectorXd gradient_l0, gradient_u0, gradient_l1, gradient_u1;
  Eigen::SparseMatrix<double> hessian_l0, hessian_u0, hessian_l1, hessian_u1;
  diff_functor.diff(theta_l0, eta, gradient_l0, hessian_l0);
  diff_functor.diff(theta_u0, eta, gradient_u0, hessian_u0);
  diff_functor.diff(theta_l1, eta, gradient_l1, hessian_l1);
  diff_functor.diff(theta_u1, eta, gradient_u1, hessian_u1);

  Eigen::VectorXd finite_hessian(2);
  finite_hessian(0) = (gradient_u0 - gradient_l0)(0) / (2 * epsilon);
  finite_hessian(1) = (gradient_u1 - gradient_l1)(1) / (2 * epsilon);

  Eigen::VectorXd finite_third_diff(2);
  finite_third_diff(0)
      = (hessian_u0 - hessian_l0).eval().coeff(0, 0) / (2 * epsilon);
  finite_third_diff(1)
      = (hessian_u1 - hessian_l1).eval().coeff(1, 1) / (2 * epsilon);

  std::cout << "gradient: " << gradient << std::endl;
  std::cout << "hessian: " << hessian << std::endl;
  std::cout << "third_diff: " << third_diff << std::endl;

  EXPECT_FLOAT_EQ(finite_gradient(0), gradient(0));
  EXPECT_FLOAT_EQ(finite_gradient(1), gradient(1));
  EXPECT_FLOAT_EQ(finite_hessian(0), hessian.coeff(0, 0));
  EXPECT_FLOAT_EQ(finite_hessian(1), hessian.coeff(1, 1));
  EXPECT_FLOAT_EQ(finite_third_diff(0), third_diff(0));
  EXPECT_FLOAT_EQ(finite_third_diff(1), third_diff(1));

  // derivatives wrt eta
  Eigen::VectorXd diff_eta = diff_functor.diff_eta(theta, eta);

  Eigen::VectorXd eta_l(1), eta_u(1);
  eta_l(0) = eta(0) - epsilon;
  eta_u(0) = eta(0) + epsilon;
  double finite_gradient_eta = (diff_functor.log_likelihood(theta, eta_u)
                                - diff_functor.log_likelihood(theta, eta_l))
                               / (2 * epsilon);

  std::cout << "diff_eta: " << diff_eta.transpose() << std::endl;

  EXPECT_FLOAT_EQ(finite_gradient_eta, diff_eta(0));

  Eigen::MatrixXd diff_theta_eta = diff_functor.diff_theta_eta(theta, eta);

  Eigen::VectorXd gradient_theta_l, gradient_theta_u;
  Eigen::SparseMatrix<double> hessian_theta_u, hessian_theta_l;

  diff_functor.diff(theta, eta_l, gradient_theta_l, hessian_theta_l);
  diff_functor.diff(theta, eta_u, gradient_theta_u, hessian_theta_u);
  Eigen::VectorXd finite_gradient_theta_eta
      = (gradient_theta_u - gradient_theta_l) / (2 * epsilon);

  std::cout << "diff_theta_eta: " << diff_theta_eta.transpose() << std::endl;
  std::cout << "finite_gradient_theta_eta: "
            << finite_gradient_theta_eta.transpose() << std::endl;
  Eigen::VectorXd diff_theta_eta1 = diff_theta_eta.col(0);
  EXPECT_MATRIX_FLOAT_EQ(finite_gradient_theta_eta, diff_theta_eta1);
  /*
  EXPECT_FLOAT_EQ(finite_gradient_theta_eta(0), diff_theta_eta(0, 0));
  std::cout << "Got Here-1";
  EXPECT_FLOAT_EQ(finite_gradient_theta_eta(1), diff_theta_eta(1, 0));
*/
  // Eigen::VectorXd W_root = (-hessian).cwiseSqrt();
  Eigen::MatrixXd diff2_theta_eta = diff_functor.diff2_theta_eta(theta, eta);

  Eigen::VectorXd finite_hessian_theta_eta
      = (hessian_theta_u.diagonal() - hessian_theta_l.diagonal())
        / (2 * epsilon);

  std::cout << "diff2_theta_eta: " << diff2_theta_eta.transpose() << std::endl;

  EXPECT_FLOAT_EQ(finite_hessian_theta_eta(0), diff2_theta_eta(0, 0));
  EXPECT_FLOAT_EQ(finite_hessian_theta_eta(1), diff2_theta_eta(1, 0));
}

// unit tests for derivatives of B
template <typename T>
Eigen::MatrixXd compute_B(const Eigen::VectorXd& theta,
                          const Eigen::VectorXd& eta,
                          const Eigen::MatrixXd& covariance, T diff_functor) {
  int group_size = theta.size();
  Eigen::VectorXd l_grad, hessian;
  diff_functor.diff(theta, eta, l_grad, hessian);
  Eigen::VectorXd W_root = (-hessian).cwiseSqrt();

  return Eigen::MatrixXd::Identity(group_size, group_size)
         + stan::math::quad_form_diag(covariance, W_root);
}
/*
TEST(laplace, neg_binomial_2_log_dbl) {
  using stan::math::diff_neg_binomial_2_log;
  using stan::math::laplace_marginal_density;
  using stan::math::laplace_marginal_neg_binomial_2_log_lpmf;
  using stan::math::to_vector;
  using stan::math::value_of;
  using stan::math::var;
  using stan::math::test::sqr_exp_kernel_functor;

  int dim_phi = 2, dim_eta = 1, dim_theta = 2;
  Eigen::VectorXd phi(dim_phi), eta(dim_eta), theta_0(dim_theta);
  phi << 1.6, 0.45;
  eta << 1;
  theta_0 << 0, 0;
  std::vector<Eigen::VectorXd> x(dim_theta);
  Eigen::VectorXd x_0(2);
  x_0 << 0.05100797, 0.16086164;
  x[0] = x_0;
  Eigen::VectorXd x_1(2);
  x_1 << -0.59823393, 0.98701425;
  x[1] = x_1;

  std::vector<double> delta;
  std::vector<int> delta_int;
  std::vector<int> y_index = {0, 1};
  std::vector<int> y_obs = {1, 6};
  Eigen::VectorXd y = to_vector(y_obs);

  diff_neg_binomial_2_log diff_functor(y, y_index, dim_theta);
  stan::math::test::sqr_exp_kernel_functor K;

  std::cout << "here1" << std::endl;
  double log_p = laplace_marginal_density(diff_functor, K, phi, eta, x, delta,
                                          delta_int, theta_0);

  std::cout << "here2" << std::endl;
  Eigen::Matrix<var, Eigen::Dynamic, 1> phi_v = phi, eta_v = eta;

  std::cout << "here3" << std::endl;
  var target = laplace_marginal_density(diff_functor, K, phi_v, eta_v, x, delta,
                                        delta_int, theta_0);

  std::cout << "here4" << std::endl;
  std::vector<double> g;
  std::vector<stan::math::var> parm_vec{phi_v(0), phi_v(1), eta_v(0)};
  target.grad(parm_vec, g);

  // finite diff benchmark
  double diff = 1e-7;
  Eigen::VectorXd phi_dbl = value_of(phi), eta_dbl = value_of(eta);
  Eigen::VectorXd phi_1l = phi_dbl, phi_1u = phi_dbl, phi_2l = phi_dbl,
                  phi_2u = phi_dbl, eta_l = eta_dbl, eta_u = eta_dbl;
  phi_1l(0) -= diff;
  phi_1u(0) += diff;
  phi_2l(1) -= diff;
  phi_2u(1) += diff;
  eta_l(0) -= diff;
  eta_u(0) += diff;

  std::cout << "here5" << std::endl;
  double target_phi_1u = laplace_marginal_density(
      diff_functor, K, phi_1u, eta_dbl, x, delta, delta_int, theta_0);
  std::cout << "here6" << std::endl;
  double target_phi_1l = laplace_marginal_density(
      diff_functor, K, phi_1l, eta_dbl, x, delta, delta_int, theta_0);
  std::cout << "here7" << std::endl;
  double target_phi_2u = laplace_marginal_density(
      diff_functor, K, phi_2u, eta_dbl, x, delta, delta_int, theta_0);
  std::cout << "here8" << std::endl;
  double target_phi_2l = laplace_marginal_density(
      diff_functor, K, phi_2l, eta_dbl, x, delta, delta_int, theta_0);

  std::cout << "here9" << std::endl;
  double target_eta_u = laplace_marginal_density(
      diff_functor, K, phi_dbl, eta_u, x, delta, delta_int, theta_0);

  std::cout << "here10" << std::endl;
  double target_eta_l = laplace_marginal_density(
      diff_functor, K, phi_dbl, eta_l, x, delta, delta_int, theta_0);

  std::vector<double> g_finite(dim_phi + dim_eta);
  g_finite[0] = (target_phi_1u - target_phi_1l) / (2 * diff);
  g_finite[1] = (target_phi_2u - target_phi_2l) / (2 * diff);
  g_finite[2] = (target_eta_u - target_eta_l) / (2 * diff);

  double tol = 4e-6;
  EXPECT_NEAR(g_finite[0], g[0], tol);
  EXPECT_NEAR(g_finite[1], g[1], tol);
  EXPECT_NEAR(g_finite[2], g[2], tol);

  // Check wrapper.
  EXPECT_EQ(target,
            laplace_marginal_neg_binomial_2_log_lpmf(
                y_obs, y_index, K, phi, eta, x, delta, delta_int, theta_0));
}
*/
