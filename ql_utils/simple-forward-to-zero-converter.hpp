#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple-rate-calculator.hpp>
#include <memory>
#include <cmath>

namespace QLUtils {
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleForwardZeroConverter {
	private:
		using SimpleRateCalculator = SimpleRateCalculator<RATE_UNIT, COUPON_FREQ>;
	private:
		SimpleForwardZeroConverter() {}
	public:
		static std::shared_ptr<MonthlyZeroRates> toZeroRates(
			const MonthlyForwardCurve& monthlyFwdCurve	// assuming tenor is 1 month and the fwd rate is simple interest rate
		) {
			auto n_forwards = monthlyFwdCurve.size();
			QL_ASSERT(n_forwards >= 1, "forward curve is empty");
			auto freq = SimpleRateCalculator::couponFrequency();
			auto multiplier = SimpleRateCalculator::multiplier();
			auto n_zeros = n_forwards + 1;	// n_zeros >= 2
			std::shared_ptr<MonthlyZeroRates> ret(new MonthlyZeroRates(n_zeros));
			auto& zeroRates = *ret;
			for (decltype(n_zeros) month = 0; month < n_zeros - 1; ++month) {	// n_zeros - 1 iterations (at least one), month_max = n_zeros - 2 = n_forwards - 1
				auto nextMonth = month + 1;
				auto t_0 = (QuantLib::Time)month / 12.;
				auto t_1 = (QuantLib::Time)(month + 1) / 12.;
				auto dt = t_1 - t_0;
				auto a_0 = (month == 0 ? 1. : std::pow(1. + zeroRates[month] * multiplier / freq, t_0 * freq));
				auto a_f = 1. + monthlyFwdCurve[month] * multiplier * dt;
				auto a_1 = a_0 * a_f;
				auto zr = (std::pow(a_1, 1. / (t_1 * freq)) - 1.) * freq;
				zeroRates[nextMonth] = zr / multiplier;
			}
			zeroRates[0] = zeroRates[1];
			return ret;
		}
		static std::shared_ptr<MonthlyForwardCurve> toForwardCurve(
			const MonthlyZeroRates& monthlyZeroRates
		) {
			auto n_zeros = monthlyZeroRates.size();
			QL_REQUIRE(n_zeros >= 2, "too few zero rate nodes (" << n_zeros << "). The minimum is 2");
			auto freq = SimpleRateCalculator::couponFrequency();
			auto multiplier = SimpleRateCalculator::multiplier();
			auto n = n_zeros - 1;
			std::shared_ptr<MonthlyForwardCurve> ret(new MonthlyForwardCurve(n));
			auto& monthlyFwdCurve = *ret;
			for (decltype(n) fwdMonth = 0; fwdMonth < n; ++fwdMonth) {
				auto t_0 = (QuantLib::Time)fwdMonth / 12.;
				auto t_1 = (QuantLib::Time)(fwdMonth + 1) / 12.;
				auto dt = t_1 - t_0;
				auto ac_0 = std::pow(1. + monthlyZeroRates[fwdMonth] * multiplier / freq, t_0 * freq);
				auto ac_1 = std::pow(1. + monthlyZeroRates[fwdMonth + 1] * multiplier / freq, t_1 * freq);
				auto ac_f = ac_1 / ac_0;
				auto fwdRate = (ac_f - 1.) / dt;
				monthlyFwdCurve[fwdMonth] = fwdRate / multiplier;
			}
			return ret;
		}
	};
}