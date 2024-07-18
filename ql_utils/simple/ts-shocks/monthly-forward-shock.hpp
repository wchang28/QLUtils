#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple/ts-shock.hpp>
#include <ql_utils/simple/bootstraps/forward-to-zero-converter.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace QLUtils {
	// apply shock on zero rate curve in monthly forward rates space
	template <
		typename IMPLIED_RATE_CALCULATOR,
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleMonthlyForwardShockTS: public SimpleShockTS<RATE_UNIT, COUPON_FREQ> {
	private:
		typedef SimpleForwardZeroConverter<IMPLIED_RATE_CALCULATOR, RATE_UNIT, COUPON_FREQ> Converter;
	public:
		// output
		std::shared_ptr<MonthlyForwardCurve> pForwardCurve;
		std::shared_ptr<std::vector<QuantLib::Rate>> pMonthlyShocks;
		std::shared_ptr<MonthlyForwardCurve> pForwardCurveShocked;
	public:
		static std::shared_ptr<MonthlyForwardCurve> toForwardCurve(
			const MonthlyZeroRates& monthlyZeroRates
		) {
			return Converter::forwardCurve(monthlyZeroRates, 1);	// monthly forward curve means tenor is 1 month
		}
		SimpleMonthlyForwardShockTS(
			const MonthlyZeroRates& monthlyZeroRates	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
		) : SimpleShockTS<RATE_UNIT, COUPON_FREQ>(monthlyZeroRates)
		{}
		void shock(
			const ISimpleMonthlyShock& monthlyShocker	// shock unit is QuantLib::Rate (decimal)
		) {
			auto multiplier = this->multiplier();
			pForwardCurve = toForwardCurve(this->monthlyZeroRates());
			auto n = pForwardCurve->size();
			pMonthlyShocks.reset(new std::vector<QuantLib::Rate>(n));
			pForwardCurveShocked.reset(new MonthlyForwardCurve(n));
			const auto& forwardCurve = *pForwardCurve;
			auto& monthlyShocks = *pMonthlyShocks;
			auto& forwardCurveShocked = *pForwardCurveShocked;
			for (decltype(n) fwdMonth = 0; fwdMonth < n; ++fwdMonth) {
				auto fwdRate = forwardCurve[fwdMonth];
				fwdRate *= multiplier;	// convert to QuantLib::Rate unit
				auto shock = monthlyShocker(fwdMonth);	// in the unit of QuantLib::Rate
				monthlyShocks[fwdMonth] = shock;
				auto fwdRateShocked = fwdRate + shock;
				fwdRateShocked /= multiplier;	// convert the rate back
				forwardCurveShocked[fwdMonth] = fwdRateShocked;
			}
			this->pMonthlyZeroRatesShocked = Converter::bootstrap(forwardCurveShocked);
		}
		QuantLib::Rate verify(
			std::ostream& os,
			std::streamsize precision = 16
		) const {
			QL_REQUIRE(pForwardCurveShocked != nullptr && this->pMonthlyZeroRatesShocked != nullptr, "shock was not performed");
			auto multiplier = this->multiplier();
			auto pForwardCurveImplied = toForwardCurve(*(this->pMonthlyZeroRatesShocked));
			const auto& forwardCurveActual = *pForwardCurveShocked;
			const auto& forwardCurveImplied = *pForwardCurveImplied;
			auto n = forwardCurveActual.size();
			QL_ASSERT(forwardCurveImplied.size() == n, "the number of forward rates (" << forwardCurveImplied.size() << ") is not what is expected (" << n << ")");
			os << std::fixed;
			os << std::setprecision(precision);
			QuantLib::Rate err = 0.0;
			for (decltype(n) fwdMonth = 0; fwdMonth < n; ++fwdMonth) {
				QuantLib::Rate actual = forwardCurveActual[fwdMonth] * multiplier;
				QuantLib::Rate implied = forwardCurveImplied[fwdMonth] * multiplier;
				auto diff = implied - actual;
				err += std::pow(diff, 2.0);
				os << "fwdMonth=" << fwdMonth;
				os << "," << "actual=" << actual * 100.;
				os << "," << "implied=" << implied * 100.;
				os << "," << "diff=" << diff * 10000. << " bp";
				os << std::endl;
			}
			return std::sqrt(err);
		}
	};
}