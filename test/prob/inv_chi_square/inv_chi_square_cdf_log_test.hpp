// Arguments: Doubles, Doubles
#include <stan/math/prim/prob/inv_chi_square_cdf_log.hpp>
#include <stan/math/prim/fun/log.hpp>
#include <stan/math/prim/fun/gamma_q.hpp>

using stan::math::var;
using std::numeric_limits;
using std::vector;

class AgradCdfLogInvChiSquare : public AgradCdfLogTest {
 public:
  void valid_values(vector<vector<double> >& parameters,
                    vector<double>& cdf_log) {
    vector<double> param(2);

    param[0] = 3.0;  // y
    param[1] = 0.5;  // Degrees of freedom
    parameters.push_back(param);
    cdf_log.push_back(
        std::log(0.317528038297796704186230));  // expected cdf_log
  }

  void invalid_values(vector<size_t>& index, vector<double>& value) {
    // y
    index.push_back(0U);
    value.push_back(-1.0);

    // nu
    index.push_back(1U);
    value.push_back(-1.0);

    index.push_back(1U);
    value.push_back(0.0);

    index.push_back(1U);
    value.push_back(numeric_limits<double>::infinity());
  }

  bool has_lower_bound() { return true; }

  double lower_bound() { return 0.0; }

  bool has_upper_bound() { return false; }

  template <typename T_y, typename T_dof, typename T2, typename T3, typename T4,
            typename T5>
  stan::return_type_t<T_y, T_dof> cdf_log(const T_y& y, const T_dof& nu,
                                          const T2&, const T3&, const T4&,
                                          const T5&) {
    return stan::math::inv_chi_square_cdf_log(y, nu);
  }

  template <typename T_y, typename T_dof, typename T2, typename T3, typename T4,
            typename T5>
  stan::return_type_t<T_y, T_dof> cdf_log_function(const T_y& y,
                                                   const T_dof& nu, const T2&,
                                                   const T3&, const T4&,
                                                   const T5&) {
    using stan::math::gamma_q;

    return log(gamma_q(0.5 * nu, 0.5 / y));
  }
};
