// Arguments: Doubles, Doubles
#include <stan/math/prim/prob/exponential_ccdf_log.hpp>

using stan::math::var;
using std::numeric_limits;
using std::vector;

class AgradCcdfLogExponential : public AgradCcdfLogTest {
 public:
  void valid_values(vector<vector<double> >& parameters,
                    vector<double>& ccdf_log) {
    vector<double> param(2);

    param[0] = 2.0;  // y
    param[1] = 1.5;  // beta
    parameters.push_back(param);
    ccdf_log.push_back(-2.0 * 1.5);  // expected ccdf_log

    param[0] = 15.0;  // y
    param[1] = 3.9;   // beta
    parameters.push_back(param);
    ccdf_log.push_back(-15.0 * 3.9);  // expected ccdf_log
  }

  void invalid_values(vector<size_t>& index, vector<double>& value) {
    // y
    index.push_back(0U);
    value.push_back(-1.0);

    index.push_back(0U);
    value.push_back(-numeric_limits<double>::infinity());

    // beta
    index.push_back(1U);
    value.push_back(0.0);

    index.push_back(1U);
    value.push_back(-1.0);

    index.push_back(1U);
    value.push_back(numeric_limits<double>::infinity());

    index.push_back(1U);
    value.push_back(-numeric_limits<double>::infinity());
  }

  bool has_lower_bound() { return true; }

  double lower_bound() { return 0.0; }

  bool has_upper_bound() { return false; }

  template <typename T_y, typename T_inv_scale, typename T2, typename T3,
            typename T4, typename T5>
  stan::return_type_t<T_y, T_inv_scale> ccdf_log(const T_y& y,
                                                 const T_inv_scale& beta,
                                                 const T2&, const T3&,
                                                 const T4&, const T5&) {
    return stan::math::exponential_ccdf_log(y, beta);
  }

  template <typename T_y, typename T_inv_scale, typename T2, typename T3,
            typename T4, typename T5>
  stan::return_type_t<T_y, T_inv_scale> ccdf_log_function(
      const T_y& y, const T_inv_scale& beta, const T2&, const T3&, const T4&,
      const T5&) {
    using std::exp;
    using std::log;

    return -beta * y;
  }
};
