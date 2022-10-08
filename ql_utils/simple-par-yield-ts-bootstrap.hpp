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
	class SimpleParYieldTSBootstrapper {
	private:
		using ParRateCalculator = SimpleParRateCalculator<RATE_UNIT, COUPON_FREQ>;
	private:
		// input
		std::vector<size_t> maturityMonths_;
		std::vector<double> parYields_;
	public:
		// output
		std::shared_ptr<std::vector<double>> pMonthlySplinedParYields;
		std::shared_ptr<std::vector<double>> pMonthlyZeroRates;
		QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<QuantLib::Linear>> pZeroCurve;
	public:
		SimpleParYieldTSBootstrapper(
			const std::vector<size_t>& maturityMonths,
			const std::vector<double>& parYields
		) : maturityMonths_(maturityMonths),
			parYields_(parYields)
		{
			QL_REQUIRE(maturityMonths.size() == parYields.size(), "maturities vector (" << maturityMonths.size() << ") and par yields vector (" << parYields_.size() << ") must have the same length");
			QL_REQUIRE(!maturityMonths.empty(), "par term structure is empty");
		}
	public:
		void bootstrap(
			bool buildZeroCurve = false,
			const QuantLib::Date& curveReferenceDate = QuantLib::Date()
		) {
			auto multiplier = ParRateCalculator::multiplier();
			auto maxMonth = maturityMonths_.back();
			if (maturityMonths_.size() == 1) {	// only one node in the par yield term structure => cannot spline
				QL_REQUIRE(maturityMonths_[0] == 1, "the only maturity month has to be month 1");
				pMonthlySplinedParYields.reset(new std::vector<double>(2));
				auto& parYields = *pMonthlySplinedParYields;
				parYields[1] = parYields_[0];
				parYields[0] = parYields[1];
				pMonthlyZeroRates.reset(new std::vector<double>(2));
				auto& zeroRates = *pMonthlyZeroRates;
				zeroRates[1] = parYields[1];
				zeroRates[0] = parYields[0];
			}
			else {	// more then 1 node in the par yield term structure
				std::vector<QuantLib::Time> terms;
				for (auto const& maturityMonth : maturityMonths_) {	// for each maturity
					auto term = (QuantLib::Time)maturityMonth / 12.;
					terms.push_back(term);
				}
				QuantLib::Cubic naturalCubicSpline(QuantLib::CubicInterpolation::Spline, false, QuantLib::CubicInterpolation::SecondDerivative, 0.0, QuantLib::CubicInterpolation::SecondDerivative, 0.0);
				auto parInterp = naturalCubicSpline.interpolate(terms.begin(), terms.end(), parYields_.begin());
				pMonthlySplinedParYields.reset(new std::vector<double>(maxMonth + 1));
				auto& parYields = *pMonthlySplinedParYields;
				for (decltype(maxMonth) month = 1; month <= maxMonth; month++) {	// for each consecutive month all the way to maxMonth
					auto term = (QuantLib::Time)month / 12.;
					auto parYield = parInterp(term, true);
					parYields[month] = parYield;
				}
				parYields[0] = parYields[1];
				pMonthlyZeroRates.reset(new std::vector<double>(parYields.size()));
				auto& zeroRates = *pMonthlyZeroRates;
				for (decltype(maxMonth) month = 0; month <= std::min((size_t)12, maxMonth); ++month) {
					zeroRates[month] = parYields[month];
				}
				auto freq = ParRateCalculator::couponFrequency();
				auto cpnIntrvlMonths = ParRateCalculator::couponIntervalMonths();
				// solve the discount factor at the given month
				auto solveLastDiscountFactorForPar = [&parYields, &zeroRates, &freq, &cpnIntrvlMonths, &multiplier](const std::size_t& month) -> QuantLib::DiscountFactor {
					// month >= 13
					double sum = 0.;
					for (int i = (int)month - cpnIntrvlMonths; i > 0; i -= cpnIntrvlMonths) {	// for each cashflow @cpnIntrvlMonths months interval except the last one
						auto zeroRate = zeroRates[i] * multiplier;
						auto t = (QuantLib::Time)i / 12.;
						auto df = 1. / std::pow(1. + zeroRate / freq, t * freq);
						auto dt = (QuantLib::Time)(std::min(cpnIntrvlMonths, (size_t)i)) / 12.;
						sum += dt * df;
					}
					// parYield * sum + parYield / freq * dfLast + dfLast = 1
					// => parYield * sum + (1 + parYield / freq) * dfLast = 1
					// => (1 + parYield / freq) * dfLast = 1 - parYield * sum
					// => dfLast = (1 - parYield * sum)/(1 + parYield / freq)
					auto parYield = parYields[month] * multiplier;
					auto rhs = 1. - parYield * sum;
					auto dfLast = rhs / (1. + parYield / freq);
					return dfLast;
				};
				for (decltype(maxMonth) month = 13; month <= maxMonth; ++month) {
					auto df = solveLastDiscountFactorForPar(month);
					auto t = (QuantLib::Time)month / 12.;
					auto zr = (std::pow(1.0 / df, 1.0 / t / freq) - 1.) * freq;
					zeroRates[month] = zr / multiplier;
				}
			}
			const auto& zeroRates = *pMonthlyZeroRates;
			if (buildZeroCurve) {
				std::vector<QuantLib::Date> dates(maxMonth + 1);
				std::vector<QuantLib::Rate> yields(maxMonth + 1);
				auto baseReferenceDate = (curveReferenceDate == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : curveReferenceDate);
				for (decltype(maxMonth) month = 0; month <= maxMonth; ++month) {
					dates[month] = baseReferenceDate + month * QuantLib::Months;
					yields[month] = zeroRates[month] * multiplier;
				}
				pZeroCurve.reset(new QuantLib::InterpolatedZeroCurve<QuantLib::Linear>(dates, yields, QuantLib::Actual365Fixed(), QuantLib::Linear(), QuantLib::Compounded, COUPON_FREQ));
			}
		}

		QuantLib::Rate verify(
			std::ostream& os,
			std::streamsize precision = 16
		) const {
			
			QL_REQUIRE(pMonthlySplinedParYields != nullptr && pMonthlyZeroRates != nullptr, "bootstrap zero rates first");
			const auto& parYields = *pMonthlySplinedParYields;
			const auto& zeroRates = *pMonthlyZeroRates;
			ParRateCalculator parRateCalculator(zeroRates);
			auto multiplier = ParRateCalculator::multiplier();
			auto n = parYields.size();
			QL_ASSERT(zeroRates.size() == n, "the number of zero rates (" << zeroRates.size() << ") is not what is expected (" << n << ")");
			os << std::fixed;
			os << std::setprecision(precision);
			QuantLib::Rate err = 0.0;
			for (decltype(n) month = 1; month < n; ++month) {
				auto actual = parYields[month] * multiplier;
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