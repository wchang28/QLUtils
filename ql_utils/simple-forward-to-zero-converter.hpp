#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple-rate-calculator.hpp>
#include <ql_utils/simple-simple-rate-calculator.hpp>
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
		using SimpleSimpleRateCalculator = SimpleSimpleRateCalculator<RATE_UNIT, COUPON_FREQ>;
	private:
		SimpleForwardZeroConverter() {}
	public:
		static std::shared_ptr<MonthlyZeroRates> toZeroRates(
			const MonthlyForwardCurve& monthlyFwdCurve	// assuming tenor is 1 month and the fwd rate is simple interest rate
		) {
			auto n_forwards = monthlyFwdCurve.size();
			QL_REQUIRE(n_forwards > 0, "forward curve is empty");
			auto freq = SimpleRateCalculator::couponFrequency();
			auto multiplier = SimpleRateCalculator::multiplier();
			auto n_zeros = n_forwards + 1;	// n_forwards >= 1 => n_zeros >= 2
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
			const MonthlyZeroRates& monthlyZeroRates,
			size_t tenorMonth = 1
		) {
			QL_REQUIRE(tenorMonth > 0, "tenor in month (" << tenorMonth << ") must be greater than zero");
			SimpleSimpleRateCalculator simpleRateCalculator(monthlyZeroRates);
			auto n_zeros = monthlyZeroRates.size(); // n_zeros >= 2
			QL_REQUIRE(tenorMonth < n_zeros, "tenor in month (" << tenorMonth << ") is over the limit (" << (n_zeros-1) << ")");
			auto n_forwards = n_zeros - tenorMonth;	// n_forwards > 0
			std::shared_ptr<MonthlyForwardCurve> ret(new MonthlyForwardCurve(n_forwards));
			auto& monthlyFwdCurve = *ret;
			for (decltype(n_forwards) fwdMonth = 0; fwdMonth < n_forwards; ++fwdMonth) {
				monthlyFwdCurve[fwdMonth] = simpleRateCalculator(tenorMonth, fwdMonth);
			}
			return ret;
		}
	};
}