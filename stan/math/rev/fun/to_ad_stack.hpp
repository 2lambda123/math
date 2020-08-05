#ifndef STAN_MATH_REF_FUN_TO_AD_STACK_HPP
#define STAN_MATH_REF_FUN_TO_AD_STACK_HPP

#include <stan/math/rev/meta.hpp>
#include <stan/math/prim/fun/Eigen.hpp>
#include <vector>

namespace stan {
namespace math {

/**
 * Converts given argument into a type that has any dynamic allocation on AD
 * stack.
 *
 * For scalars this is a no-op.
 * @tparam type of scalar
 * @param a argument
 * @return argument
 */
template <typename T, require_not_container_t<T>* = nullptr>
T to_AD_stack(T&& a) {
  return std::forward<T>(a);
}

/**
 * Converts given argument into a type that has any dynamic allocation on AD
 * stack.
 *
 * Converts eigen types to `AD_stack_matrix`.
 * @tparam type of argument
 * @param a argument
 * @return argument copied/evaluated on AD stack
 */
template <typename T, require_eigen_t<T>* = nullptr>
AD_stack_t<T> to_AD_stack(const T& a) {
  return AD_stack_t<T>(a);
}

/**
 * Converts given argument into a type that has any dynamic allocation on AD
 * stack.
 *
 * Std vectors are copied into another std vector with custom allocator that
 * uses AD stack.
 *
 * This overload works on vectors with simple scalars that don't need to be
 * converthed themselves.
 *
 * @tparam type of argument
 * @param a argument
 * @return argument copied on AD stack
 */
template <typename T, require_same_t<T, AD_stack_t<T>>* = nullptr>
AD_stack_t<std::vector<T>> to_AD_stack(const std::vector<T>& a) {
  return {a.begin(), a.end()};
}

/**
 * Converts given argument into a type that has any dynamic allocation on AD
 * stack.
 *
 * Std vectors are copied into another std vector with custom allocator that
 * uses AD stack.
 *
 * This overload works on vectors with scalars that also need conversion.
 *
 * @tparam type of argument
 * @param a argument
 * @return argument copied on AD stack
 */
template <typename T, require_not_same_t<T, AD_stack_t<T>>* = nullptr>
AD_stack_t<std::vector<T>> to_AD_stack(const std::vector<T>& a) {
  AD_stack_t<std::vector<T>> res;
  res.reserve(a.size());
  for (const T& i : a) {
    res.push_back(to_AD_stack(i));
  }
  return res;
}

}  // namespace math
}  // namespace stan

#endif