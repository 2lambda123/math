#ifndef STAN_MATH_REV_FUNCTOR_ALGEBRA_SOLVER_POWELL_HPP
#define STAN_MATH_REV_FUNCTOR_ALGEBRA_SOLVER_POWELL_HPP

#include <stan/math/rev/meta.hpp>
#include <stan/math/rev/core.hpp>
#include <stan/math/rev/functor/algebra_system.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/fun/value_of.hpp>
#include <stan/math/prim/fun/eval.hpp>
#include <stan/math/prim/functor/apply.hpp>
#include <stan/math/prim/functor/algebra_solver_adapter.hpp>
#include <unsupported/Eigen/NonLinearOptimization>
#include <iostream>
#include <string>
#include <vector>

namespace stan {
namespace math {

/** Function for internal use that actually calls the powell solver. */
template <typename S, typename F, typename T,
          require_eigen_vector_t<T>* = nullptr>
Eigen::VectorXd algebra_solver_powell_call_solver_(
    S& solver, const F& fx, const T& x, std::ostream* msgs,
    double relative_tolerance, double function_tolerance,
    long int max_num_steps) {  // NOLINT(runtime/int)
  // Compute theta_dbl
  Eigen::VectorXd theta_dbl = x;
  solver.parameters.xtol = relative_tolerance;
  solver.parameters.maxfev = max_num_steps;
  solver.solve(theta_dbl);

  // Check if the max number of steps has been exceeded
  if (solver.nfev >= max_num_steps) {
    throw_domain_error("algebra_solver", "maximum number of iterations",
                       max_num_steps, "(", ") was exceeded in the solve.");
  }

  // Check solution is a root
  double system_norm = fx.get_value(theta_dbl).stableNorm();
  if (system_norm > function_tolerance) {
    std::ostringstream message;
    message << "the norm of the algebraic function is " << system_norm
            << " but should be lower than the function "
            << "tolerance:";
    throw_domain_error("algebra_solver", message.str().c_str(),
                       function_tolerance, "",
                       ". Consider decreasing the relative tolerance and "
                       "increasing max_num_steps.");
  }

  return theta_dbl;
}

/** Implementation of ordinary powell solver. */
template <typename F, typename T, typename... T_Args,
          require_eigen_vector_t<T>* = nullptr,
          require_all_st_arithmetic<T_Args...>* = nullptr>
Eigen::VectorXd algebra_solver_powell_impl(
    const F& f, const T& x, std::ostream* msgs, double relative_tolerance,
    double function_tolerance, int64_t max_num_steps, const Eigen::VectorXd& y,
    const T_Args&... args) {  // NOLINT(runtime/int)
  const auto& x_val = to_ref(value_of(x));
  auto args_vals_tuple = std::make_tuple(y, eval(value_of(args))...);

  check_nonzero_size("algebra_solver_powell", "initial guess", x_val);
  check_finite("algebra_solver_powell", "initial guess", x_val);
  check_nonnegative("alegbra_solver_powell", "relative_tolerance",
                    relative_tolerance);
  check_nonnegative("algebra_solver_powell", "function_tolerance",
                    function_tolerance);
  check_positive("algebra_solver_powell", "max_num_steps", max_num_steps);

  // Construct the Powell solver
  auto f_wrt_x = [&](const auto& x) {
    return apply([&](const auto&... args) { return f(x, msgs, args...); },
                 args_vals_tuple);
  };

  hybrj_functor_solver<decltype(f_wrt_x)> fx(f_wrt_x);
  Eigen::HybridNonLinearSolver<decltype(fx)> solver(fx);

  // Check dimension unknowns equals dimension of system output
  check_matching_sizes("algebra_solver", "the algebraic system's output",
                       fx.get_value(x_val), "the vector of unknowns, x,", x);

  // Solve the system
  return algebra_solver_powell_call_solver_(solver, fx, x_val, 0,
                                            relative_tolerance,
                                            function_tolerance, max_num_steps);
}

/** Implementation of autodiff powell solver. */
template <typename F, typename T, typename... T_Args,
          require_eigen_vector_t<T>* = nullptr,
          require_any_st_var<T_Args...>* = nullptr>
Eigen::Matrix<var, Eigen::Dynamic, 1> algebra_solver_powell_impl(
    const F& f, const T& x, std::ostream* msgs, double relative_tolerance,
    double function_tolerance, int64_t max_num_steps,
    const T_Args&... args) {  // NOLINT(runtime/int)
  const auto& x_val = to_ref(value_of(x));
  auto arena_args_tuple = std::make_tuple(to_arena(args)...);
  auto args_vals_tuple = std::make_tuple(eval(value_of(args))...);

  check_nonzero_size("algebra_solver_powell", "initial guess", x_val);
  check_finite("algebra_solver_powell", "initial guess", x_val);
  check_nonnegative("alegbra_solver_powell", "relative_tolerance",
                    relative_tolerance);
  check_nonnegative("algebra_solver_powell", "function_tolerance",
                    function_tolerance);
  check_positive("algebra_solver_powell", "max_num_steps", max_num_steps);

  // Construct the Powell solver
  auto f_wrt_x = [&](const auto& x) {
    return apply([&](const auto&... args) { return f(x, msgs, args...); },
                 args_vals_tuple);
  };

  hybrj_functor_solver<decltype(f_wrt_x)> fx(f_wrt_x);
  Eigen::HybridNonLinearSolver<decltype(fx)> solver(fx);

  // Check dimension unknowns equals dimension of system output
  check_matching_sizes("algebra_solver", "the algebraic system's output",
                       fx.get_value(x_val), "the vector of unknowns, x,", x);

  // Solve the system
  Eigen::VectorXd theta_dbl = algebra_solver_powell_call_solver_(
      solver, fx, x_val, 0, relative_tolerance, function_tolerance,
      max_num_steps);

  Eigen::MatrixXd Jf_x;
  Eigen::VectorXd f_x;

  jacobian(f_wrt_x, theta_dbl, f_x, Jf_x);

  using ret_type = Eigen::Matrix<var, Eigen::Dynamic, -1>;
  using lu_type = Eigen::PartialPivLU<Eigen::Matrix<double, -1, -1, 1, -1, -1>>;
  plain_type_t<lu_type>* Jf_x_lu_ptr = make_chainable_ptr((lu_type) Jf_x.transpose().lu());

  arena_t<ret_type> ret = theta_dbl;

  reverse_pass_callback([f, ret, arena_args_tuple, Jf_x_lu_ptr, msgs]() mutable {
    using Eigen::Dynamic;
    using Eigen::Matrix;
    using Eigen::MatrixXd;
    using Eigen::VectorXd;

    // Contract specificities with inverse Jacobian of f with respect to x.
    VectorXd ret_adj = ret.adj();
    VectorXd eta = -Jf_x_lu_ptr->solve(ret_adj);

    // Contract with Jacobian of f with respect to y using a nested reverse
    // autodiff pass.
    {
      nested_rev_autodiff rev;

      VectorXd ret_val = ret.val();
      auto x_nrad_ = apply(
          [&](const auto&... args) { return eval(f(ret_val, msgs, args...)); },
          arena_args_tuple);
      x_nrad_.adj() = eta;
      grad();
    }
  });

  return ret_type(ret);
}
/**
 * Return the solution to the specified system of algebraic
 * equations given an initial guess, and parameters and data,
 * which get passed into the algebraic system.
 * Use Powell's dogleg solver.
 *
 * The user can also specify the relative tolerance
 * (xtol in Eigen's code), the function tolerance,
 * and the maximum number of steps (maxfev in Eigen's code).
 *
 * Overload the previous definition to handle the case where y
 * is a vector of parameters (var). The overload calls the
 * algebraic solver defined above and builds a vari object on
 * top, using the algebra_solver_vari class.
 *
 * @tparam F type of equation system function
 * @tparam T1 type of elements in the x vector
 * @tparam T2 type of elements in the y vector
 *
 * @param[in] f Functor that evaluates the system of equations.
 * @param[in] x Vector of starting values (initial guess).
 * @param[in] y parameter vector for the equation system.
 * @param[in] dat continuous data vector for the equation system.
 * @param[in] dat_int integer data vector for the equation system.
 * @param[in, out] msgs the print stream for warning messages.
 * @param[in] relative_tolerance determines the convergence criteria
 *            for the solution.
 * @param[in] function_tolerance determines whether roots are acceptable.
 * @param[in] max_num_steps  maximum number of function evaluations.
 * @return theta Vector of solutions to the system of equations.
 * @throw <code>std::invalid_argument</code> if x has size zero.
 * @throw <code>std::invalid_argument</code> if x has non-finite elements.
 * @throw <code>std::invalid_argument</code> if y has non-finite elements.
 * @throw <code>std::invalid_argument</code> if dat has non-finite elements.
 * @throw <code>std::invalid_argument</code> if dat_int has non-finite
 * elements.
 * @throw <code>std::invalid_argument</code> if relative_tolerance is strictly
 * negative.
 * @throw <code>std::invalid_argument</code> if function_tolerance is strictly
 * negative.
 * @throw <code>std::invalid_argument</code> if max_num_steps is not positive.
 * @throw <code>std::domain_error</code> solver exceeds max_num_steps.
 * @throw <code>std::domain_error</code> if the norm of the solution exceeds
 * the function tolerance.
 */
template <typename F, typename T1, typename T2,
          require_all_eigen_vector_t<T1, T2>* = nullptr>
Eigen::Matrix<value_type_t<T2>, Eigen::Dynamic, 1> algebra_solver_powell(
    const F& f, const T1& x, const T2& y, const std::vector<double>& dat,
    const std::vector<int>& dat_int, std::ostream* msgs = nullptr,
    double relative_tolerance = 1e-10, double function_tolerance = 1e-6,
    long int max_num_steps = 1e+3) {  // NOLINT(runtime/int)
  return algebra_solver_powell_impl(algebra_solver_adapter<F>(f), x, msgs,
                                    relative_tolerance, function_tolerance,
                                    max_num_steps, y, dat, dat_int);
}

/**
 * Return the solution to the specified system of algebraic
 * equations given an initial guess, and parameters and data,
 * which get passed into the algebraic system.
 * Use Powell's dogleg solver.
 *
 * The user can also specify the relative tolerance
 * (xtol in Eigen's code), the function tolerance,
 * and the maximum number of steps (maxfev in Eigen's code).
 *
 * Signature to maintain backward compatibility, will be removed
 * in the future.
 *
 * @tparam F type of equation system function
 * @tparam T1 type of elements in the x vector
 * @tparam T2 type of elements in the y vector
 *
 * @param[in] f Functor that evaluates the system of equations.
 * @param[in] x Vector of starting values (initial guess).
 * @param[in] y parameter vector for the equation system.
 * @param[in] dat continuous data vector for the equation system.
 * @param[in] dat_int integer data vector for the equation system.
 * @param[in, out] msgs the print stream for warning messages.
 * @param[in] relative_tolerance determines the convergence criteria
 *            for the solution.
 * @param[in] function_tolerance determines whether roots are acceptable.
 * @param[in] max_num_steps  maximum number of function evaluations.
 * @return theta Vector of solutions to the system of equations.
 * @throw <code>std::invalid_argument</code> if x has size zero.
 * @throw <code>std::invalid_argument</code> if x has non-finite elements.
 * @throw <code>std::invalid_argument</code> if y has non-finite elements.
 * @throw <code>std::invalid_argument</code> if dat has non-finite elements.
 * @throw <code>std::invalid_argument</code> if dat_int has non-finite
 * elements.
 * @throw <code>std::invalid_argument</code> if relative_tolerance is strictly
 * negative.
 * @throw <code>std::invalid_argument</code> if function_tolerance is strictly
 * negative.
 * @throw <code>std::invalid_argument</code> if max_num_steps is not positive.
 * @throw <code>boost::math::evaluation_error</code> (which is a subclass of
 * <code>std::domain_error</code>) if solver exceeds max_num_steps.
 * @throw <code>boost::math::evaluation_error</code> (which is a subclass of
 * <code>std::domain_error</code>) if the norm of the solution exceeds the
 * function tolerance.
 */
template <typename F, typename T1, typename T2,
          require_all_eigen_vector_t<T1, T2>* = nullptr>
Eigen::Matrix<value_type_t<T2>, Eigen::Dynamic, 1> algebra_solver(
    const F& f, const T1& x, const T2& y, const std::vector<double>& dat,
    const std::vector<int>& dat_int, std::ostream* msgs = nullptr,
    double relative_tolerance = 1e-10, double function_tolerance = 1e-6,
    long int max_num_steps = 1e+3) {  // NOLINT(runtime/int)
  return algebra_solver_powell(f, x, y, dat, dat_int, msgs, relative_tolerance,
                               function_tolerance, max_num_steps);
}

}  // namespace math
}  // namespace stan

#endif
