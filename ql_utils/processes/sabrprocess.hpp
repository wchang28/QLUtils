#pragma once

#include <ql/quantlib.hpp>
#include <cmath>

namespace QuantLib {
	class SABRProcess : public StochasticProcess {
	private:
		Rate F0_;
		Real alpha_;
		Real beta_ ;
		Real nu_;
		Real rho_;
		Real shift_;
		Matrix pseudoSqrt_;
	public:
		SABRProcess(
			Rate F0,
			Real alpha,
			Real beta,
			Real nu,
			Real rho,
			Real shift = 0.0
		)
			: StochasticProcess(ext::shared_ptr<discretization>(new EulerDiscretization)),
			F0_(F0), alpha_(alpha), beta_(beta), nu_(nu), rho_(rho), shift_(shift)
		{
			QL_REQUIRE(alpha_ >= 0.0, "SABR alpha (" << alpha_ << ") must be non-negative");
			QL_REQUIRE(beta_ >= 0.0 && beta_ <= 1.0, "SABR beta (" << beta_ << ") must be in [0,1]");
			QL_REQUIRE(nu_ >= 0.0, "SABR nu (" << nu_ << ") must be non-negative");
			QL_REQUIRE(rho_ >= -1.0 && rho_ <= 1.0, "SABR rho (" << rho_ << ") must be in[-1, 1]");
			if (beta_ != 0.0) {
				QL_REQUIRE(F0_ >= 0.0, "SABR model with non-zero beta (" << beta_ << ") cannot have non-negative initial forward rate (" << io::percent(F0_) << ")");
			}
			// pseudo square root of the correlation matrix via Cholesky decomposition
			// CorrMatrix = L * transpose(L)
			// where L is a lower triangular matrix
			Matrix m(2, 2, 0.0);
			m[0][0] = 1.0;
			m[0][1] = 0.0;
			m[1][0] = rho_;
			m[1][1] = std::sqrt(1 - rho_ * rho_);
			pseudoSqrt_ = m;
		}
		Size size() const override {
			return 2;
		}
		Size factors() const override {
			return 2;
		}
		Array initialValues() const override {
			Array init(2);
			init[0] = F0_;
			init[1] = alpha_;
			return init;
		}
		Array drift(Time, const Array&) const {
			return Array(size(), 0.0);
		}
		Matrix diffusion(Time, const Array&) const {
			Matrix m(size(), factors(), 0.0);
			return m;
		}
		const Rate& F0() const {
			return F0_;
		}
		const Real& alpha() const {
			return alpha_;
		}
		const Real& beta() const {
			return beta_;
		}
		const Real& nu() const {
			return nu_;
		}
		const Real& rho() const {
			return rho_;
		}
		const Real& shift() const {
			return shift_;
		}
		bool isNormal() const {
			return (beta_ == 0.0);
		}
		bool isElastic() const {
			return (beta_ != 0.0);
		}
		const Matrix& pseudoSqrt() const {
			return pseudoSqrt_;
		}
		Array evolve(Time t0, const Array& x0, Time dt, const Array& dw) const override {
			const Real F_0 = x0[0];		// F @ t0
			const Real sigma_0 = x0[1];	// sigma @ t0

			if (beta_ != 0.0 && F_0+shift_ < 0.0) {	// std::pow(F_0+shift_, beta_) would produce NaN
				QL_FAIL("SABR model with non-zero beta (" << beta_ << ") cannot handle negative shifted forward rate (" << io::percent(F_0 + shift_) << ") during simulation: t0=" << t0);
			}

			const Real sdt = std::sqrt(dt);
			Array x1 = x0;			// asset @ t0+dt (to be returned)
			Real& F_1 = x1[0];		// F @ t0+dt (non-const reference)
			Real& sigma_1 = x1[1];	// sigma @ t0+dt (non-const reference)

			const Array& Z = dw;
			Array W = pseudoSqrt_ * Z;
			const Real W1 = W[0];
			const Real W2 = W[1];
			// Milstein discretization scheme for F
			const Real dW1 = sdt * W1;
			const Real firstOrderTerm = sigma_0 * (beta_ == 0.0 ? 1.0 : std::pow(F_0+shift_, beta_)) * dW1;
			const Real secondOrderTerm = (beta_ == 0.0 ? 0.0 : 0.5 * beta_ * sigma_0 * sigma_0 * std::pow(F_0+ shift_, 2.0 * beta_ - 1) * (dW1 * dW1 - dt));
			const Real dF = firstOrderTerm + secondOrderTerm;
			F_1 += dF;
			// Log-Euler discretization scheme for sigma
			const Real dW2 = sdt * W2;
			sigma_1 = sigma_0 * std::exp(-0.5 * nu_ * nu_ * dt + nu_ * dW2);

			return x1;
		}
	};
}
