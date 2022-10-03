#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

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

	// calculate spot/forward par rate/yield give a monthly zero rates vector, compounded in some frequency
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleParRateCalculator {
	public:
		typedef std::vector<double> MonthlyZeroRates;
	private:
		const MonthlyZeroRates& monthlyZeroRates_;	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
	public:
		SimpleParRateCalculator(
			const MonthlyZeroRates& monthlyZeroRates	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
		) :
			monthlyZeroRates_(monthlyZeroRates)
		{
			auto n = monthlyZeroRates_.size();
			QL_REQUIRE(n >= 2, "too few zero rate nodes (" << n << "). The minimum is 2");
		}
	private:
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
	public:
		static double couponFrequency() {
			return (double)COUPON_FREQ;
		}
		static size_t couponIntervalMonths() {
			return (size_t)12 / (size_t)COUPON_FREQ;
		}
		double operator() (
			size_t tenorMonth,
			size_t fwdMonth = 0
		) const {
			auto freq = couponFrequency();
			auto n = monthlyZeroRates_.size();
			QL_REQUIRE(tenorMonth > 0, "tenor in month (" << tenorMonth << ") must be greater than zero");
			QL_REQUIRE(fwdMonth >= 0, "forward in month (" << fwdMonth << ") cannot be negative");
			auto lastRelevantMonth = fwdMonth + tenorMonth;
			QL_REQUIRE(lastRelevantMonth < n, "forward+tenor (" << (lastRelevantMonth) << ") is over the limit (" << (n - 1) << ")");
			auto t_0 = (QuantLib::Time)fwdMonth / 12.;
			auto zr_0 = monthlyZeroRates_[fwdMonth] * multiplier();
			auto df_0 = std::pow(1.0 + zr_0 / freq, -t_0 * freq);
			if (tenorMonth <= 12) {
				if (fwdMonth == 0) {
					return monthlyZeroRates_[tenorMonth];
				}
				else {
					auto month = lastRelevantMonth;
					auto zr = monthlyZeroRates_[month] * multiplier();
					auto t = (QuantLib::Time)month / 12.;
					auto df = std::pow(1.0 + zr / freq, -t * freq);
					df /= df_0;
					t = (QuantLib::Time)tenorMonth / 12.;
					zr = (std::pow(df, -1. / (t * freq)) - 1.0) * freq;
					return zr / multiplier();
				}
			}
			else { // tenorMonth > 12
				auto cpnIntrvlMonths = couponIntervalMonths();
				auto start = fwdMonth + (tenorMonth % cpnIntrvlMonths == 0 ? cpnIntrvlMonths : tenorMonth % cpnIntrvlMonths);
				auto prevMonth = fwdMonth;
				auto last_df = 1.;
				auto sum = 0.;
				for (auto month = start; month <= lastRelevantMonth; month += cpnIntrvlMonths) {
					auto d_months = month - prevMonth;
					auto dt = (QuantLib::Time)d_months / 12.;
					auto zr = monthlyZeroRates_[month] * multiplier();
					auto t = (QuantLib::Time)month / 12.;
					auto df = std::pow(1.0 + zr / freq, -t * freq);
					df /= df_0;
					sum += dt * df;
					prevMonth = month;
					last_df = df;
				}
				auto r = (1. - last_df) / sum;
				return r / multiplier();
			}
		}
	};
}