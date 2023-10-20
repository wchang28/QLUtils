#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple-rate-calculator.hpp>
#include <algorithm>
#include <cmath>

namespace QLUtils {
	// calculate spot/forward par rate/yield give a monthly zero rates vector, compounded in some frequency
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleParRateCalculator:
		public SimpleRateCalculator<RATE_UNIT, COUPON_FREQ> {
	public:
		SimpleParRateCalculator(
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
			if (tenorMonth <= 12) {
				if (fwdMonth == 0) {
					return monthlyZeroRates[tenorMonth];
				}
				else {
					auto month = lastRelevantMonth;
					auto zr = monthlyZeroRates[month] * multiplier;
					auto t = (QuantLib::Time)month / 12.;
					auto df = std::pow(1.0 + zr / freq, -t * freq);
					df /= df_0;
					t = (QuantLib::Time)tenorMonth / 12.;
					zr = (std::pow(df, -1. / (t * freq)) - 1.0) * freq;
					return zr / multiplier;
				}
			}
			else { // tenorMonth > 12
				auto cpnIntrvlMonths = this->couponIntervalMonths();
				auto start = fwdMonth + (tenorMonth % cpnIntrvlMonths == 0 ? cpnIntrvlMonths : tenorMonth % cpnIntrvlMonths);
				auto prevMonth = fwdMonth;
				auto last_df = 1.;
				auto sum = 0.;
				for (auto month = start; month <= lastRelevantMonth; month += cpnIntrvlMonths) {
					auto d_months = month - prevMonth;
					auto dt = (QuantLib::Time)d_months / 12.;
					auto zr = monthlyZeroRates[month] * multiplier;
					auto t = (QuantLib::Time)month / 12.;
					auto df = std::pow(1.0 + zr / freq, -t * freq);
					df /= df_0;
					sum += dt * df;
					prevMonth = month;
					last_df = df;
				}
				auto r = (1. - last_df) / sum;
				return r / multiplier;
			}
		}
	};
}