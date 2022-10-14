#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple-rate-calculator.hpp>
#include <cmath>

namespace QLUtils {
	// calculate spot/forward simple rate give a monthly zero rates vector, compounded in some frequency
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleSimpleRateCalculator:
		public SimpleRateCalculator<RATE_UNIT, COUPON_FREQ> {
	public:
		SimpleSimpleRateCalculator(
			const MonthlyZeroRates& monthlyZeroRates	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
		): SimpleRateCalculator<RATE_UNIT, COUPON_FREQ>(monthlyZeroRates) {}
		double operator() (
			size_t tenorMonth,
			size_t fwdMonth = 0
		) const {
			this->checkForwardBounds(tenorMonth, fwdMonth);
			auto freq = this->couponFrequency();
			auto multiplier = this->multiplier();
			const auto& monthlyZeroRates = this->monthlyZeroRates_;
			auto lastRelevantMonth = fwdMonth + tenorMonth;
			auto t_0 = (QuantLib::Time)fwdMonth / 12.;
			auto zr_0 = monthlyZeroRates[fwdMonth] * multiplier;
			auto df_0 = std::pow(1.0 + zr_0 / freq, -t_0 * freq);
			auto t_1 = (QuantLib::Time)lastRelevantMonth / 12.;
			auto zr_1 = monthlyZeroRates[lastRelevantMonth] * multiplier;
			auto df_1 = std::pow(1.0 + zr_1 / freq, -t_1 * freq);
			auto df = df_1 / df_0;
			auto dt = t_1 - t_0;
			auto r = (1./ df - 1.) / dt;
			return r / multiplier;
		}
	};
}