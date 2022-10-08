#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <vector>

namespace QLUtils {
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleRateCalculator {
	protected:
		SimpleRateCalculator() {}
	public:
		static double multiplier() {
			auto unit = RATE_UNIT;
			switch (unit) {
			case RateUnit::Decimal:
			default:
				return 1.;
			case RateUnit::Percent:
				return 0.01;
			case RateUnit::BasisPoint:
				return 0.0001;
			}
		}
		static double couponFrequency() {
			return (double)COUPON_FREQ;
		}
		static size_t couponIntervalMonths() {
			return (size_t)12 / (size_t)COUPON_FREQ;
		}
	};
}