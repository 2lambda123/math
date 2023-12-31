#ifdef STAN_OPENCL
#include <stan/math.hpp>
#include <stan/math/opencl/kernels/device_functions/multiply_log.hpp>
#include <stan/math/opencl/kernel_cl.hpp>
#include <gtest/gtest.h>
#include <test/unit/math/expect_near_rel.hpp>
#include <string>

static const std::string test_lmultiply_kernel_code
    = STRINGIFY(__kernel void test(__global double *C, __global double *A,
                                   __global double *B) {
        const int i = get_global_id(0);
        C[i] = lmultiply(A[i], B[i]);
      });

const stan::math::opencl_kernels::kernel_cl<
    stan::math::opencl_kernels::out_buffer,
    stan::math::opencl_kernels::in_buffer,
    stan::math::opencl_kernels::in_buffer>
    lmultiply("test", {stan::math::opencl_kernels::lmultiply_device_function,
                       test_lmultiply_kernel_code});

TEST(MathMatrixCL, lmultiply) {
  Eigen::VectorXd a = Eigen::VectorXd::Random(1000).array() * 5;
  Eigen::VectorXd b = Eigen::VectorXd::Random(1000).array().abs();

  stan::math::matrix_cl<double> a_cl(a);
  stan::math::matrix_cl<double> b_cl(b);
  stan::math::matrix_cl<double> res_cl(1000, 1);
  lmultiply(cl::NDRange(1000), res_cl, a_cl, b_cl);
  Eigen::VectorXd res = stan::math::from_matrix_cl<Eigen::VectorXd>(res_cl);

  EXPECT_NEAR_REL(res, stan::math::lmultiply(a, b));
}

TEST(MathMatrixCL, lmultiply_edge_cases) {
  Eigen::VectorXd a(7);
  a << NAN, INFINITY, 1.0E50, 0, 1, 1, 1;

  Eigen::VectorXd b(7);
  b << 1, 1, 1, 0, NAN, INFINITY, 1.0E50;

  stan::math::matrix_cl<double> a_cl(a);
  stan::math::matrix_cl<double> b_cl(b);
  stan::math::matrix_cl<double> res_cl(7, 1);
  lmultiply(cl::NDRange(7), res_cl, a_cl, b_cl);
  Eigen::VectorXd res = stan::math::from_matrix_cl<Eigen::VectorXd>(res_cl);

  EXPECT_NEAR_REL(res, stan::math::lmultiply(a, b));
}

#endif
