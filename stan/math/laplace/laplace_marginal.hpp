#ifndef STAN_MATH_LAPLACE_LAPLACE_MARGINAL_HPP
#define STAN_MATH_LAPLACE_LAPLACE_MARGINAL_HPP

#include <stan/math/prim/fun/Eigen.hpp>
#include <stan/math/prim/fun/quad_form_diag.hpp>
#include <stan/math/prim/fun/diag_pre_multiply.hpp>
#include <stan/math/prim/fun/diag_post_multiply.hpp>
#include <stan/math/prim/fun/cholesky_decompose.hpp>
#include <stan/math/prim/fun/sqrt.hpp>
#include <stan/math/rev/fun/cholesky_decompose.hpp>
#include <stan/math/laplace/laplace_likelihood.hpp>
#include <stan/math/laplace/laplace_pseudo_target.hpp>

#include <iostream>
#include <istream>  // CHECK -- do we need this?
#include <fstream>  // CHECK -- do we need this?

// Reference for calculations of marginal and its gradients:
// Charles C Margossian, Aki Vehtari, Daniel Simpson and Raj Agrawal
// "Hamiltonian Monte Carlo using an adjoint-differentiated
// Laplace approximation: Bayesian inference for latent Gaussian
// models and beyond."  NeurIPS 2020
// https://arxiv.org/abs/2004.12550


namespace stan {
namespace math {
  /**
   * For a latent Gaussian model with hyperparameters phi and eta,
   * latent variables theta, and observations y, this function computes
   * an approximation of the log marginal density, p(y | phi).
   * This is done by marginalizing out theta, using a Laplace
   * approxmation. The latter is obtained by finding the mode,
   * via Newton's method, and computing the Hessian of the likelihood.
   *
   * The convergence criterion for the Newton is a small change in
   * log marginal density. The user controls the tolerance (i.e.
   * threshold under which change is deemed small enough) and
   * maximum number of steps.
   * TO DO: add more robust convergence criterion.
   *
   * This algorithm is adapted from Rasmussen and Williams,
   * "Gaussian Processes for Machine Learning", second edition,
   * MIT Press 2006, algorithm 3.1.
   *
   * Variables needed for the gradient or generating quantities
   * are stored by reference.
   *
   * @tparam D structure type for the likelihood object.
   * @tparam K structure type for the covariance object.
   * @tparam Tx type of x, which can in Stan be passed as a matrix or
   *            an array of vectors.
   * @param[in] D structure to compute and differentiate the log likelihood.
   * @param[in] K structure to compute the covariance function.
   * @param[in] phi hyperparameter (input for the covariance function).
   * @param[in] eta hyperparameter (input for likelihood).
   * @param[in] x fixed spatial data (input for the covariance function).
   * @param[in] delta additional fixed real data (input for covariance
   *            function).
   * @param[in] delta_int additional fixed integer data (input for covariance
   *            function).
   * @param[in, out] covariance the evaluated covariance function for the
   *                 latent gaussian variable.
   * @param[in, out] theta a vector to store the mode.
   * @param[in, out] W_root a vector to store the square root of the
   *                 diagonal negative Hessian.
   * @param[in, out] L cholesky decomposition of stabilized inverse covariance.
   * @param[in, out] a element in the Newton step
   * @param[in, out] l_grad the log density of the likelihood.
   * @param[in] theta_0 the initial guess for the mode.
   * @param[in] tolerance the convergence criterion for the Newton solver.
   * @param[in] max_num_steps maximum number of steps for the Newton solver.
   * @return the log marginal density, p(y | phi).
   */
  template <typename D, typename K, typename Tx>
  double
  laplace_marginal_density (const D& diff_likelihood,
                            const K& covariance_function,
                            const Eigen::VectorXd& phi,
                            const Eigen::VectorXd& eta,
                            const Tx& x,
                            const std::vector<double>& delta,
                            const std::vector<int>& delta_int,
                            Eigen::MatrixXd& covariance,
                            Eigen::VectorXd& theta,
                            Eigen::VectorXd& W_root,
                            Eigen::MatrixXd& L,
                            Eigen::VectorXd& a,
                            Eigen::VectorXd& l_grad,
                            const Eigen::VectorXd& theta_0,
                            std::ostream* msgs = nullptr,
                            double tolerance = 1e-6,
                            long int max_num_steps = 100) {
    using Eigen::MatrixXd;
    using Eigen::VectorXd;

    int group_size = theta_0.size();
    covariance = covariance_function(phi, x, delta, delta_int, msgs);
    theta = theta_0;
    double objective_old = - 1e+10;  // CHECK -- what value to use?
    double objective_new;

    for (int i = 0; i <= max_num_steps; i++) {
      if (i == max_num_steps) {
        std::ostringstream message;
        message << "laplace_marginal_density: max number of iterations:"
                << max_num_steps << " exceeded.";
        throw boost::math::evaluation_error(message.str());
      }

      // Compute variable a.
      VectorXd hessian;
      diff_likelihood.diff(theta, eta, l_grad, hessian);
      VectorXd W = - hessian;
      W_root = sqrt(W);
      {
        MatrixXd B = MatrixXd::Identity(group_size, group_size)
          + quad_form_diag(covariance, W_root);
        L = cholesky_decompose(B);
      }
      VectorXd b = W.cwiseProduct(theta) + l_grad;
      a = b - W_root.asDiagonal() * mdivide_left_tri<Eigen::Upper>(transpose(L),
           mdivide_left_tri<Eigen::Lower>(L,
           diag_pre_multiply(W_root, multiply(covariance, b))));

      // Simple Newton step
      theta = covariance * a;

      // Check for convergence.
      if (i != 0) objective_old = objective_new;
      objective_new = -0.5 * a.dot(theta)
        + diff_likelihood.log_likelihood(theta, eta);
      double objective_diff = abs(objective_new - objective_old);
      if (objective_diff < tolerance) break;
    }

    return objective_new - sum(L.diagonal().array().log());
  }

  /**
   * For a latent Gaussian model with global parameters phi, latent
   * variables theta, and observations y, this function computes
   * an approximation of the log marginal density, p(y | phi).
   * This is done by marginalizing out theta, using a Laplace
   * approxmation. The latter is obtained by finding the mode,
   * using a custom Newton method, and the Hessian of the likelihood.
   *
   * The convergence criterion for the Newton is a small change in
   * log marginal density. The user controls the tolerance (i.e.
   * threshold under which change is deemed small enough) and
   * maximum number of steps.
   *
   * Wrapper for when the hyperparameters passed as a double.
   *
   * @tparam T type of the initial guess.
   * @tparam D structure type for the likelihood object.
   * @tparam K structure type for the covariance object.
   * @tparam Tx type of spatial data for covariance: in Stan, this can
   *            either be a matrix or an array of vectors.
   * @param[in] D structure to compute and differentiate the log likelihood.
   *            The object stores the sufficient stats for the observations.
   * @param[in] K structure to compute the covariance function.
   * @param[in] phi the global parameter (input for the covariance function).
   * @param[in] x data for the covariance function.
   * @param[in] delta additional fixed real data (input for covariance
   *            function).
   * @param[in] delta_int additional fixed integer data (input for covariance
   *            function).
   * @param[in] theta_0 the initial guess for the mode.
   * @param[in] tolerance the convergence criterion for the Newton solver.
   * @param[in] max_num_steps maximum number of steps for the Newton solver.
   * @return the log maginal density, p(y | phi).
   */
  // TODO: Operands and partials version of this.
  template <typename T, typename D, typename K, typename Tx>
  double
  laplace_marginal_density (const D& diff_likelihood,
                            const K& covariance_function,
                            const Eigen::VectorXd& phi,
                            const Eigen::VectorXd& eta,
                            const Tx& x,
                            const std::vector<double>& delta,
                            const std::vector<int>& delta_int,
                            const Eigen::Matrix<T, Eigen::Dynamic, 1>& theta_0,
                            std::ostream* msgs = nullptr,
                            double tolerance = 1e-6,
                            long int max_num_steps = 100) {
    Eigen::VectorXd theta, W_root, a, l_grad;
    Eigen::MatrixXd L, covariance;
    return laplace_marginal_density(diff_likelihood, covariance_function,
                                    phi, eta, x, delta, delta_int,
                                    covariance,
                                    theta, W_root, L, a, l_grad,
                                    value_of(theta_0), msgs,
                                    tolerance, max_num_steps);
  }

  /**
   * The vari class for the laplace marginal density.
   * The method is adapted from algorithm 5.1 in Rasmussen & Williams,
   * "Gaussian Processes for Machine Learning"
   * with modifications described in my (Charles Margossian)
   * thesis proposal.
   *
   * To make computation efficient, variables produced during the
   * Newton step are stored and reused. To avoid storing these variables
   * for too long, the sensitivies are computed in the constructor, and
   * stored for the chain method. Hence, we store a single small vector,
   * instead of multiple large matrices.
   */
  struct laplace_marginal_density_vari : public vari {
    /* dimension of hyperparameters. */
    int phi_size_;
    /* hyperparameters for covariance K. */
    vari** phi_;
    /* dimension of hyperparameters for likelihood. */
    int eta_size_;
    /* hyperparameters for likelihood. */
    vari** eta_;
    /* the marginal density of the observation, conditional on the
     * globl parameters. */
    vari** marginal_density_;
    /* An object to store the sensitivities of phi. */
    Eigen::VectorXd phi_adj_;
    /* An object to store the sensitivities of eta. */
    Eigen::VectorXd eta_adj_;

    template <typename T, typename K, typename D, typename Tx>
    laplace_marginal_density_vari
      (const D& diff_likelihood,
       const K& covariance_function,
       const Eigen::Matrix<T, Eigen::Dynamic, 1>& phi,
       const Eigen::Matrix<T, Eigen::Dynamic, 1>& eta,
       const Tx& x,
       const std::vector<double>& delta,
       const std::vector<int>& delta_int,
       double marginal_density,
       const Eigen::MatrixXd& covariance,
       const Eigen::VectorXd& theta,
       const Eigen::VectorXd& W_root,
       const Eigen::MatrixXd& L,
       const Eigen::VectorXd& a,
       const Eigen::VectorXd& l_grad,
       std::ostream* msgs = nullptr)
      : vari(marginal_density),
        phi_size_(phi.size()),
        phi_(ChainableStack::instance_->memalloc_.alloc_array<vari*>(
	        phi.size())),
        eta_size_(eta.size()),
        eta_(ChainableStack::instance_->memalloc_.alloc_array<vari*>(
          eta.size())),
        marginal_density_(
          ChainableStack::instance_->memalloc_.alloc_array<vari*>(1)) {
      using Eigen::Matrix;
      using Eigen::Dynamic;
      using Eigen::MatrixXd;
      using Eigen::VectorXd;

      int theta_size = theta.size();
      for (int i = 0; i < phi_size_; i++) phi_[i] = phi(i).vi_;
      for (int i = 0; i < eta_size_; i++) eta_[i] = eta(i).vi_;

      // CHECK -- is there a cleaner way of doing this?
      marginal_density_[0] = this;
      marginal_density_[0] = new vari(marginal_density, false);

      // auto start = std::chrono::system_clock::now();
      Eigen::MatrixXd R;
      {
        Eigen::MatrixXd W_root_diag = W_root.asDiagonal();
        R = W_root_diag *
              L.transpose().triangularView<Eigen::Upper>()
               .solve(L.triangularView<Eigen::Lower>()
                 .solve(W_root_diag));
      }

      Eigen::MatrixXd
        C = mdivide_left_tri<Eigen::Lower>(L,
                  diag_pre_multiply(W_root, covariance));

      Eigen::VectorXd eta_dbl = value_of(eta);
      // CHECK -- should there be a minus sign here?
      Eigen::VectorXd s2 = 0.5 * (covariance.diagonal()
                 - (C.transpose() * C).diagonal())
                 .cwiseProduct(diff_likelihood.third_diff(theta, eta_dbl));

     phi_adj_ = Eigen::VectorXd(phi_size_);
     start_nested();
     try {
       //  = std::chrono::system_clock::now();
       Matrix<var, Dynamic, 1> phi_v = value_of(phi);
       Matrix<var, Dynamic, Dynamic>
         K_var = covariance_function(phi_v, x, delta, delta_int, msgs);
       var Z = laplace_pseudo_target(K_var, a, R, l_grad, s2);

       set_zero_all_adjoints_nested();
       grad(Z.vi_);

       for (int j = 0; j < phi_size_; j++) phi_adj_[j] = phi_v(j).adj();

     } catch (const std::exception& e) {
       recover_memory_nested();
       throw;
     }
     recover_memory_nested();

     if (eta_size_ != 0) {
       VectorXd diff_eta = diff_likelihood.diff_eta(theta, eta_dbl);
       MatrixXd diff_theta_eta = diff_likelihood.diff_theta_eta(theta, eta_dbl);
       MatrixXd diff2_theta_eta
         = diff_likelihood.diff2_theta_eta(theta, eta_dbl);
       for (int l = 0; l < eta_size_; l++) {
         VectorXd b = diff_theta_eta.col(l);
         // CHECK -- can we use the fact the covariance matrix is symmetric?
         VectorXd s3 = b - covariance * (R * b);

         eta_adj_(l) = diff_eta(l) - (W_root.cwiseInverse().asDiagonal()
           * (R * (covariance * diff2_theta_eta.col(l)))).trace()
           - s2.dot(s3);
       }
     }

     // auto end = std::chrono::system_clock::now();
     // std::chrono::duration<double> time = end - ;
     // std::cout << "diffentiation time: " << time.count() << std::endl;

      // Implementation with fwd mode computation of C,
      // and then following R&W's scheme.
      /*
       = std::chrono::system_clock::now();
      covariance_sensitivities<K> f(x, delta, delta_int,
                                    covariance_function, msgs);
      Eigen::MatrixXd diff_cov;
      {
        Eigen::VectorXd covariance_vector;
        jacobian_fwd(f, value_of(phi), covariance_vector, diff_cov);
        // covariance = to_matrix(covariance_vector, theta_size, theta_size);
      }

      phi_adj_ = Eigen::VectorXd(phi_size_);

      for (int j = 0; j < phi_size_; j++) {
        Eigen::VectorXd j_col = diff_cov.col(j);
        C = to_matrix(j_col, theta_size, theta_size);
        double s1 = 0.5 * quad_form(C, a) - 0.5 * sum((R * C).diagonal());
        Eigen::VectorXd b = C * l_grad;
        Eigen::VectorXd s3 = b - covariance * (R * b);
        // std::cout << "old Z: " << s1 + s2.dot(s3) << std::endl;
        phi_adj_[j] = s1 + s2.dot(s3);
      }
      end = std::chrono::system_clock::now();
      time = end - ;
      std::cout << "Former diff: " << time.count() << std::endl;
      */
    }

    void chain() {
      for (int j = 0; j < phi_size_; j++)
        phi_[j]->adj_ += marginal_density_[0]->adj_ * phi_adj_[j];

      for (int l = 0; l < eta_size_; l++)
        eta_[l]->adj_ += marginal_density_[0]->adj_ * eta_adj_[l];
    }
  };

  /**
   * For a latent Gaussian model with global parameters phi, latent
   * variables theta, and observations y, this function computes
   * an approximation of the log marginal density, p(y | phi).
   * This is done by marginalizing out theta, using a Laplace
   * approxmation. The latter is obtained by finding the mode,
   * using a custom Newton method, and the Hessian of the likelihood.
   *
   * The convergence criterion for the Newton is a small change in
   * the log marginal density. The user controls the tolerance (i.e.
   * threshold under which change is deemed small enough) and
   * maximum number of steps.
   *
   * Wrapper for when the global parameter is passed as a double.
   *
   * @tparam T0 type of the initial guess.
   * @tparam T1 type of the global parameters.
   * @tparam D structure type for the likelihood object.
   * @tparam K structure type for the covariance object.
   *@tparam Tx type for the spatial data passed to the covariance.
   * @param[in] D structure to compute and differentiate the log likelihood.
   *            The object stores the sufficient stats for the observations.
   * @param[in] K structure to compute the covariance function.
   * @param[in] phi the global parameter (input for the covariance function).
   * @param[in] x data for the covariance function.
   * @param[in] delta addition real data for covariance function.
   * @param[in] delta_int additional interger data for covariance function.
   * @param[in] theta_0 the initial guess for the mode.
   * @param[in] tolerance the convergence criterion for the Newton solver.
   * @param[in] max_num_steps maximum number of steps for the Newton solver.
   * @return the log maginal density, p(y | phi).
   */
  template <typename T0, typename T1, typename D, typename K, typename Tx>
  T1 laplace_marginal_density
      (const D& diff_likelihood,
       const K& covariance_function,
       const Eigen::Matrix<T1, Eigen::Dynamic, 1>& phi,
       const Tx& x,
       const std::vector<double>& delta,
       const std::vector<int>& delta_int,
       const Eigen::Matrix<T0, Eigen::Dynamic, 1>& theta_0,
       std::ostream* msgs = nullptr,
       double tolerance = 1e-6,
       long int max_num_steps = 100) {
    Eigen::VectorXd theta, W_root, a, l_grad;
    Eigen::MatrixXd L;
    double marginal_density_dbl;
    Eigen::MatrixXd covariance;

    // TEST
    // auto start = std::chrono::system_clock::now();

    marginal_density_dbl
      = laplace_marginal_density(diff_likelihood,
                                 covariance_function,
                                 value_of(phi), x, delta, delta_int,
                                 covariance,
                                 theta, W_root, L, a, l_grad,
                                 value_of(theta_0),
                                 msgs,
                                 tolerance, max_num_steps);

    // TEST
    // auto end = std::chrono::system_clock::now();
    // std::chrono::duration<double> elapsed_time = end - start;
    // std::cout << "Evaluation time: " << elapsed_time.count() << std::endl;

    // TEST
    // start = std::chrono::system_clock::now();

    // construct vari
    laplace_marginal_density_vari* vi0
      = new laplace_marginal_density_vari(diff_likelihood,
                                          covariance_function,
                                          phi, x, delta, delta_int,
                                          marginal_density_dbl,
                                          covariance,
                                          theta, W_root, L, a, l_grad,
                                          msgs);

    var marginal_density = var(vi0->marginal_density_[0]);

    return marginal_density;
  }

}  // namespace math
}  // namespace stan

#endif
