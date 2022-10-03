#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple-par-yield-calculator.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace QLUtils {
	template <
		RateUnit RATE_UNIT = RateUnit::Percent,
		QuantLib::Frequency COUPON_FREQ = QuantLib::Frequency::Semiannual
	>
	class SimpleParShockTS {
	public:
		typedef std::vector<double> MonthlyZeroRates;
	private:
		using ParRateCalculator = SimpleParRateCalculator<RATE_UNIT, COUPON_FREQ>;
		using Bootstrapper = SimpleParYieldTSBootstrapper<RATE_UNIT, COUPON_FREQ>;
	private:
		const MonthlyZeroRates& monthlyZeroRates_;	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
	public:
		// output
		std::shared_ptr<std::vector<size_t>> pParTenorMonths;
		std::shared_ptr<std::vector<double>> pParYields;
		std::shared_ptr<std::vector<QuantLib::Rate>> pParShocks;
		std::shared_ptr<std::vector<double>> pParYieldsShocked;
		std::shared_ptr<std::vector<double>> pMonthlyZeroRatesShocked;
		QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<QuantLib::Linear>> pZeroCurveShocked;
	public:
		SimpleParShockTS(
			const MonthlyZeroRates& monthlyZeroRates	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
		) : monthlyZeroRates_(monthlyZeroRates)
		{
			QL_REQUIRE(!monthlyZeroRates.empty(), "monthly zero rates vector is empty");
		}
		template <typename MONTHLY_PAR_SHOCKER>
		void shock(
			const MONTHLY_PAR_SHOCKER& monthlyParShocker,
			bool buildZeroCurve = false,
			const QuantLib::Date& curveReferenceDate = QuantLib::Date()
		) {
			ParRateCalculator parRateCalculator(monthlyZeroRates_);
			auto n = monthlyZeroRates_.size();
			pParTenorMonths.reset(new std::vector<size_t>(n - 1));
			pParYields.reset(new std::vector<double>(n - 1));
			pParShocks.reset(new std::vector<QuantLib::Rate>(n - 1));
			pParYieldsShocked.reset(new std::vector<double>(n - 1));
			auto& parTenorMonths = *pParTenorMonths;
			auto& parYields = *pParYields;
			auto& parShocks = *pParShocks;
			auto& parYieldsShocked = *pParYieldsShocked;
			auto multiplier = ParRateCalculator::multiplier();
			for (decltype(n) tenorMonth = 1; tenorMonth < n; ++tenorMonth) {	// for each month
				auto index = tenorMonth - 1;
				parTenorMonths[index] = tenorMonth;
				auto parYield = parRateCalculator(tenorMonth);
				parYields[index] = parYield;
				parYield *= multiplier;	// convert to QuantLib::Rate
				auto parShock = monthlyParShocker(tenorMonth);
				parShocks[index] = parShock;
				auto parYieldShocked = parYield + parShock;
				parYieldShocked /= multiplier;
				parYieldsShocked[index] = parYieldShocked;
			}
			Bootstrapper bootstrapper(parTenorMonths, parYieldsShocked);
			bootstrapper.bootstrap(buildZeroCurve, curveReferenceDate);
			pMonthlyZeroRatesShocked = bootstrapper.pMonthlyZeroRates;
			pZeroCurveShocked = bootstrapper.pZeroCurve;
		}

		QuantLib::Rate verify(
			std::ostream& os,
			std::streamsize precision = 16
		) const {
			QL_REQUIRE(pParTenorMonths != nullptr && pParYieldsShocked != nullptr && pMonthlyZeroRatesShocked != nullptr, "perform par shock first first");
			const auto& parTenorMonths = *pParTenorMonths;
			const auto& parYields = *pParYieldsShocked;
			const auto& zeroRates = *pMonthlyZeroRatesShocked;
			auto n = parTenorMonths.size();
			QL_ASSERT(parYields.size() == n, "the number of par yields (" << parYields.size() << ") is not what is expected (" << n << ")");
			ParRateCalculator parRateCalculator(zeroRates);
			auto multiplier = ParRateCalculator::multiplier();
			os << std::fixed;
			os << std::setprecision(precision);
			QuantLib::Rate err = 0.0;
			for (decltype(n) i = 0; i < n; ++i) {
				const auto& month = parTenorMonths[i];
				auto actual = parYields[i] * multiplier;
				auto implied = parRateCalculator(month) * multiplier;
				auto diff = implied - actual;
				err += std::pow(diff, 2.0);
				os << "maturity=" << month;
				os << "," << "actual=" << actual * 100.;
				os << "," << "implied=" << implied * 100.;
				os << "," << "diff=" << diff * 10000. << " bp";
				os << std::endl;
			}
			return std::sqrt(err);
		}
	};
}