// Arguments: Doubles, Doubles, Doubles
#include <stan/math/prim/prob/lognormal_cdf_log.hpp>
#include <stan/math/prim/fun/log.hpp>
#include <stan/math/prim/fun/erfc.hpp>

using stan::math::var;
using std::numeric_limits;
using std::vector;

class AgradCdfLogLognormal : public AgradCdfLogTest {
 public:
  void valid_values(vector<vector<double> >& parameters,
                    vector<double>& cdf_log) {
    vector<double> param(3);

    param[0] = 1.2;  // y
    param[1] = 0.3;  // mu
    param[2] = 1.5;  // sigma
    parameters.push_back(param);
    cdf_log.push_back(std::log(0.4687341379535833185699));  // expected cdf_log

    param[0] = 12.0;  // y
    param[1] = 3.0;   // mu
    param[2] = 0.9;   // sigma
    parameters.push_back(param);
    cdf_log.push_back(std::log(0.2835505880143094903367));  // expected cdf_log
  }

  void invalid_values(vector<size_t>& index, vector<double>& value) {
    // y
    index.push_back(0U);
    value.push_back(-1.0);

    index.push_back(0U);
    value.push_back(-numeric_limits<double>::infinity());

    // mu
    index.push_back(1U);
    value.push_back(numeric_limits<double>::infinity());

    index.push_back(1U);
    value.push_back(-numeric_limits<double>::infinity());

    // sigma
    index.push_back(2U);
    value.push_back(0.0);

    index.push_back(2U);
    value.push_back(-1.0);

    index.push_back(2U);
    value.push_back(numeric_limits<double>::infinity());

    index.push_back(2U);
    value.push_back(-numeric_limits<double>::infinity());
  }

  bool has_lower_bound() { return true; }

  double lower_bound() { return 0.0; }

  bool has_upper_bound() { return false; }

  template <typename T_y, typename T_loc, typename T_scale, typename T3,
            typename T4, typename T5>
  stan::return_type_t<T_y, T_loc, T_scale> cdf_log(const T_y& y,
                                                   const T_loc& mu,
                                                   const T_scale& sigma,
                                                   const T3&, const T4&,
                                                   const T5&) {
    return stan::math::lognormal_cdf_log(y, mu, sigma);
  }

  template <typename T_y, typename T_loc, typename T_scale, typename T3,
            typename T4, typename T5>
  stan::return_type_t<T_y, T_loc, T_scale> cdf_log_function(
      const T_y& y, const T_loc& mu, const T_scale& sigma, const T3&, const T4&,
      const T5&) {
    return log(0.5 * erfc(-(log(y) - mu) / (std::sqrt(2) * sigma)));
  }
};
