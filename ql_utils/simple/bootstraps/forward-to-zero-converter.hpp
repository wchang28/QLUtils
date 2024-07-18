#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple/rate-calculators/fwd-rate-calculator.hpp>
#include <memory>
#include <cmath>

namespace QLUtils {
	// convert between montly zero rates and monthly forward curve
	template <
		typename IMPLIED_RATE_CALCULATOR = NominalSimpleImpliedRateCalculator,
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleForwardZeroConverter {
	private:
		using FwdRateCalculator = SimpleForwardRateCalculator<IMPLIED_RATE_CALCULATOR, RATE_UNIT, COUPON_FREQ>;
	private:
		SimpleForwardZeroConverter() {}
	public:
		// bootstrap a monthly forward curve to a monthly zero rate curve
		static std::shared_ptr<MonthlyZeroRates> bootstrap(
			const MonthlyForwardCurve& monthlyFwdCurve	// assuming tenor is 1 month and the fwd rate is interest rate calculated using IMPLIED_RATE_CALCULATOR
		) {
			IMPLIED_RATE_CALCULATOR impliedRateCalculator;
			auto n_forwards = monthlyFwdCurve.size();
			QL_REQUIRE(n_forwards > 0, "forward curve is empty");
			auto freq = FwdRateCalculator::couponFrequency();
			auto multiplier = FwdRateCalculator::multiplier();
			auto n_zeros = n_forwards + 1;	// n_forwards >= 1 => n_zeros >= 2
			std::shared_ptr<MonthlyZeroRates> ret(new MonthlyZeroRates(n_zeros));
			auto& zeroRates = *ret;
			for (decltype(n_zeros) month = 0; month < n_zeros - 1; ++month) {	// n_zeros - 1 iterations (at least one), month_max = n_zeros - 2 = n_forwards - 1
				auto nextMonth = month + 1;
				auto t_0 = (QuantLib::Time)month / 12.;
				auto t_1 = (QuantLib::Time)nextMonth / 12.;
				auto dt = t_1 - t_0;
				QuantLib::Real compounding_0 = (month == 0 ? 1. : std::pow(1. + zeroRates[month] * multiplier / freq, t_0 * freq));
				QuantLib::Rate fwdRate = monthlyFwdCurve[month] * multiplier;
				QuantLib::Real compounding_fwd = impliedRateCalculator.compounding(fwdRate, dt);	// calculate the forward compounding factor giving the forward rate
				QuantLib::Real compounding_1 = compounding_0 * compounding_fwd;
				QuantLib::Rate zr_1 = (std::pow(compounding_1, 1. / (t_1 * freq)) - 1.) * freq;
				zeroRates[nextMonth] = zr_1 / multiplier;
			}
			zeroRates[0] = zeroRates[1];
			return ret;
		}
		static std::shared_ptr<MonthlyForwardCurve> forwardCurve(
			const MonthlyZeroRates& monthlyZeroRates,
			size_t tenorMonth = 1
		) {
			QL_REQUIRE(tenorMonth > 0, "tenor in month (" << tenorMonth << ") must be greater than zero");
			FwdRateCalculator fwdRateCalculator(monthlyZeroRates);
			auto n_zeros = monthlyZeroRates.size(); // n_zeros >= 2
			QL_REQUIRE(tenorMonth < n_zeros, "tenor in month (" << tenorMonth << ") is over the limit (" << (n_zeros-1) << ")");
			auto n_forwards = n_zeros - tenorMonth;	// n_forwards > 0
			std::shared_ptr<MonthlyForwardCurve> ret(new MonthlyForwardCurve(n_forwards));
			auto& monthlyFwdCurve = *ret;
			for (decltype(n_forwards) fwdMonth = 0; fwdMonth < n_forwards; ++fwdMonth) {
				monthlyFwdCurve[fwdMonth] = fwdRateCalculator(tenorMonth, fwdMonth);
			}
			return ret;
		}
	};
}