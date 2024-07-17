#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/simple-par-yield-calculator.hpp>
#include <ql_utils/simple-par-yield-ts-bootstrap.hpp>
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
	public:
		SimpleParShockTS(
			const MonthlyZeroRates& monthlyZeroRates	// zero rate vectors, compounded in COUPON_FREQ, first element must be the spot zero rate (ie: month=0)
		) : monthlyZeroRates_(monthlyZeroRates)
		{
			auto n = monthlyZeroRates.size();
			QL_REQUIRE(n >= 2, "too few zero rate nodes (" << n << "). The minimum is 2");
		}
		template <typename MONTHLY_PAR_SHOCKER>
		void shock(
			const MONTHLY_PAR_SHOCKER& monthlyParShocker	// shock unit is QuantLib::Rate (decimal)
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
				parYield *= multiplier;	// convert to QuantLib::Rate unit
				auto parShock = monthlyParShocker(tenorMonth);
				parShocks[index] = parShock;
				auto parYieldShocked = parYield + parShock;
				parYieldShocked /= multiplier;	// convert the rate back
				parYieldsShocked[index] = parYieldShocked;
			}
			Bootstrapper bootstrapper(parTenorMonths, parYieldsShocked);
			bootstrapper.bootstrap();
			pMonthlyZeroRatesShocked = bootstrapper.pMonthlyZeroRates;
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