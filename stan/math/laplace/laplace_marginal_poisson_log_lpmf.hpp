#ifndef STAN_MATH_LAPLACE_LAPLACE_MARGINAL_POISSON_LOG_LPMF_HPP
#define STAN_MATH_LAPLACE_LAPLACE_MARGINAL_POISSON_LOG_LPMF_HPP

#include <stan/math/laplace/laplace_marginal.hpp>
#include <stan/math/laplace/laplace_likelihood_poisson_log.hpp>
#include <stan/math/laplace/laplace_likelihood_general.hpp>

namespace stan {
namespace math {
/**
 * Wrapper function around the laplace_marginal function for
 * a log poisson likelihood. Returns the marginal density
 * p(y | phi) by marginalizing out the latent gaussian variable,
 * with a Laplace approximation. See the laplace_marginal function
 * for more details.
 *
 * @tparam T0 The type of the initial guess, theta_0.
 * @tparam T1 The type for the global parameter, phi.
 * @param[in] y total counts per group. Second sufficient statistics.
 * @param[in] n_samples number of samples per group. First sufficient
 *            statistics.
 * @param[in] covariance a function which returns the prior covariance.
 * @param[in] phi model parameters for the covariance functor.
 * @param[in] x data for the covariance functor.
 * @param[in] delta additional real data for the covariance functor.
 * @param[in] delta_int additional integer data for covariance functor.
 * @param[in] theta_0 the initial guess for the Laplace approximation.
 * @param[in] tolerance controls the convergence criterion when finding
 *            the mode in the Laplace approximation.
 * @param[in] max_num_steps maximum number of steps before the Newton solver
 *            breaks and returns an error.
 */
template <typename T0, typename T1, typename K>
T1 laplace_marginal_poisson_log_lpmf(
    const std::vector<int>& y, const std::vector<int>& n_samples,
    const K& covariance_function,
    const Eigen::Matrix<T1, Eigen::Dynamic, 1>& phi,
    const std::vector<Eigen::VectorXd>& x, const std::vector<double>& delta,
    const std::vector<int>& delta_int,
    const Eigen::Matrix<T0, Eigen::Dynamic, 1>& theta_0,
    std::ostream* msgs = nullptr, double tolerance = 1e-6,
    long int max_num_steps = 100) {
  // TODO: change this to a VectorXd once we have operands & partials.
  Eigen::Matrix<T1, Eigen::Dynamic, 1> eta_dummy(0);
  poisson_log_likelihood L;
  return laplace_marginal_density(
      diff_likelihood<poisson_log_likelihood>(L, to_vector(y), n_samples, msgs),
      covariance_function,
      phi, eta_dummy, x, delta, delta_int, theta_0, msgs, tolerance,
      max_num_steps);
}

template <typename T0, typename T1, typename K>
T1 laplace_marginal_poisson_log_lpmf(
    const std::vector<int>& y, const std::vector<int>& n_samples,
    const Eigen::VectorXd& ye, const K& covariance_function,
    const Eigen::Matrix<T1, Eigen::Dynamic, 1>& phi,
    const std::vector<Eigen::VectorXd>& x, const std::vector<double>& delta,
    const std::vector<int>& delta_int,
    const Eigen::Matrix<T0, Eigen::Dynamic, 1>& theta_0,
    std::ostream* msgs = nullptr, double tolerance = 1e-6,
    long int max_num_steps = 100) {
  // TODO: change this to a VectorXd once we have operands & partials.
  Eigen::Matrix<T1, Eigen::Dynamic, 1> eta_dummy(0);
  Eigen::VectorXd y_vec = to_vector(y);
  Eigen::VectorXd y_and_ye(y_vec.size() + ye.size());
  y_and_ye << y_vec, ye;
  poisson_log_exposure_likelihood L;
  return laplace_marginal_density(
    diff_likelihood<poisson_log_exposure_likelihood>(L, y_and_ye, n_samples,
                                                     msgs),
      covariance_function, phi, eta_dummy, x, delta, delta_int, theta_0, msgs,
      tolerance, max_num_steps);
}

}  // namespace math
}  // namespace stan

#endif
