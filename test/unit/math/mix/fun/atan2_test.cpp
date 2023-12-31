#include <test/unit/math/test_ad.hpp>
#include <limits>
#include <vector>

TEST(mathMixCore, atan2) {
  auto f = [](const auto& x1, const auto& x2) {
    using stan::math::atan2;
    return atan2(x1, x2);
  };
  double nan = std::numeric_limits<double>::quiet_NaN();
  stan::test::expect_ad(f, nan, 1.0);
  stan::test::expect_ad(f, 1.0, nan);
  stan::test::expect_ad(f, nan, nan);
  stan::test::expect_ad(f, 1.0, 1.0);
  stan::test::expect_ad(f, 1.5, 1.5);
  stan::test::expect_ad(f, 1.2, 3.9);
  stan::test::expect_ad(f, 0.5, 2.3);
}

TEST(mathMixScalFun, atan2) {
  auto f = [](const auto& x1, const auto& x2) {
    using stan::math::atan2;
    return atan2(x1, x2);
  };

  // finite differences fails for
  // infinite inputs
  // stan::test::expect_common_nonzero_binary(f);

  stan::test::expect_ad(f, 1.0, 1.0);
  stan::test::expect_ad(f, 1.0, 0.5);
  stan::test::expect_ad(f, 1.2, 3.9);
  stan::test::expect_ad(f, 7.5, 1.8);

  Eigen::VectorXd in1(3);
  in1 << 1.0, 1.0, 1.2;
  Eigen::VectorXd in2(3);
  in2 << 1.0, 0.5, 3.9;
  stan::test::expect_ad_vectorized_binary(f, in1, in2);
}

TEST(mathMixScalFun, atan2_varmat) {
  auto f = [](const auto& x1, const auto& x2) {
    using stan::math::atan2;
    return atan2(x1, x2);
  };

  Eigen::VectorXd in1(3);
  in1 << 0.5, 3.4, 5.2;
  Eigen::VectorXd in2(3);
  in2 << 3.3, 0.9, 6.7;
  stan::test::expect_ad_matvar(f, in1, in2);
  stan::test::expect_ad_matvar(f, in1(0), in2);
  stan::test::expect_ad_matvar(f, in1, in2(0));
}
