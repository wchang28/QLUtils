#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>

namespace QLUtils {
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleRateCalculator {
	protected:
		const MonthlyZeroRates& monthlyZeroRates_;	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
	protected:
		SimpleRateCalculator(
			const MonthlyZeroRates& monthlyZeroRates
		): monthlyZeroRates_(monthlyZeroRates)
		{
			auto n = monthlyZeroRates.size();
			QL_REQUIRE(n >= 2, "too few zero rate nodes (" << n << "). The minimum is 2");
		}
		void checkForwardBounds(
			size_t tenorMonth,
			size_t fwdMonth
		) const {
			QL_REQUIRE(tenorMonth > 0, "tenor in month (" << tenorMonth << ") must be greater than zero");
			auto n_zeros = monthlyZeroRates_.size();
			auto lastRelevantMonth = fwdMonth + tenorMonth;
			QL_REQUIRE(lastRelevantMonth < n_zeros, "forward+tenor (" << (lastRelevantMonth) << ") is over the limit (" << (n_zeros - 1) << ")");
		}
	public:
		const MonthlyZeroRates& monthlyZeroRates() const {
			return monthlyZeroRates_;
		}
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