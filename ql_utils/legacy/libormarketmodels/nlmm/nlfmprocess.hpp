#pragma once

#include <ql/quantlib.hpp>
#include <vector>
#include <numeric>
#include <cmath>

namespace QuantLib {
    class NLiborForwardModelProcess : public LiborForwardModelProcess {
    private:
        std::vector<Time> accrualPeriod_;
    public:
        NLiborForwardModelProcess(
            Size size,
            ext::shared_ptr<IborIndex> index
        ):LiborForwardModelProcess(size, index), accrualPeriod_(size)
		{
            for (Size i = 0; i < size; ++i) {
                accrualPeriod_[i] = accrualEndTimes()[i] - accrualStartTimes()[i];
            }
        }
		// L[i](t0) to L[i](t0+dt) rates evolution
        Array evolve(
            Time t0,
            const Array& x0,    // length = size
            Time dt,
            const Array& dw // length = factors
        ) const override {
            const Size m = nextIndexReset(t0);
			const Real sdt = std::sqrt(dt); // square root of the time step dt
			Array f(x0);    // L[i](t0+dt), length = size, initialize it with L[i](t0)
            Matrix diff = covarParam()->diffusion(t0, x0);  // diff[k][f](t0) = vol[k](t0) * pseudoSqrt[k][f] (size x factors)
            Matrix covariance = covarParam()->covariance(t0, x0);   // covariance[i][j](t0) = vol[i](t0) * rho[i][j] * vol[j](t0) (size x size)
            const Size size = this->size();
			// at time t0, the forward rates L[0], L[1], ..., L[m-1] are fixed/locked, only L[m], L[m+1], ..., L[size-1] are still evolving (size-m)
			for (Size k = m; k < size; ++k) {   // for each forward rate greater than the next rate reset date (still evolving forward rates)
                Array a(k - m + 1, 0.0);
				for (Size i = m; i <= k; ++i) { // k-m+1 iterations [m, m+1, ..., k]
					const Time tau_i = accrualPeriod_[i];
					a[i - m] = tau_i / (1.0 + tau_i * x0[i]);   // x0[i] = L[i](t0)
                }
                const Real d = std::inner_product(a.begin(), a.end(), covariance.column_begin(k) + m, Real(0.0)) * dt;
				const Real r = std::inner_product(diff.row_begin(k), diff.row_end(k), dw.begin(), Real(0.0)) * sdt; // (1 x factors) * (factors x 1) = 1x1
				const Real increment_k = d + r;   // increment for the k-th forward rate (= deterministic increment + random increment)
                f[k] += increment_k;
            }
            return f;
        }
    };
}
