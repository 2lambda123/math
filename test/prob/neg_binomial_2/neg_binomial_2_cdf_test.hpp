// Arguments: Ints, Doubles, Doubles
#include <stan/math/prim/prob/neg_binomial_2_cdf.hpp>
#include <stan/math/prim/fun/multiply_log.hpp>
#include <stan/math/prim/fun/log.hpp>
#include <stan/math/prim/fun/exp.hpp>
#include <boost/math/special_functions/binomial.hpp>

using stan::math::var;
using std::numeric_limits;
using std::vector;

class AgradCdfNegBinomial2 : public AgradCdfTest {
 public:
  void valid_values(vector<vector<double> >& parameters, vector<double>& cdf) {
    vector<double> param(3);

    param[0] = 3;   // n
    param[1] = 10;  // mu
    param[2] = 20;  // phi
    parameters.push_back(param);
    cdf.push_back(0.0264752601628231235);  // expected cdf

    param[0] = 7;   // n
    param[1] = 15;  // mu
    param[2] = 10;  // phi
    parameters.push_back(param);
    cdf.push_back(0.091899254171238523026);  // expected cdf

    param[0] = 0;   // n
    param[1] = 15;  // mu
    param[2] = 10;  // phi
    parameters.push_back(param);
    cdf.push_back(0.0001048576000000001529);  // expected cdf

    param[0] = 1;   // n
    param[1] = 15;  // mu
    param[2] = 10;  // phi
    parameters.push_back(param);
    cdf.push_back(0.00073400320000000126002);  // expected cdf

    param[0] = 0;   // n
    param[1] = 10;  // mu
    param[2] = 1;   // phi
    parameters.push_back(param);
    cdf.push_back(0.090909090909090897736);  // expected cdf

    param[0] = -1;  // n
    param[1] = 10;  // mu
    param[2] = 1;   // phi
    parameters.push_back(param);
    cdf.push_back(0);  // expected cdf

    param[0] = -89;  // n
    param[1] = 10;   // mu
    param[2] = 1;    // phi
    parameters.push_back(param);
    cdf.push_back(0);  // expected cdf
  }

  void invalid_values(vector<size_t>& index, vector<double>& value) {
    // mu
    index.push_back(1U);
    value.push_back(-1);

    // phi
    index.push_back(2U);
    value.push_back(-1);
  }

  bool has_lower_bound() { return false; }

  bool has_upper_bound() { return false; }

  template <typename T_n, typename T_location, typename T_precision,
            typename T3, typename T4, typename T5>
  stan::return_type_t<T_location, T_precision> cdf(const T_n& n,
                                                   const T_location& alpha,
                                                   const T_precision& beta,
                                                   const T3&, const T4&,
                                                   const T5&) {
    return stan::math::neg_binomial_2_cdf(n, alpha, beta);
  }

  template <typename T_n, typename T_location, typename T_precision,
            typename T3, typename T4, typename T5>
  stan::return_type_t<T_location, T_precision> cdf_function(
      const T_n& nn, const T_location& mu, const T_precision& phi, const T3&,
      const T4&, const T5&) {
    using stan::math::binomial_coefficient_log;
    using stan::math::multiply_log;
    using std::exp;
    using std::log;

    stan::return_type_t<T_location, T_precision> cdf(0);

    for (int n = 0; n <= nn; n++) {
      stan::return_type_t<T_location, T_precision> lp(0);
      if (n != 0)
        lp += binomial_coefficient_log<
            typename stan::scalar_type<T_precision>::type>(n + phi - 1.0, n);
      lp += multiply_log(n, mu) + multiply_log(phi, phi)
            - (n + phi) * log(mu + phi);
      cdf += exp(lp);
    }

    return cdf;
  }
};
