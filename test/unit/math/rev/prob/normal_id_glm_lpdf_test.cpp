#include <stan/math/rev.hpp>
#include <stan/math/prim.hpp>
#include <test/unit/math/rev/util.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <cmath>

//  We check that the values of the new regression match those of one built
//  from existing primitives.
TEST(ProbDistributionsNormalIdGLM, glm_matches_normal_id_doubles) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  Matrix<double, Dynamic, 1> y(3, 1);
  y << 51, 32, 12;
  Matrix<double, Dynamic, Dynamic> x(3, 2);
  x << -12, 46, -42, 24, 25, 27;
  Matrix<double, Dynamic, 1> beta(2, 1);
  beta << 0.3, 2;
  double alpha = 0.3;
  Matrix<double, Dynamic, 1> alphavec = alpha * Matrix<double, 3, 1>::Ones();
  Matrix<double, Dynamic, 1> theta(3, 1);
  theta = x * beta + alphavec;
  double sigma = 10;
  EXPECT_FLOAT_EQ((stan::math::normal_lpdf(y, theta, sigma)),
                  (stan::math::normal_id_glm_lpdf(y, x, alpha, beta, sigma)));
  EXPECT_FLOAT_EQ(
      (stan::math::normal_lpdf<true>(y, theta, sigma)),
      (stan::math::normal_id_glm_lpdf<true>(y, x, alpha, beta, sigma)));
}
//  We check that the values of the new regression match those of one built
//  from existing primitives.
TEST(ProbDistributionsNormalIdGLM, glm_matches_normal_id_doubles_rand) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  for (size_t ii = 0; ii < 200; ii++) {
    Matrix<double, Dynamic, 1> y(3, 1);
    for (size_t i = 0; i < 3; i++) {
      y[i] = Matrix<unsigned int, 1, 1>::Random(1, 1)[0] % 200;
    }
    Matrix<double, Dynamic, Dynamic> x
        = Matrix<double, Dynamic, Dynamic>::Random(3, 2);
    Matrix<double, Dynamic, 1> beta
        = Matrix<double, Dynamic, Dynamic>::Random(2, 1);
    Matrix<double, 1, 1> alphamat = Matrix<double, 1, 1>::Random(1, 1);
    double alpha = alphamat[0];
    Matrix<double, Dynamic, 1> alphavec = alpha * Matrix<double, 3, 1>::Ones();
    Matrix<double, Dynamic, 1> theta(3, 1);
    theta = x * beta + alphavec;
    Matrix<double, Dynamic, 1> phi
        = Matrix<double, Dynamic, Dynamic>::Random(1, 1)
          + Matrix<double, 1, 1>::Ones();
    EXPECT_FLOAT_EQ(
        (stan::math::normal_lpdf(y, theta, phi[0])),
        (stan::math::normal_id_glm_lpdf(y, x, alpha, beta, phi[0])));
    EXPECT_FLOAT_EQ(
        (stan::math::normal_lpdf<true>(y, theta, phi[0])),
        (stan::math::normal_id_glm_lpdf<true>(y, x, alpha, beta, phi[0])));
  }
}

template <class T>
class ProbDistributionsNormalIdGLM
    : public stan::math::test::VarMatrixTypedTests<T> {};
// For testing var_value<Matrix> and Matrix<var>
TYPED_TEST_SUITE(ProbDistributionsNormalIdGLM, stan::math::test::VarMatImpls);

//  We check that the gradients of the new regression match those of one built
//  from existing primitives.
TYPED_TEST(ProbDistributionsNormalIdGLM, glm_matches_normal_id_vars) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;
  Matrix<var, Dynamic, 1> y(3, 1);
  y << 14, 32, 21;
  Matrix<var, Dynamic, Dynamic> x(3, 2);
  x << -12, 46, -42, 24, 25, 27;
  Matrix<var, Dynamic, 1> beta(2, 1);
  beta << 0.3, 2;
  var alpha = 0.3;
  var sigma = 10;
  Matrix<var, Dynamic, 1> alphavec = alpha * Matrix<double, 3, 1>::Ones();
  Matrix<var, Dynamic, 1> theta(3, 1);
  theta = x * beta + alphavec;
  var lp = stan::math::normal_lpdf(y, theta, sigma);
  lp.grad();

  double lp_val = lp.val();
  double alpha_adj = alpha.adj();
  Matrix<double, Dynamic, Dynamic> x_adj(3, 2);
  Matrix<double, Dynamic, 1> beta_adj(2, 1);
  Matrix<double, Dynamic, 1> y_adj(3, 1);
  for (size_t i = 0; i < 2; i++) {
    beta_adj[i] = beta[i].adj();
    for (size_t j = 0; j < 3; j++) {
      x_adj(j, i) = x(j, i).adj();
    }
  }
  double sigma_adj = sigma.adj();
  for (size_t j = 0; j < 3; j++) {
    y_adj[j] = y[j].adj();
  }
  stan::math::recover_memory();
  Matrix<double, Dynamic, 1> y2_val(3, 1);
  y2_val << 14, 32, 21;
  vector_v y2 = y2_val;
  Matrix<double, Dynamic, Dynamic> x2_val(3, 2);
  x2_val << -12, 46, -42, 24, 25, 27;
  matrix_v x2 = x2_val;
  Matrix<double, Dynamic, 1> beta2_val(2, 1);
  beta2_val << 0.3, 2;
  vector_v beta2 = beta2_val;
  var alpha2 = 0.3;
  var sigma2 = 10;
  var lp2 = stan::math::normal_id_glm_lpdf(y2, x2, alpha2, beta2, sigma2);
  lp2.grad();
  EXPECT_FLOAT_EQ(lp_val, lp2.val());
  for (size_t i = 0; i < 2; i++) {
    EXPECT_FLOAT_EQ(beta_adj[i], beta2.adj()[i]);
  }
  EXPECT_FLOAT_EQ(alpha_adj, alpha2.adj());
  EXPECT_FLOAT_EQ(sigma_adj, sigma2.adj());
  for (size_t j = 0; j < 3; j++) {
    EXPECT_FLOAT_EQ(y_adj[j], y2.adj()[j]);
    for (size_t i = 0; i < 2; i++) {
      EXPECT_FLOAT_EQ(x_adj(j, i), x2.adj()(j, i));
    }
  }
}

TYPED_TEST(ProbDistributionsNormalIdGLM, glm_matches_normal_id_simple) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using vector_v = typename TypeParam::vector_v;

  double eps = 1e-9;
  Matrix<double, Dynamic, 1> y(3, 1);
  y << 14, 32, 21;
  Matrix<double, Dynamic, Dynamic> x(3, 2);
  x << -12, 46, -42, 24, 25, 27;
  Matrix<double, Dynamic, 1> beta(2, 1);
  beta << 0.3, 2;
  vector_v beta1 = beta;
  vector_v beta2 = beta;
  var alpha1 = 0.3;
  var alpha2 = 0.3;
  double sigma = 10;

  var lp1 = stan::math::normal_id_glm_lpdf(y, x, alpha1, beta1, sigma);
  var lp2 = stan::math::normal_lpdf(
      y, ((x * beta2).array() + alpha2).matrix().eval(), sigma);

  EXPECT_NEAR(lp1.val(), lp2.val(), eps);

  (lp1 + lp2).grad();

  for (int i = 0; i < 2; i++) {
    EXPECT_NEAR(beta1.adj()[i], beta2.adj()[i], eps);
  }
  EXPECT_NEAR(alpha1.adj(), alpha2.adj(), eps);
}

TYPED_TEST(ProbDistributionsNormalIdGLM, broadcast_x) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;
  using row_vector_v = typename TypeParam::row_vector_v;

  Matrix<double, Dynamic, 1> y(3, 1);
  y << 14, 32, 21;
  vector_v y1 = y;
  vector_v y2 = y;
  Matrix<double, 1, Dynamic> x(1, 2);
  x << -12, 46;
  row_vector_v x1 = x;
  matrix_v x_mat = x.replicate(3, 1);
  Matrix<double, Dynamic, 1> beta(2, 1);
  beta << 0.3, 2;
  vector_v beta1 = beta;
  vector_v beta2 = beta;
  var alpha1 = 0.3;
  var alpha2 = 0.3;
  var sigma1 = 10;
  var sigma2 = 10;

  var lp1 = stan::math::normal_id_glm_lpdf(y1, x1, alpha1, beta1, sigma1);
  var lp2 = stan::math::normal_id_glm_lpdf(y2, x_mat, alpha2, beta2, sigma2);

  EXPECT_DOUBLE_EQ(lp1.val(), lp2.val());

  (lp1 + lp2).grad();

  for (int i = 0; i < 3; i++) {
    EXPECT_DOUBLE_EQ(y1.adj()[i], y2.adj()[i]);
  }

  for (int i = 0; i < 2; i++) {
    EXPECT_DOUBLE_EQ(x1.adj()[i], x_mat.col(i).adj().sum());
    EXPECT_DOUBLE_EQ(beta1.adj()[i], beta2.adj()[i]);
  }
  EXPECT_DOUBLE_EQ(alpha1.adj(), alpha2.adj());
  EXPECT_DOUBLE_EQ(sigma1.adj(), sigma2.adj());
}

TYPED_TEST(ProbDistributionsNormalIdGLM, broadcast_y) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;

  double y = 13;
  var y1 = y;
  vector_v y_vec = Matrix<double, Dynamic, 1>::Constant(3, 1, y);
  Matrix<double, Dynamic, Dynamic> x(3, 2);
  x << -12, 46, -42, 24, 25, 27;
  matrix_v x1 = x;
  matrix_v x2 = x;
  Eigen::VectorXd beta(2, 1);
  beta << 0.3, 2;
  vector_v beta1 = beta;
  vector_v beta2 = beta;
  var alpha1 = 0.3;
  var alpha2 = 0.3;
  var sigma1 = 10;
  var sigma2 = 10;

  var lp1 = stan::math::normal_id_glm_lpdf(y1, x1, alpha1, beta1, sigma1);
  var lp2 = stan::math::normal_id_glm_lpdf(y_vec, x2, alpha2, beta2, sigma2);

  EXPECT_DOUBLE_EQ(lp1.val(), lp2.val());

  (lp1 + lp2).grad();

  EXPECT_DOUBLE_EQ(y1.adj(), y_vec.adj().sum());

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      EXPECT_DOUBLE_EQ(x1.adj()(j, i), x2.adj()(j, i));
    }
    EXPECT_DOUBLE_EQ(beta1.adj()[i], beta2.adj()[i]);
  }
  EXPECT_DOUBLE_EQ(alpha1.adj(), alpha2.adj());
  EXPECT_DOUBLE_EQ(sigma1.adj(), sigma2.adj());
}

TYPED_TEST(ProbDistributionsNormalIdGLM,
           glm_matches_normal_id_vars_zero_instances) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;

  Matrix<double, Dynamic, 1> y_val(0, 1);
  vector_v y = y_val;
  Matrix<double, Dynamic, Dynamic> x_val(0, 2);
  Matrix<double, Dynamic, 1> beta_val(2, 1);
  beta_val << 0.3, 2;
  vector_v beta = beta_val;
  var alpha = 0.3;
  var sigma = 10;
  Matrix<double, Dynamic, 1> theta_val(0, 1);
  vector_v theta = theta_val;
  var lp = stan::math::normal_lpdf(y, theta, sigma);
  lp.grad();

  double lp_val = lp.val();
  double alpha_adj = alpha.adj();
  Matrix<double, Dynamic, 1> beta_adj(2, 1);
  for (size_t i = 0; i < 2; i++) {
    beta_adj[i] = beta.adj()[i];
  }
  double sigma_adj = sigma.adj();

  stan::math::recover_memory();

  Matrix<double, Dynamic, 1> y2_val(0, 1);
  vector_v y2 = y2_val;
  Matrix<double, Dynamic, Dynamic> x2_val(0, 2);
  matrix_v x2 = x2_val;
  Matrix<double, Dynamic, 1> beta2_val(2, 1);
  beta2_val << 0.3, 2;
  vector_v beta2 = beta2_val;
  var alpha2 = 0.3;
  var sigma2 = 10;
  var lp2 = stan::math::normal_id_glm_lpdf(y2, x2, alpha2, beta2, sigma2);
  lp2.grad();
  EXPECT_FLOAT_EQ(lp_val, lp2.val());
  for (size_t i = 0; i < 2; i++) {
    EXPECT_FLOAT_EQ(beta_adj[i], beta2.adj()[i]);
  }
  EXPECT_FLOAT_EQ(alpha_adj, alpha2.adj());
  EXPECT_FLOAT_EQ(sigma_adj, sigma2.adj());
}

TYPED_TEST(ProbDistributionsNormalIdGLM,
           glm_matches_normal_id_vars_zero_attributes) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;

  Matrix<double, Dynamic, 1> y_val(3, 1);
  y_val << 14, 32, 21;
  vector_v y = y_val;
  Matrix<double, Dynamic, Dynamic> x_val(3, 0);
  matrix_v x = x_val;
  Matrix<double, Dynamic, 1> beta_val(0, 1);
  vector_v beta = beta_val;
  var alpha = 0.3;
  var sigma = 10;
  vector_v alphavec = stan::math::rep_vector<vector_v>(alpha, 3);
  vector_v theta = x * beta + alphavec;
  var lp = stan::math::normal_lpdf(y, theta, sigma);
  lp.grad();

  double lp_val = lp.val();
  double alpha_adj = alpha.adj();
  Matrix<double, Dynamic, 1> y_adj(3, 1);
  double sigma_adj = sigma.adj();
  for (size_t j = 0; j < 3; j++) {
    y_adj[j] = y.adj()[j];
  }

  stan::math::recover_memory();

  Matrix<double, Dynamic, 1> y2_val(3, 1);
  y2_val << 14, 32, 21;
  vector_v y2 = y2_val;
  Matrix<double, Dynamic, Dynamic> x2_val(3, 0);
  matrix_v x2 = x2_val;
  Matrix<double, Dynamic, 1> beta2_val(0, 1);
  vector_v beta2 = beta2_val;
  var alpha2 = 0.3;
  var sigma2 = 10;
  var lp2 = stan::math::normal_id_glm_lpdf(y2, x2, alpha2, beta2, sigma2);
  lp2.grad();
  EXPECT_FLOAT_EQ(lp_val, lp2.val());
  EXPECT_FLOAT_EQ(alpha_adj, alpha2.adj());
  EXPECT_FLOAT_EQ(sigma_adj, sigma2.adj());
  for (size_t j = 0; j < 3; j++) {
    EXPECT_FLOAT_EQ(y_adj[j], y2.adj()[j]);
  }
}

//  We check that the gradients of the new regression match those of one built
//  from existing primitives.
TYPED_TEST(ProbDistributionsNormalIdGLM, glm_matches_normal_id_vars_rand) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;

  for (size_t ii = 0; ii < 42; ii++) {
    Matrix<double, Dynamic, 1> yreal = Matrix<double, Dynamic, 1>::Random(3, 1);
    Matrix<double, Dynamic, Dynamic> xreal
        = Matrix<double, Dynamic, Dynamic>::Random(3, 2);
    Matrix<double, Dynamic, 1> betareal
        = Matrix<double, Dynamic, Dynamic>::Random(2, 1);
    Matrix<double, 1, 1> alphareal = Matrix<double, 1, 1>::Random(1, 1);
    double phireal = Matrix<double, Dynamic, 1>::Random(1, 1)[0] + 1;
    Matrix<var, Dynamic, 1> y = yreal;
    Matrix<var, Dynamic, 1> beta = betareal;
    Matrix<var, Dynamic, 1> theta(3, 1);
    Matrix<var, Dynamic, Dynamic> x = xreal;
    var alpha = alphareal[0];
    Matrix<var, Dynamic, 1> alphavec = Matrix<double, 3, 1>::Ones() * alpha;
    var phi = phireal;
    theta = (x * beta) + alphavec;
    var lp = stan::math::normal_lpdf(y, theta, phi);
    lp.grad();

    double lp_val = lp.val();
    double alpha_adj = alpha.adj();
    Matrix<double, Dynamic, Dynamic> x_adj(3, 2);
    Matrix<double, Dynamic, 1> beta_adj(2, 1);
    Matrix<double, Dynamic, 1> y_adj(3, 1);
    for (size_t i = 0; i < 2; i++) {
      beta_adj[i] = beta[i].adj();
      for (size_t j = 0; j < 3; j++) {
        x_adj(j, i) = x(j, i).adj();
      }
    }
    double phi_adj = phi.adj();
    for (size_t j = 0; j < 3; j++) {
      y_adj[j] = y[j].adj();
    }

    stan::math::recover_memory();

    vector_v y2 = yreal;
    vector_v beta2 = betareal;
    matrix_v x2 = xreal;
    var alpha2 = alphareal[0];
    var phi2 = phireal;
    var lp2 = stan::math::normal_id_glm_lpdf(y2, x2, alpha2, beta2, phi2);
    lp2.grad();
    EXPECT_FLOAT_EQ(lp_val, lp2.val());
    for (size_t i = 0; i < 2; i++) {
      EXPECT_FLOAT_EQ(beta_adj[i], beta2.adj()[i]);
    }
    EXPECT_FLOAT_EQ(alpha_adj, alpha2.adj());
    EXPECT_FLOAT_EQ(phi_adj, phi2.adj());
    for (size_t j = 0; j < 3; j++) {
      EXPECT_FLOAT_EQ(y_adj[j], y2.adj()[j]);
      for (size_t i = 0; i < 2; i++) {
        EXPECT_FLOAT_EQ(x_adj(j, i), x2.adj()(j, i));
      }
    }
  }
}

//  We check that the gradients of the new regression match those of one built
//  from existing primitives, in case beta is a scalar.
TYPED_TEST(ProbDistributionsNormalIdGLM,
           glm_matches_normal_id_vars_rand_scal_beta) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;

  for (size_t ii = 0; ii < 42; ii++) {
    Matrix<double, Dynamic, 1> yreal = Matrix<double, Dynamic, 1>::Random(3, 1);
    Matrix<double, Dynamic, Dynamic> xreal
        = Matrix<double, Dynamic, Dynamic>::Random(3, 1);
    double betareal = Matrix<double, Dynamic, Dynamic>::Random(1, 1)(0, 0);
    Matrix<double, 1, 1> alphareal = Matrix<double, 1, 1>::Random(1, 1);
    double phireal = Matrix<double, Dynamic, 1>::Random(1, 1)[0] + 1;
    Matrix<var, Dynamic, 1> y = yreal;
    var beta = betareal;
    Matrix<var, Dynamic, 1> theta(3, 1);
    Matrix<var, Dynamic, Dynamic> x = xreal;
    var alpha = alphareal[0];
    Matrix<var, Dynamic, 1> alphavec = Matrix<double, 3, 1>::Ones() * alpha;
    var phi = phireal;
    theta = (x * beta) + alphavec;
    var lp = stan::math::normal_lpdf(y, theta, phi);
    lp.grad();

    double lp_val = lp.val();
    double alpha_adj = alpha.adj();
    Matrix<double, Dynamic, Dynamic> x_adj(3, 1);
    Matrix<double, Dynamic, 1> y_adj(3, 1);
    double beta_adj = beta.adj();
    for (size_t j = 0; j < 3; j++) {
      x_adj(j, 0) = x(j, 0).adj();
    }
    double phi_adj = phi.adj();
    for (size_t j = 0; j < 3; j++) {
      y_adj[j] = y[j].adj();
    }

    stan::math::recover_memory();
    vector_v y2 = yreal;
    var beta2 = betareal;
    matrix_v x2 = xreal;
    var alpha2 = alphareal[0];
    var phi2 = phireal;
    var lp2 = stan::math::normal_id_glm_lpdf(y2, x2, alpha2, beta2, phi2);
    lp2.grad();
    EXPECT_FLOAT_EQ(lp_val, lp2.val());
    EXPECT_FLOAT_EQ(beta_adj, beta2.adj());
    EXPECT_FLOAT_EQ(alpha_adj, alpha2.adj());
    EXPECT_FLOAT_EQ(phi_adj, phi2.adj());
    for (size_t j = 0; j < 3; j++) {
      EXPECT_FLOAT_EQ(y_adj[j], y2.adj()[j]);
      EXPECT_FLOAT_EQ(x_adj(j, 0), x2.adj()(j, 0));
    }
  }
}

//  We check that the gradients of the new regression match those of one built
//  from existing primitives.
TYPED_TEST(ProbDistributionsNormalIdGLM,
           glm_matches_normal_id_varying_intercept) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;

  for (size_t ii = 0; ii < 42; ii++) {
    Matrix<double, Dynamic, 1> yreal = Matrix<double, Dynamic, 1>::Random(3, 1);
    Matrix<double, Dynamic, Dynamic> xreal
        = Matrix<double, Dynamic, Dynamic>::Random(3, 2);
    Matrix<double, Dynamic, 1> betareal
        = Matrix<double, Dynamic, Dynamic>::Random(2, 1);
    Matrix<double, Dynamic, 1> alphareal
        = Matrix<double, Dynamic, 1>::Random(3, 1);
    double phireal = Matrix<double, Dynamic, 1>::Random(3, 1)[0] + 1;
    Matrix<var, Dynamic, 1> y = yreal;
    Matrix<var, Dynamic, 1> beta = betareal;
    Matrix<var, Dynamic, 1> theta(3, 1);
    Matrix<var, Dynamic, Dynamic> x = xreal;
    Matrix<var, Dynamic, 1> alpha = alphareal;
    var phi = phireal;
    theta = (x * beta) + alpha;
    var lp = stan::math::normal_lpdf(y, theta, phi);
    lp.grad();

    double lp_val = lp.val();
    Matrix<double, Dynamic, 1> alpha_adj(3, 1);
    Matrix<double, Dynamic, Dynamic> x_adj(3, 2);
    Matrix<double, Dynamic, 1> beta_adj(2, 1);
    double phi_adj = phi.adj();
    Matrix<double, Dynamic, 1> y_adj(3, 1);
    for (size_t i = 0; i < 2; i++) {
      beta_adj[i] = beta[i].adj();
      for (size_t j = 0; j < 3; j++) {
        x_adj(j, i) = x(j, i).adj();
      }
    }
    for (size_t j = 0; j < 3; j++) {
      alpha_adj[j] = alpha[j].adj();
      y_adj[j] = y[j].adj();
    }

    stan::math::recover_memory();

    vector_v y2 = yreal;
    vector_v beta2 = betareal;
    matrix_v x2 = xreal;
    vector_v alpha2 = alphareal;
    var phi2 = phireal;
    var lp2 = stan::math::normal_id_glm_lpdf(y2, x2, alpha2, beta2, phi2);
    lp2.grad();
    EXPECT_FLOAT_EQ(lp_val, lp2.val());
    EXPECT_FLOAT_EQ(phi_adj, phi2.adj());
    for (size_t i = 0; i < 2; i++) {
      EXPECT_FLOAT_EQ(beta_adj[i], beta2.adj()[i]);
    }
    for (size_t j = 0; j < 3; j++) {
      EXPECT_FLOAT_EQ(alpha_adj[j], alpha2.adj()[j]);
      EXPECT_FLOAT_EQ(y_adj[j], y2.adj()[j]);
      for (size_t i = 0; i < 2; i++) {
        EXPECT_FLOAT_EQ(x_adj(j, i), x2.adj()(j, i));
      }
    }
  }
}

//  We check that the gradients of the new regression match those of one built
//  from existing primitives.
TYPED_TEST(ProbDistributionsNormalIdGLM,
           glm_matches_normal_id_varying_intercept_and_scale) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;

  for (size_t ii = 0; ii < 42; ii++) {
    Matrix<double, Dynamic, 1> yreal = Matrix<double, Dynamic, 1>::Random(3, 1);
    Matrix<double, Dynamic, Dynamic> xreal
        = Matrix<double, Dynamic, Dynamic>::Random(3, 2);
    Matrix<double, Dynamic, 1> betareal
        = Matrix<double, Dynamic, Dynamic>::Random(2, 1);
    Matrix<double, Dynamic, 1> alphareal
        = Matrix<double, Dynamic, 1>::Random(3, 1);
    Matrix<double, Dynamic, 1> phireal
        = Matrix<double, Dynamic, Dynamic>::Random(3, 1)
          + Matrix<double, Dynamic, 1>::Ones(3, 1);
    Matrix<var, Dynamic, 1> y = yreal;
    Matrix<var, Dynamic, 1> beta = betareal;
    Matrix<var, Dynamic, 1> theta(3, 1);
    Matrix<var, Dynamic, Dynamic> x = xreal;
    Matrix<var, Dynamic, 1> alpha = alphareal;
    Matrix<var, Dynamic, 1> phi = phireal;
    theta = (x * beta) + alpha;
    var lp = stan::math::normal_lpdf(y, theta, phi);
    lp.grad();

    double lp_val = lp.val();
    Matrix<double, Dynamic, 1> alpha_adj(3, 1);
    Matrix<double, Dynamic, Dynamic> x_adj(3, 2);
    Matrix<double, Dynamic, 1> beta_adj(2, 1);
    Matrix<double, Dynamic, 1> phi_adj(3, 1);
    Matrix<double, Dynamic, 1> y_adj(3, 1);
    for (size_t i = 0; i < 2; i++) {
      beta_adj[i] = beta[i].adj();
      for (size_t j = 0; j < 3; j++) {
        x_adj(j, i) = x(j, i).adj();
      }
    }
    for (size_t j = 0; j < 3; j++) {
      alpha_adj[j] = alpha[j].adj();
      phi_adj[j] = phi[j].adj();
      y_adj[j] = y[j].adj();
    }

    stan::math::recover_memory();
    vector_v y2 = yreal;
    vector_v beta2 = betareal;
    matrix_v x2 = xreal;
    vector_v alpha2 = alphareal;
    vector_v phi2 = phireal;
    var lp2 = stan::math::normal_id_glm_lpdf(y2, x2, alpha2, beta2, phi2);
    lp2.grad();
    EXPECT_FLOAT_EQ(lp_val, lp2.val());
    for (size_t i = 0; i < 2; i++) {
      EXPECT_FLOAT_EQ(beta_adj[i], beta2.adj()[i]);
    }
    for (size_t j = 0; j < 3; j++) {
      EXPECT_FLOAT_EQ(alpha_adj[j], alpha2.adj()[j]);
      EXPECT_FLOAT_EQ(y_adj[j], y2.adj()[j]);
      EXPECT_FLOAT_EQ(phi_adj[j], phi2.adj()[j]);
      for (size_t i = 0; i < 2; i++) {
        EXPECT_FLOAT_EQ(x_adj(j, i), x2.adj()(j, i));
      }
    }
  }
}

//  We check that we can instantiate all different interface types.
TYPED_TEST(ProbDistributionsNormalIdGLM,
           glm_matches_normal_id_interface_types) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  using matrix_v = typename TypeParam::matrix_v;
  using vector_v = typename TypeParam::vector_v;
  using row_vector_v = typename TypeParam::row_vector_v;

  double value = 0;
  double value2 = 0;

  double d = 1.0;
  std::vector<double> vd = {{1.0, 2.0}};
  Eigen::VectorXd ev(2);
  Eigen::RowVectorXd rv(2);
  Eigen::MatrixXd m1(1, 1);
  m1 << 1.0;
  Eigen::MatrixXd m(2, 2);
  ev << 1.0, 2.0;
  rv << 1.0, 2.0;
  m << 1.0, 2.0, 3.0, 4.0;

  value += stan::math::normal_id_glm_lpdf(d, m1, d, d, d);
  value += stan::math::normal_id_glm_lpdf(vd, m, vd, vd, vd);
  value += stan::math::normal_id_glm_lpdf(ev, m, ev, ev, ev);
  value += stan::math::normal_id_glm_lpdf(rv, m, rv, rv, rv);

  var v = 1.0;
  std::vector<var> vv = {{1.0, 2.0}};

  Eigen::Matrix<double, -1, 1> evv_val(2);
  evv_val << 1.0, 2.0;
  vector_v evv = evv_val;
  Eigen::Matrix<double, 1, -1> rvv_val(2);
  rvv_val << 1.0, 2.0;
  row_vector_v rvv = rvv_val;
  Eigen::Matrix<double, -1, -1> m1v_val(1, 1);
  m1v_val << 1.0;
  matrix_v m1v = m1v_val;
  Eigen::Matrix<double, -1, -1> mv_val(2, 2);
  mv_val << 1.0, 2.0, 3.0, 4.0;
  matrix_v mv = mv_val;

  value2 += stan::math::normal_id_glm_lpdf(v, m1v, v, v, v).val();
  value2 += stan::math::normal_id_glm_lpdf(vv, mv, vv, vv, vv).val();
  value2 += stan::math::normal_id_glm_lpdf(evv, mv, evv, evv, evv).val();
  value2 += stan::math::normal_id_glm_lpdf(rvv, mv, rvv, rvv, rvv).val();

  EXPECT_FLOAT_EQ(value, value2);
}

//  We check that the right errors are thrown.
TEST(ProbDistributionsNormalIdGLM, glm_matches_normal_id_error_checking) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::math::var;
  int N = 3;
  int M = 2;
  int W = 4;

  Eigen::Matrix<double, -1, 1> y = Eigen::Matrix<double, -1, 1>::Random(N, 1);
  Eigen::Matrix<double, -1, 1> yw1 = Eigen::Matrix<double, -1, 1>::Random(W, 1);
  Eigen::Matrix<double, -1, 1> yw2
      = Eigen::Matrix<double, -1, 1>::Random(N, 1) * NAN;
  Eigen::Matrix<double, -1, -1> x = Eigen::Matrix<double, -1, -1>::Random(N, M);
  Eigen::Matrix<double, -1, -1> xw1
      = Eigen::Matrix<double, -1, -1>::Random(W, M);
  Eigen::Matrix<double, -1, -1> xw2
      = Eigen::Matrix<double, -1, -1>::Random(N, W);
  Eigen::Matrix<double, -1, -1> xw3
      = Eigen::Matrix<double, -1, -1>::Random(N, M) * NAN;
  Eigen::Matrix<double, -1, 1> alpha
      = Eigen::Matrix<double, -1, 1>::Random(N, 1);
  Eigen::Matrix<double, -1, 1> alphaw1
      = Eigen::Matrix<double, -1, 1>::Random(W, 1);
  Eigen::Matrix<double, -1, 1> alphaw2
      = Eigen::Matrix<double, -1, 1>::Random(N, 1) * NAN;
  Eigen::Matrix<double, -1, 1> beta
      = Eigen::Matrix<double, -1, 1>::Random(M, 1);
  Eigen::Matrix<double, -1, 1> betaw1
      = Eigen::Matrix<double, -1, 1>::Random(W, 1);
  Eigen::Matrix<double, -1, 1> betaw2
      = Eigen::Matrix<double, -1, 1>::Random(M, 1) * NAN;
  Eigen::Matrix<double, -1, 1> sigma
      = Eigen::Matrix<double, -1, 1>::Random(N, 1)
        + Eigen::Matrix<double, -1, 1>::Ones(N, 1);
  Eigen::Matrix<double, -1, 1> sigmaw1
      = Eigen::Matrix<double, -1, 1>::Random(W, 1)
        + Eigen::Matrix<double, -1, 1>::Ones(W, 1);
  Eigen::Matrix<double, -1, 1> sigmaw2
      = Eigen::Matrix<double, -1, 1>::Random(N, 1)
        - Eigen::Matrix<double, -1, 1>::Ones(N, 1);
  Eigen::Matrix<double, -1, 1> sigmaw3
      = (Eigen::Matrix<double, -1, 1>::Random(N, 1)
         + Eigen::Matrix<double, -1, 1>::Ones(N, 1))
        * NAN;

  EXPECT_THROW(stan::math::normal_id_glm_lpdf(yw1, x, alpha, beta, sigma),
               std::invalid_argument);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(yw2, x, alpha, beta, sigma),
               std::domain_error);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, xw1, alpha, beta, sigma),
               std::invalid_argument);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, xw2, alpha, beta, sigma),
               std::invalid_argument);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, xw3, alpha, beta, sigma),
               std::domain_error);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, x, alphaw1, beta, sigma),
               std::invalid_argument);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, x, alphaw2, beta, sigma),
               std::domain_error);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, x, alpha, betaw1, sigma),
               std::invalid_argument);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, x, alpha, betaw2, sigma),
               std::domain_error);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, x, alpha, beta, sigmaw1),
               std::invalid_argument);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, x, alpha, beta, sigmaw2),
               std::domain_error);
  EXPECT_THROW(stan::math::normal_id_glm_lpdf(y, x, alpha, beta, sigmaw3),
               std::domain_error);
}
