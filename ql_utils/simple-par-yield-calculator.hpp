#pragma once

#include <ql/quantlib.hpp>
#include <algorithm>

namespace QLUtils {
	class SimpleParYieldCalculator {
	public:
		static QuantLib::Rate parYield(
			const QuantLib::YieldTermStructure& yieldCurve,
			const std::size_t& tenorMonth,
			const QuantLib::Period& forward = QuantLib::Period(0, QuantLib::Days),
			const QuantLib::Frequency parCouponFreq = QuantLib::Semiannual
		) {
			QL_ASSERT(tenorMonth > 0, "par tenor in month must be greater than 0");
			auto today = yieldCurve.referenceDate();
			auto settlemenDate = today + forward;
			QuantLib::Period tenor(tenorMonth, QuantLib::Months);
			auto maturityDate = settlemenDate + tenor;
			auto dfSettle = yieldCurve.discount(settlemenDate);
			if (tenorMonth <= 12) {	// for tenor <= 1yr, zero rate is par yield
				auto zeroRateSemiAnnual = yieldCurve.forwardRate(
					settlemenDate,
					maturityDate,
					QuantLib::Thirty360(QuantLib::Thirty360::ISDA),
					QuantLib::Compounded,
					parCouponFreq
				);
				auto parYield = zeroRateSemiAnnual;
				return parYield;
			}
			else {	// for tenor > 1yr => none-zero coupon, price at par
				int couponIntervalMonths = 12 / (int)parCouponFreq;
				auto sum = 0.;
				auto dfLast = 1.;
				for (int i = (int)tenorMonth; i > 0; i -= couponIntervalMonths) { // for each cashflow at coupon interval
					if (i == tenorMonth) {	// at maturity date
						dfLast = yieldCurve.discount(maturityDate) / dfSettle;
					}
					auto cf_date = settlemenDate + QuantLib::Period(i, QuantLib::Months);
					auto df = yieldCurve.discount(cf_date) / dfSettle;
					auto dt = (double)(std::min(couponIntervalMonths, i)) / 12.0;
					sum += dt * df;
				}
				auto parYield = (QuantLib::Rate)((1. - dfLast) / sum);
				return parYield;
			}
		}
	};
}