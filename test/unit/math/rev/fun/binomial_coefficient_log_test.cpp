#include <stan/math/prim.hpp>
#include <stan/math/rev.hpp>
#include <gtest/gtest.h>
#include <limits>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>

TEST(MathFunctions, binomial_coefficient_log_identities) {
  using stan::math::binomial_coefficient_log;
  using stan::math::is_nan;
  using stan::math::log;
  using stan::math::log_sum_exp;
  using stan::math::value_of;
  using stan::math::var;

  std::vector<double> n_to_test
      //      = {-0.1, 0, 1e-100, 1e-8, 1e-1, 1, 1 + 1e-6, 1e3, 1e30, 1e100};
      = {15, 1e3, 1e30, 1e100};

  std::vector<double> k_ratios_to_test
      //      = { -0.1, 1e-10, 1e-5, 1e-3, 1e-1, 0.5, 0.9, 1 - 1e-5, 1 - 1e-10
      //      };
      = {1e-3, 1e-1, 0.5, 0.9, 1 - 1e-5};

  // Recurrence relation
  for (double n_dbl : n_to_test) {
    for (double k_ratio : k_ratios_to_test) {
      double k_dbl = n_dbl * k_ratio;
      if (n_dbl <= 0 || k_dbl <= 0) {
        continue;
      }
      // The redundant -1  +1 is necessary as this copies the loss of precision
      // for very small n_dbl
      if ((n_dbl - 1) + 1 - k_dbl <= 0) {
        continue;
      }

      var n(n_dbl);
      var k(k_dbl);
      var val;

      val = binomial_coefficient_log(n, k)
            / (binomial_coefficient_log(n - 1, k - 1) + log(n) - log(k));

      std::vector<var> vars;
      vars.push_back(n);
      vars.push_back(k);

      std::vector<double> gradients;
      val.grad(vars, gradients);

      for (int i = 0; i < 2; ++i) {
        EXPECT_FALSE(is_nan(gradients[i]));
      }

      std::stringstream msg;
      msg << std::setprecision(22) << " (n - 1) choose (k - 1): n = " << n
          << ", k = " << k << std::endl
          << "val = " << binomial_coefficient_log(n_dbl, k_dbl);

      EXPECT_NEAR(value_of(val), 1, 1e-8) << "val" << msg.str();
      EXPECT_NEAR(gradients[0], 0, 1e-8) << "dn" << msg.str();
      EXPECT_NEAR(gradients[1], 0, 1e-8) << "dx" << msg.str();
    }
  }
}

namespace binomial_coefficient_log_test_internal {
struct TestValue {
  double n;
  double k;
  double val;
  double dn;
  double dk;
};

const double NaN = stan::math::NOT_A_NUMBER;
// Test values generated in Mathematica, reproducible notebook at
// https://www.wolframcloud.com/obj/martin.modrak/Published/binomial_coefficient_log.nb
// Mathematica Code reproduced below for convenience:

// toCString[x_] := ToString[CForm[N[x, 24]]];
// singleTest[x_,y_]:= Module[{val, cdn,cdk},{
// val = toCString[bclog[x,y]];
// cdn = If[x > 10^6 || y > 10^6,"NaN", toCString[dbclogdn[x,y]]];
// cdk = If[x > 10^6 || y > 10^6,"NaN", toCString[dbclogdk[x,y]]];
// StringJoin["  {",toCString[x],",",toCString[y],",",
//            val,",",cdn,",",cdk,"},\n"]
// }];
// ns= {-0.1,3*10^-5,2*10^-3,1,8, 1325,845*10^3};
// ratios =  {-1,10^- 10,10^-5,10^-2,1/5,1/2,1-3*10^-2,1-6*10^-8, 1 -3*10^-9,2};
// out = "std::vector<TestValue> testValues = {\n";
// For[i = 1, i <= Length[ns], i++, {
//   For[j = 1, j <= Length[ratios], j++, {
//     cn = ns[[i]];
//     ck = If[ratios[[j]] < 0,-9/10,
//     If[ratios[[j]] > 1,cn + 9/10,cn * ratios[[j]] ]];
//     out = StringJoin[out, singleTest[cn,ck]];
//   }]
// }]
// extremeNs = {3*10^15+1/2,10^20 + 1/2};
// lowKs = {3, 100, 12895};
// For[i = 1, i <= Length[extremeNs], i++, {
//   For[j = 1, j <= Length[lowKs], j++, {
//     cn = extremeNs[[i]];
//     ck = lowKs[[j]];
//     out = StringJoin[out,singleTest[cn,ck]];
//   }]
// }]
// out = StringJoin[out,"};\n"];
// out
std::vector<TestValue> testValues = {
    {-0.1, -0.9, -2.11525253908509081592028, -1.0399183832409129390763,
     10.7087463737049383316859},
    {-0.1, -1.0000000000000001e-11, 1.77711285027681189779234e-12,
     -1.92253995946119474331752e-11, -0.177711285009843801688898},
    {-0.1, -2.0000000000000003e-6, 3.55415435144048059050025e-7,
     -3.8450735153733054435704e-6, -0.177704150099061235922007},
    {-0.1, -0.003, 0.000517083756840281579810978, -0.00575325547677734075296938,
     -0.167012379549101675417438},
    {-0.1, -0.020000000000000004, 0.00284168667292114343243947,
     -0.0378231526143224327191161, -0.106499800517313678664015},
    {-0.1, -0.05, 0.0044386492587971776182016, -0.0923173337417260614732854, 0},
    {-0.1, -0.097, 0.000517083756840282014391421, -0.172765635025879011871306,
     0.167012379549101666140604},
    {-0.1, -0.09999999400000001, 1.06626764549734262955223e-9,
     -0.177711275175914102783629, 0.177711263640674409624395},
    {-0.1, -0.0999999998, 3.55422574122942521222662e-11,
     -0.17771128471653172414194, 0.177711284332023727176803},
    {-0.1, 0.8, -2.11525253908509135092963, 9.66882799046403046022088,
     -10.7087463737049434361164},
    {0.00003, -0.9, -2.21375637737528044964183, -0.933371118080918307851001,
     10.7799597405306456982508},
    {0.00003, 3.0000000000000002e-15, 1.4804082053556344384283e-19,
     4.93458583906860402434769e-15, 0.0000493469401735864500932795},
    {0.00003, 6.000000000000001e-10, 2.96075719467911309956804e-14,
     9.86917168246424148279385e-10, 0.000049344966305847915513204},
    {0.00003, 9.e-7, 4.30798787797857747053402e-11,
     1.48037672530856143443777e-6, 0.0000463861237716491755189368},
    {0.00003, 6.e-6, 2.36865312869367146776112e-10,
     9.86921494891293022056408e-6, 0.0000296081641072682823142211},
    {0.00003, 0.000015, 3.70102051348524063287983e-10,
     0.0000246731996398828634493583, 0},
    {0.00003, 0.0000291, 4.30798787797858483751685e-11,
     0.0000478665004969577343409155, -0.0000463861237716491702941263},
    {0.00003, 0.000029999998200000002, 8.88244869467749046930373e-17,
     0.0000493469372225745196092216, -0.0000493469342618230179633344},
    {0.00003, 0.00002999999994, 2.96081652032227041059628e-18,
     0.0000493469400847597902807008, -0.0000493469399860680696581904},
    {0.00003, 0.90003, -2.2137563773752804140164, 9.84658862244972705518472,
     -10.7799597405306453607622},
    {0.002, -0.9, -2.21559326412971099690821, -0.931489785799666354301045,
     10.7813141298573216196055},
    {0.002, 2.e-13, 6.57013709556564711297722e-16,
     3.28027758802827577274611e-13, 0.00328506854745431617062256},
    {0.002, 4.e-8, 1.31400113866164680767665e-10, 6.56055536734964490768367e-8,
     0.00328493714519690660826286},
    {0.002, 0.00006, 1.91190982195918985147542e-7,
     0.0000984126319888139547815645, 0.0030879641992827061931754},
    {0.002, 0.0004, 1.05122171458287357370166e-6, 0.000656246880415250699426597,
     0.0019710403008191429519067},
    {0.002, 0.001, 1.64253373496215320086901e-6, 0.00164133545687894157470528,
     0},
    {0.002, 0.0019399999999999999, 1.91190982195919466419307e-7,
     0.00318637683127151989160979, -0.00308796419928270568118357},
    {0.002, 0.00199999988, 3.94208201970328159413461e-13,
     0.00328506835071924281368086, -0.00328506815390258758994027},
    {0.002, 0.001999999996, 1.31402741136587958866517e-14,
     0.00328506854153159450171264, -0.0032850685349710393518526},
    {0.002, 0.902, -2.21559326412971125500408, 9.84982434405765769353491,
     -10.7813141298573240642835},
    {1., -0.9, -2.85558226198351740582195, -0.459715615539276790357555,
     11.3062548910488207249193},
    {1., 1.e-10, 9.9999999988550662975071e-11, 6.44934066868432150285563e-11,
     0.99999999977101318664035},
    {1., 0.00002, 0.0000199995420290398824268595,
     0.0000128987621603843854005126, 0.999954203037316753927052},
    {1., 0.03, 0.0289783282362563119258558, 0.0195321262846958328746912,
     0.932173296099201809954111},
    {1., 0.2, 0.156457962917688023080733, 0.137792901804605606966842,
     0.57403132988604981390314},
    {1., 0.5, 0.241564475270490444691037, 0.386294361119890618834464, 0},
    {1., 0.97, 0.0289783282362563377988622, 0.951705422383897599096425,
     -0.932173296099201747978444},
    {1., 0.99999994, 5.99999958466560848176267e-8, 0.999999901303960368460267,
     -0.999999862607915650529701},
    {1., 0.999999998, 2.00000004987870302153356e-9, 0.999999996710131781531233,
     -0.999999995420263611904449},
    {1., 1.9, -2.85558226198351640162479, 10.846539275509534925475,
     -11.3062548910488116793316},
    {8., -0.9, -4.22528965320883461943031, -0.100538838650771402252215,
     12.6649352570174581939568},
    {8., 8.e-10, 2.17428571372173161903874e-9, 9.40096117596390056755358e-11,
     2.71785714144718599267395},
    {8., 0.00016, 0.000434834585178913876603697, 0.0000188020989077387807855132,
     2.71757518207576360648536},
    {8., 0.24, 0.606274586245453630229602, 0.0286077438730426700300604,
     2.35152741320850413169898},
    {8., 1.6, 2.90678606291134293918723, 0.208248082071609215411629,
     1.18134594311052448766911},
    {8., 4., 4.24849524204935898912334, 0.634523809523809523809524, 0},
    {8., 7.76, 0.606274586245454152373578, 2.38013515708154653288918,
     -2.35152741320850383600988},
    {8., 7.99999952, 1.30457122485101544957266e-6, 2.71785635328906813937859,
     -2.71785629688329952694258},
    {8., 7.999999984, 4.34857152442032476701103e-8, 2.71785711653819737865347,
     -2.71785711465800509058726},
    {8., 8.9, -4.22528965320883911891918, 12.5643964183667228280515,
     -12.664935257017494268063},
    {1325.45, -0.9, -8.72391406172695433576549, -0.000678528341300029218848988,
     17.6143179550958346212948},
    {1325.45, 1.32545e-7, 1.02949027509089702892375e-6,
     9.99622864543865370572321e-11, 7.76709993311726359860762},
    {1325.45, 0.026509000000000005, 0.205327156306863379978836,
     0.0000199926571417065267265905, 7.72429965627466779820619},
    {1325.45, 39.7635, 175.846725556752161664879, 0.0304475435453562149043387,
     3.46396589228722918392492},
    {1325.45, 265.09000000000003, 659.660660749231586923507,
     0.223049270402325103613771, 1.38488085895377544787304},
    {1325.45, 662.725, 914.911196853663854616454, 0.692770092488070560492316,
     0},
    {1325.45, 1285.6865, 175.846725556752235503753, 3.49441343583258486943696,
     -3.46396589228722863795938},
    {1325.45, 1325.449920473, 0.000617688970012744110414011,
     7.76696934217533185815885, -7.76696928219795817077374},
    {1325.45, 1325.4499973491, 0.0000205898008981706567184859,
     7.76709579069753161291118, -7.76709578869828579554802},
    {1325.45, 1326.3500000000001, -8.72391406172855634865104,
     17.613639426763759896892, -17.6143179551050599946563},
    {845000.3, -0.9, -14.5350966987995578090701, -1.06508718182367185039981e-6,
     24.0708488585827418941125},
    {845000.3, 0.00008450003000000001, 0.00120194862411218431712068,
     9.99999408334467162987851e-11, 14.2241695294903594263306},
    {845000.3, 16.900006, 197.416639012626785645761,
     0.0000200001881681193593499904, 10.790464758593674854536},
    {845000.3, 25350.009000000002, 113851.198555151510249396,
     0.0304591891842282610860051, 3.47607957612220465494725},
    {845000.3, 169000.06000000003, 422833.371816046100941544,
     0.223143403385333866823948, 1.38629214218850240580417},
    {845000.3, 422500.15, 585702.52617952879969091, 0.693146588844529182207756,
     0},
    {845000.3, 819650.291, 113851.198555151775813377, 3.50653876530642990238351,
     -3.47607957612220154809006},
    {845000.3, 845000.2492999821, 0.71910904869572166682554,
     14.1438656829570673122744, -14.1438656229571010748933},
    {845000.3, 845000.2983099994, 0.0240367434486935944287041,
     14.2215320063338616170949, -14.2215320043338627454076},
    {845000.3, 845001.2000000001, -14.5350966993600009324015,
     24.0708477958572381039047, -24.0708488609444199551305},
    {3.0000000000000005e15, 3.1, 108.557127724329303822723, NaN, NaN},
    {3.0000000000000005e15, 100.2, 3206.2047392970248044977, NaN, NaN},
    {3.0000000000000005e15, 12895.6, 350403.227999624864153782, NaN, NaN},
    {1.e20, 3.1, 140.841498570865873374959, NaN, NaN},
    {1.e20, 100.2, 4249.7189195624987706026, NaN, NaN},
    {1.e20, 12895.6, 484702.044995974181173534, NaN, NaN},
};

}  // namespace binomial_coefficient_log_test_internal

TEST(MathFunctions, binomial_coefficient_log_precomputed) {
  using binomial_coefficient_log_test_internal::TestValue;
  using binomial_coefficient_log_test_internal::testValues;
  using stan::math::is_nan;
  using stan::math::value_of;
  using stan::math::var;

  for (TestValue t : testValues) {
    std::stringstream msg;
    msg << std::setprecision(22) << "n = " << t.n << ", k = " << t.k;

    var n(t.n);
    var k(t.k);
    var val = stan::math::binomial_coefficient_log(n, k);

    std::vector<var> vars;
    vars.push_back(n);
    vars.push_back(k);

    std::vector<double> gradients;
    val.grad(vars, gradients);

    for (int i = 0; i < 2; ++i) {
      EXPECT_FALSE(is_nan(gradients[i]));
    }

    double tol_val = std::max(1e-14 * fabs(t.val), 1e-14);
    EXPECT_NEAR(value_of(val), t.val, tol_val) << msg.str();

    std::function<double(double)> tol_grad;
    if (n < 1 || k < 1) {
      tol_grad = [](double x) { return std::max(fabs(x) * 1e-8, 1e-7); };
    } else {
      tol_grad = [](double x) { return std::max(fabs(x) * 1e-10, 1e-8); };
    }
    if (!is_nan(t.dn)) {
      EXPECT_NEAR(gradients[0], t.dn, tol_grad(t.dn)) << "dn: " << msg.str();
    }
    if (!is_nan(t.dk)) {
      EXPECT_NEAR(gradients[1], t.dk, tol_grad(t.dk)) << "dk: " << msg.str();
    }
  }
}
