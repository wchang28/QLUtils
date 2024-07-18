#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple/rate-calculator.hpp>
#include <cmath>

namespace QLUtils {
	// calculate spot/forward rate give a monthly zero rates vector, compounded in some frequency
	template <
		typename IMPLIED_RATE_CALCULATOR,
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleForwardRateCalculator:
		public SimpleRateCalculator<RATE_UNIT, COUPON_FREQ> {
	private:
		IMPLIED_RATE_CALCULATOR impliedRateCalculator_;
	public:
		SimpleForwardRateCalculator(
			const MonthlyZeroRates& monthlyZeroRates	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
		): SimpleRateCalculator<RATE_UNIT, COUPON_FREQ>(monthlyZeroRates) {}
		double operator() (
			size_t tenorMonth,
			size_t fwdMonth = 0
		) const {
			this->checkForwardBounds(tenorMonth, fwdMonth);
			auto freq = this->couponFrequency();	// zero rate's compounding frequency
			auto multiplier = this->multiplier();
			const auto& monthlyZeroRates = this->monthlyZeroRates();
			auto lastRelevantMonth = fwdMonth + tenorMonth;
			auto t_0 = (QuantLib::Time)fwdMonth / 12.;
			QuantLib::Rate zr_0 = monthlyZeroRates[fwdMonth] * multiplier;
			QuantLib::DiscountFactor df_0 = std::pow(1. + zr_0 / freq, -t_0 * freq);
			auto t_1 = (QuantLib::Time)lastRelevantMonth / 12.;
			QuantLib::Rate zr_1 = monthlyZeroRates[lastRelevantMonth] * multiplier;
			QuantLib::DiscountFactor df_1 = std::pow(1. + zr_1 / freq, -t_1 * freq);
			QuantLib::Real compounding = df_0 / df_1;
			auto dt = t_1 - t_0;
			QuantLib::Rate r = impliedRateCalculator_(compounding, dt);
			return r / multiplier;
		}
	};
	// implied simple rate calculator
	struct NominalSimpleImpliedRateCalculator {
		QuantLib::Rate operator() (
			QuantLib::Real compounding,
			QuantLib::Time t
		) const {
			return (compounding - 1.) / t;
		}
		QuantLib::Real compounding(
			QuantLib::Rate r,
			QuantLib::Time t
		) const {
			return (1. + r * t);
		}
	};
	// implied continuously-compounded rate calculator
	struct NominalContinuouslyCompoundedImpliedRateCalculator {
		QuantLib::Rate operator() (
			QuantLib::Real compounding,
			QuantLib::Time t
		) const {
			return std::log(compounding) / t;
		}
		QuantLib::Real compounding(
			QuantLib::Rate r,
			QuantLib::Time t
		) const {
			return std::exp(r * t);
		}
	};
	// implied compounded rate calculator
	template <
		QuantLib::Frequency COMPOUNDING_FREQ = QuantLib::Frequency::Semiannual
	>
	struct NominalCompoundedImpliedRateCalculator {
		QuantLib::Rate operator() (
			QuantLib::Real compounding,
			QuantLib::Time t
		) const {
			auto frequency = (QuantLib::Real)COMPOUNDING_FREQ;
			return (std::pow(compounding, 1. / (t * frequency)) - 1.) * frequency;
		}
		QuantLib::Real compounding(
			QuantLib::Rate r,
			QuantLib::Time t
		) const {
			auto frequency = (QuantLib::Real)COMPOUNDING_FREQ;
			return std::pow(1. + r/frequency, t * frequency);
		}
	};
}