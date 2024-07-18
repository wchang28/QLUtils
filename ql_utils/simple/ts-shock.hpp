#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple/rate-calculator.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <functional>

namespace QLUtils {
	struct ISimpleMonthlyShock {
		virtual QuantLib::Rate operator() (
			size_t month
		) const = 0;
	};

	typedef std::function<QuantLib::Rate(size_t)> SimpleMonthlyShockProc;

	// base class for shocking on the monthly zero rate curve
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleShockTS {
	protected:
		// input
		MonthlyZeroRates monthlyZeroRates_;	// monthly zero rate vectors representing a zero curve, compounded in COUPON_FREQ, in the unit of RATE_UNIT, first element must be the spot zero rate (ie: month=0)
	public:
		// output
		std::shared_ptr<std::vector<double>> pMonthlyZeroRatesShocked;
	public:
		static double multiplier() {
			return SimpleRateCalculator<RATE_UNIT, COUPON_FREQ>::multiplier();
		}
		static double compoundFrequency() {
			return SimpleRateCalculator<RATE_UNIT, COUPON_FREQ>::couponFrequency();
		}
		const MonthlyZeroRates& monthlyZeroRates() const {
			return monthlyZeroRates_;
		}
		SimpleShockTS(
			const MonthlyZeroRates& monthlyZeroRates	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
		) : monthlyZeroRates_(monthlyZeroRates)
		{
			auto n = monthlyZeroRates.size();
			QL_REQUIRE(n >= 2, "too few zero rate nodes (" << n << "). The minimum is 2");
		}
		virtual void shock(
			const SimpleMonthlyShockProc& monthlyShock
		) = 0;
		virtual QuantLib::Rate verify(
			std::ostream& os,
			std::streamsize precision = 16
		) const = 0;
		virtual void shock(
			const ISimpleMonthlyShock& monthlyShocker
		) {
			shock([&monthlyShocker](size_t month) {
				return monthlyShocker(month);
			});
		}
	};
}