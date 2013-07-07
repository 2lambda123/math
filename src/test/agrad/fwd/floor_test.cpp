#include <gtest/gtest.h>
#include <stan/agrad/fvar.hpp>
#include <stan/agrad/var.hpp>
#include <test/agrad/util.hpp>

TEST(floor,AgradFvar) {
  using stan::agrad::fvar;
  using std::floor;

  fvar<double> x(0.5,1.0);
  fvar<double> y(2.0,2.0);

  fvar<double> a = floor(x);
  EXPECT_FLOAT_EQ(floor(0.5), a.val_);
  EXPECT_FLOAT_EQ(0, a.d_);

  fvar<double> b = floor(y);
  EXPECT_FLOAT_EQ(floor(2.0), b.val_);
  EXPECT_FLOAT_EQ(0.0, b.d_);

  fvar<double> c = floor(2 * x);
  EXPECT_FLOAT_EQ(floor(2 * 0.5), c.val_);
   EXPECT_FLOAT_EQ(0.0, c.d_);
}

TEST(floor,AgradFvarVar_1stderiv) {
  using stan::agrad::fvar;
  using stan::agrad::var;
  using std::floor;

  fvar<var> x(1.5,1.3);
  fvar<var> a = floor(x);

  EXPECT_FLOAT_EQ(floor(1.5), a.val_.val());
  EXPECT_FLOAT_EQ(0, a.d_.val());

  AVEC y = createAVEC(x.val_);
  VEC g;
  a.val_.grad(y,g);
  EXPECT_FLOAT_EQ(0, g[0]);
}

TEST(floor,AgradFvarVar_2ndderiv) {
  using stan::agrad::fvar;
  using stan::agrad::var;
  using std::floor;

  fvar<var> x(1.5,1.3);
  fvar<var> a = floor(x);

  AVEC y = createAVEC(x.val_);
  VEC g;
  a.d_.grad(y,g);
  EXPECT_FLOAT_EQ(0, g[0]);
}

TEST(floor,AgradFvarFvarDouble) {
  using stan::agrad::fvar;
  using std::floor;

  fvar<fvar<double> > x;
  x.val_.val_ = 1.5;
  x.val_.d_ = 2.0;

  fvar<fvar<double> > a = floor(x);

  EXPECT_FLOAT_EQ(floor(1.5), a.val_.val_);
  EXPECT_FLOAT_EQ(0, a.val_.d_);
  EXPECT_FLOAT_EQ(0, a.d_.val_);
  EXPECT_FLOAT_EQ(0, a.d_.d_);

  fvar<fvar<double> > y;
  y.val_.val_ = 1.5;
  y.d_.val_ = 2.0;

  a = floor(y);
  EXPECT_FLOAT_EQ(floor(1.5), a.val_.val_);
  EXPECT_FLOAT_EQ(0, a.val_.d_);
  EXPECT_FLOAT_EQ(0, a.d_.val_);
  EXPECT_FLOAT_EQ(0, a.d_.d_);
}

TEST(floor,AgradFvarFvarVar_1stderiv) {
  using stan::agrad::fvar;
  using stan::agrad::var;
  using std::floor;

  fvar<fvar<var> > x;
  x.val_.val_ = 1.5;
  x.val_.d_ = 2.0;

  fvar<fvar<var> > a = floor(x);

  EXPECT_FLOAT_EQ(floor(1.5), a.val_.val_.val());
  EXPECT_FLOAT_EQ(0, a.val_.d_.val());
  EXPECT_FLOAT_EQ(0, a.d_.val_.val());
  EXPECT_FLOAT_EQ(0, a.d_.d_.val());

  AVEC p = createAVEC(x.val_.val_);
  VEC g;
  a.val_.val_.grad(p,g);
  EXPECT_FLOAT_EQ(0.0, g[0]);

  fvar<fvar<var> > y;
  y.val_.val_ = 1.5;
  y.d_.val_ = 2.0;

  fvar<fvar<var> > b = floor(y);
  EXPECT_FLOAT_EQ(floor(1.5), a.val_.val_.val());
  EXPECT_FLOAT_EQ(0, a.val_.d_.val());
  EXPECT_FLOAT_EQ(0, a.d_.val_.val());
  EXPECT_FLOAT_EQ(0, a.d_.d_.val());

  AVEC q = createAVEC(y.val_.val_);
  VEC r;
  b.val_.val_.grad(q,r);
  EXPECT_FLOAT_EQ(0.0, r[0]);
}


TEST(floor,AgradFvarFvarVar_2ndderiv) {
  using stan::agrad::fvar;
  using stan::agrad::var;
  using std::floor;

  fvar<fvar<var> > x;
  x.val_.val_ = 1.5;
  x.val_.d_ = 2.0;

  fvar<fvar<var> > a = floor(x);

  AVEC p = createAVEC(x.val_.val_);
  VEC g;
  a.val_.d_.grad(p,g);
  EXPECT_FLOAT_EQ(0.0, g[0]);

  fvar<fvar<var> > y;
  y.val_.val_ = 1.5;
  y.d_.val_ = 2.0;

  fvar<fvar<var> > b = floor(y);

  AVEC q = createAVEC(y.val_.val_);
  VEC r;
  b.d_.val_.grad(q,r);
  EXPECT_FLOAT_EQ(0.0, r[0]);
}
