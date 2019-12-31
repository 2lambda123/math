#ifndef STAN_MATH_REV_SCAL_FUN_IBETA_HPP
#define STAN_MATH_REV_SCAL_FUN_IBETA_HPP

#include <stan/math/rev/meta.hpp>
#include <stan/math/rev/core.hpp>
#include <stan/math/prim/scal/fun/abs.hpp>
#include <stan/math/prim/scal/fun/digamma.hpp>
#include <stan/math/prim/scal/fun/ibeta.hpp>
#include <stan/math/prim/scal/fun/tgamma.hpp>
#include <cmath>

namespace stan {
namespace math {

namespace internal {
/**
 * Calculates the generalized hypergeometric 3F2(a, a, b; a + 1, a + 1; z).
 *
 * Handles negative values of b properly.
 */
inline double ibeta_hypergeometric_helper(double a, double b, double z,
                                          double precision = 1e-8,
                                          double max_steps = 1000) {
  double val = 0;
  double diff = 1;
  double k = 0;
  double a_2 = a * a;
  double bprod = 1;
  while (abs(diff) > precision && ++k < max_steps) {
    val += diff;
    bprod *= b + k - 1.0;
    diff = a_2 * std::pow(a + k, -2) * bprod * std::pow(z, k) / tgamma(k + 1);
  }
  return val;
}

class ibeta_vvv_vari : public op_vvv_vari {
 public:
  ibeta_vvv_vari(vari* avi, vari* bvi, vari* xvi)
      : op_vvv_vari(ibeta(avi->val_, bvi->val_, xvi->val_), avi, bvi, xvi) {}
  void chain() {
    double a = avi_->val_;
    double b = bvi_->val_;
    double c = cvi_->val_;

    using std::log;
    using std::pow;
    using std::sin;
    avi_->adj_ += adj_ * (log(c) - digamma(a) + digamma(a + b)) * val_
                  - tgamma(a) * tgamma(a + b) / tgamma(b) * pow(c, a)
                        / tgamma(1 + a) / tgamma(1 + a)
                        * ibeta_hypergeometric_helper(a, 1 - b, c);
    bvi_->adj_ += adj_
                  * (tgamma(b) * tgamma(a + b) / tgamma(a) * pow(1 - c, b)
                         * ibeta_hypergeometric_helper(b, 1 - a, 1 - c)
                         / tgamma(b + 1) / tgamma(b + 1)
                     + ibeta(b, a, 1 - c)
                           * (digamma(b) - digamma(a + b) - log(1 - c)));
    cvi_->adj_ += adj_ * boost::math::ibeta_derivative(a, b, c);
  }
};
}  // namespace internal

/**
 * The normalized incomplete beta function of a, b, and x.
 *
 * Used to compute the cumulative density function for the beta
 * distribution.
 *
 * Partial derivatives are those specified by wolfram alpha.
 * The values were checked using both finite differences and
 * by independent code for calculating the derivatives found
 * in JSS (paper by Boik and Robison-Cox).
 *
 * @param a Shape parameter.
 * @param b Shape parameter.
 * @param x Random variate.
 *
 * @return The normalized incomplete beta function.
 * @throws if any argument is NaN.
 */
inline var ibeta(const var& a, const var& b, const var& x) {
  return var(new internal::ibeta_vvv_vari(a.vi_, b.vi_, x.vi_));
}

}  // namespace math
}  // namespace stan
#endif
