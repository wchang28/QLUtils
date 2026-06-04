#pragma once

#include <ql/quantlib.hpp>
#include <functional>
#include <cmath>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <ql_utils/yield-termstructure-shocker.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>
#include <ql_utils/utilities/possible-enum-values.hpp>
#include <ql_utils/types.hpp>

namespace QuantLib {
    namespace Utils {
		// "Actual/Actual" like day counter types for the shock primitive calculation for the instantaneous forward rate shock
        // i.e. instantaneous forward ramp shock is applied in the "Actual/Actual" like space
        enum InstFwdShockActActDayCounterType {
            ifsaadct_ActualActual_ISDA = 0,
            ifsaadct_Thirty360_BondBasis = 1,
            ifsaadct_Actual36525 = 2,
			ifsaadct_UseCurveDayCounter = 3,    // use the output curve's day counter for the shock calculation, the output curve's day counter should be as close to "Actual/Actual" as possible
		};
        // possible_enum_values specializatiuon for InstFwdShockActActDayCounterType 
        template <>
        inline const std::set<InstFwdShockActActDayCounterType>& possible_enum_values<InstFwdShockActActDayCounterType>::get() {
            static std::set<InstFwdShockActActDayCounterType> s{
                InstFwdShockActActDayCounterType::ifsaadct_ActualActual_ISDA,
                InstFwdShockActActDayCounterType::ifsaadct_Thirty360_BondBasis,
                InstFwdShockActActDayCounterType::ifsaadct_Actual36525,
                InstFwdShockActActDayCounterType::ifsaadct_UseCurveDayCounter
            };
            return s;
        }

        class InstantaneousFwdYieldTermStructureShocker:
            public YieldCurveShocker<InterpolatedForwardCurve<BackwardFlat>> {
        private:
            typedef YieldCurveShocker<InterpolatedForwardCurve<BackwardFlat>> BaseClass;
        public:
            // input
            InstFwdShockActActDayCounterType shockDayCounterType;
            // output
            DayCounter shockDayCounter;
            Date maxDate;
            std::vector<Date> pillarDates;    // pillar dates
            std::vector<Rate> fwdRates;  // original forward rates (in output curve counter space)
            std::vector<Rate> shockedPillarFwdRates;  // pillar shocked forward rates (in output curve counter space)
            std::vector<Rate> origForwardRates;  // original forward rates (in shock day counter space) my calling the forwardRate() on the original curve
            std::vector<Rate> shockValues;  // shock values
			std::vector<Rate> expectedShockedForwardRates;  // = origForwardRates + shockValues, used for shock verification
            Real totalArea;
            Real totalRampArea;
            Real totalShockedArea;
        public:
            InstantaneousFwdYieldTermStructureShocker(
				InstFwdShockActActDayCounterType shockDayCounterType = InstFwdShockActActDayCounterType::ifsaadct_ActualActual_ISDA
            ) :
                shockDayCounterType(shockDayCounterType),
                totalArea(Null<Real>()),
                totalRampArea(Null<Real>()),
                totalShockedArea(Null<Real>())
            {}
        protected:
            void resetOutputs() override {
                BaseClass::resetOutputs();
                shockDayCounter = DayCounter{};
                maxDate = Date();
                pillarDates.clear();
                fwdRates.clear();
                shockedPillarFwdRates.clear();
				origForwardRates.clear();
				shockValues.clear();
                expectedShockedForwardRates.clear();
                totalArea = Null<Real>();
                totalRampArea = Null<Real>();
                totalShockedArea = Null<Real>();
            }
            void verifyOutputs() const override {
                BaseClass::verifyOutputs();
				QL_ASSERT(shockDayCounter != DayCounter(), "shock day counter is not set");
                QL_ASSERT(maxDate != Date(), "max. date is null");
                QL_ASSERT(shockedCurve->maxDate() == maxDate, "shocked curve's max. date (" << ISODateConv::to_str(shockedCurve->maxDate()) << ") does not match the expected max. date (" << ISODateConv::to_str(maxDate) << ")");
                auto n = pillarDates.size();
                QL_ASSERT(n > 0, "pillar dates array cannot be empty");
                QL_ASSERT(pillarDates.back() == maxDate, "last date in pillar dates array (" << ISODateConv::to_str(pillarDates.back()) << ") does not match the max. date (" << ISODateConv::to_str(maxDate) << ")");
                QL_ASSERT(fwdRates.size() == n, "size of the forward rates array (" << fwdRates.size() << ") is not what's expected (" << n << ")");
                QL_ASSERT(shockedPillarFwdRates.size() == n, "size of the shocked forward rates array (" << shockedPillarFwdRates.size() << ") is not what's expected (" << n << ")");
                QL_ASSERT(origForwardRates.size() == n, "size of the original forward rates array (" << origForwardRates.size() << ") is not what's expected (" << n << ")");
                QL_ASSERT(shockValues.size() == n, "size of the shock values array (" << shockValues.size() << ") is not what's expected (" << n << ")");
				QL_ASSERT(expectedShockedForwardRates.size() == n, "size of the expected shocked forward rates array (" << expectedShockedForwardRates.size() << ") is not what's expected (" << n << ")");
				QL_ASSERT(totalArea != Null<Real>(), "total area is not set");
                QL_ASSERT(totalRampArea != Null<Real>(), "total ramp area is not set");
                QL_ASSERT(totalShockedArea != Null<Real>(), "total shocked area is not set");
            }
        protected:
            DayCounter makeShockDayCounter(
				const DayCounter& curveDayCounter
            ) const {
                switch (shockDayCounterType) {
                    case InstFwdShockActActDayCounterType::ifsaadct_ActualActual_ISDA:
                    default:
						return ActualActual(ActualActual::ISDA);
                    case InstFwdShockActActDayCounterType::ifsaadct_Thirty360_BondBasis:
						return Thirty360(Thirty360::BondBasis);
					case InstFwdShockActActDayCounterType::ifsaadct_Actual36525:
						return Actual36525();
					case InstFwdShockActActDayCounterType::ifsaadct_UseCurveDayCounter:
						return curveDayCounter;
                }
				QL_FAIL("unsupported shock day counter type: " << shockDayCounterType);
            }
            Rate curveInstantaneousForwardRate(
				const YieldTermStructure& curve,
				const Date& dt
            ) const {
                return curve.forwardRate(dt, dt, shockDayCounter, Compounding::Continuous).rate();
            }
            typedef std::function<Real(Time)> ShockPrimitiveCalculator;  // time-based shock primitive calculator for the shock ramp
            typedef std::function<Real(Time)> ShockValueCalculator;  // time-based shock value calculator for the shock ramp
            // the actual shock implementation
            void shockImpl(
                const ShockPrimitiveCalculator& shockPrimitive,
                const ShockValueCalculator& shockValueCalc,
                const DayCounter& curveDayCounter
            ) {
				shockDayCounter = makeShockDayCounter(curveDayCounter);
                auto curveRefDate = this->curveRefDate();
                maxDate = yieldTermStructure->maxDate();
                QL_REQUIRE(maxDate > curveRefDate, "curve max. date (" << ISODateConv::to_str(maxDate) << ") must be greater than the curve ref. date (" << ISODateConv::to_str(curveRefDate) << ")");
                auto dtPrev = curveRefDate;
                auto dt = dtPrev + 1 * Days;
                totalArea = 0.0;
                totalRampArea = 0.0;
                totalShockedArea = 0.0;
                Rate origForwardRate = curveInstantaneousForwardRate(*yieldTermStructure, curveRefDate);
				origForwardRates.push_back(origForwardRate);
                Rate shockValue = shockValueCalc(0.0);
                shockValues.push_back(shockValue);
                Rate expectedShockedForwardRate = origForwardRate + shockValue;
				expectedShockedForwardRates.push_back(expectedShockedForwardRate);
                while (dt <= maxDate) {
                    pillarDates.push_back(dt);
                    auto r_t_prev = std::log(1.0 / yieldTermStructure->discount(dtPrev));   // r(t_prev) * t_prev = Integration(f(tau), 0, dtPrev)
                    auto r_t = std::log(1.0 / yieldTermStructure->discount(dt));    // r(t) * t = Integration(f(tau), 0, dt)
                    auto area = r_t - r_t_prev; // area under the forward rate curve between dtPrev and dt = Integration(f(tau), dtPrev, dt)
                    auto t_prev = shockDayCounter.yearFraction(curveRefDate, dtPrev); // time used to get the primitive from the shock ramp
                    auto t = shockDayCounter.yearFraction(curveRefDate, dt); // time used to get the primitive from the shock ramp
                    auto shock_ramp_area = shockPrimitive(t) - shockPrimitive(t_prev);  // Integration(shock(tau), t_prev, dt)
                    auto shocked_area = area + shock_ramp_area; // Integration(f(tau)+shock(tau), dtPrev, t) = Integration(f_shocked(tau), dtPrev, dt)
                    totalArea += area;
                    totalRampArea += shock_ramp_area;
                    totalShockedArea += shocked_area;

                    Rate origForwardRate = curveInstantaneousForwardRate(*yieldTermStructure, dt);
					origForwardRates.push_back(origForwardRate);
					Rate shockValue = shockValueCalc(t);
                    shockValues.push_back(shockValue);
					Rate expectedShockedForwardRate = origForwardRate + shockValue;
                    expectedShockedForwardRates.push_back(expectedShockedForwardRate);
                    
                    auto delta_t = curveDayCounter.yearFraction(dtPrev, dt);    // time in the unit of curve day counter
                    Rate avg_fwd_rate = area / delta_t;
                    fwdRates.push_back(avg_fwd_rate);
                    Rate avg_fwd_rate_shocked = shocked_area / delta_t;
                    shockedPillarFwdRates.push_back(avg_fwd_rate_shocked);

                    dtPrev = dt;
                    dt += 1 * Days;
                }
				// sanity check the ramp primitive calculation logic
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                {
                    auto T = shockDayCounter.yearFraction(curveRefDate, maxDate);
                    auto a_r = shockPrimitive(T);
                    auto diff = std::abs(a_r - totalRampArea);
                    QL_ASSERT(close_enough(a_r, totalRampArea), "bad/inaccurate primitive calculation logic (diff=" << diff << ") for the shock ramp");
                }
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                pillarDates.insert(pillarDates.begin(), curveRefDate);
                auto f = fwdRates[0];
                fwdRates.insert(fwdRates.begin(), f);
                f = shockedPillarFwdRates[0];
                shockedPillarFwdRates.insert(shockedPillarFwdRates.begin(), f);
                this->shockedCurve.reset(new OutputCurveType(pillarDates, shockedPillarFwdRates, curveDayCounter));
            };
        public:
            template <
				typename SHOCKER    // must implement primitive(x) and value(x) methods, where x is time in the unit of shockDayCounter
            >
            void shock(
                const SHOCKER& shocker,
                const DayCounter& curveDayCounter = Actual365Fixed()
            ) {
                auto shockPrimitiveCalculator = [&shocker](Time t) {
                    return (Real)shocker.primitive(t);
                };
                auto shockValueCalculator = [&shocker](Time t) {
                    return (Real)shocker.value(t);
                };
                this->verifyInputs();
                this->resetOutputs();
                shockImpl(shockPrimitiveCalculator, shockValueCalculator, curveDayCounter);
            }
            Real compoundingAtMaxDate() const {
				QL_ASSERT(totalArea != Null<Real>(), "total area is not set");
                return std::exp(totalArea);
            }
            Real shockCompoundingMultiplierAtMaxDate() const {
                QL_ASSERT(totalRampArea != Null<Real>(), "total ramp area is not set");
                return std::exp(totalRampArea);
            }
            Real shockedCompoundingAtMaxDate() const {
				QL_ASSERT(totalShockedArea != Null<Real>(), "total shocked area is not set");
                return std::exp(totalShockedArea);
            }
            void monthlyRampShock(
                const monthly_ramp& monthlyRamp,
                const DayCounter& curveDayCounter
            ) override {
                shock(monthlyRamp, curveDayCounter);
            }
            Rate verifyShock(
                std::ostream& os,
                std::streamsize precision
            ) const override {
                this->verifyOutputs();
                auto curveRefDate = shockedCurve->referenceDate();
                Rate max_err = Null<Rate>();
                {
					auto years = Size(std::round(shockDayCounter.yearFraction(curveRefDate, maxDate)));
					auto months = years * 12;
                    std::map<Date, Period> monthFilter;
                    for (Size month = 0; month <= months; ++month) {
                        Period tenor(month, Months);
                        auto dt = curveRefDate + tenor;
                        monthFilter[dt] = tenor;
                    }
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(precision);
                    for (Size i = 0; i < pillarDates.size(); ++i) {
						const auto& dt = pillarDates[i];
                        if (monthFilter.find(dt) != monthFilter.end()) {
                            const auto& tenor = monthFilter.at(dt);
                            Rate shockForwardRate = curveInstantaneousForwardRate(*shockedCurve, dt);
                            Rate expectedShockForwardRate = expectedShockedForwardRates[i];
                            auto implied = shockForwardRate;
                            auto actual = expectedShockForwardRate;
                            auto diff = implied - actual;
							auto abs_diff = std::abs(diff);
                            if (max_err == Null<Rate>() || abs_diff > max_err) {
                                max_err = abs_diff;
                            }
                            oss << tenor;
                            oss << "," << ISODateConv::to_str(dt);
                            oss << "," << "actual=" << actual * 100.0;
                            oss << "," << "implied=" << implied * 100.0;
                            oss << "," << "diff=" << (diff * 10000.0) << " bp";
                            oss << std::endl;
                        }
                    }
					os << oss.str();
                }
				// compare continuously compounded zero rates at the max. date between the shocked curve and the expected value calculated from the total shocked area
                {
                    auto curveDayCounter = shockedCurve->dayCounter();
                    auto T = curveDayCounter.yearFraction(curveRefDate, maxDate);   // time in the unit of curve day counter between the curve reference date and the max. date
                    Rate actual = (Rate)(totalShockedArea / T); // expected continuously compounded zero rate on the shocked curve at maxDate
                    Rate implied = shockedCurve->zeroRate(maxDate, curveDayCounter, Compounding::Continuous, Frequency::NoFrequency, false).rate(); // continuously compounded zero rate implied by the shocked curve at maxDate
                    Rate diff = implied - actual;
                    auto abs_diff = std::abs(diff);
                    max_err = (max_err == Null<Rate>() ? abs_diff : std::max(max_err, abs_diff));
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(precision);
                    oss << "maxDate";
                    oss << "," << ISODateConv::to_str(maxDate);
                    oss << "," << "actual=" << actual * 100.0;
                    oss << "," << "implied=" << implied * 100.0;
                    oss << "," << "diff=" << (diff * 10000.0) << " bp";
                    oss << std::endl;
                    os << oss.str();
                }
                return max_err;
            }
        };
    }
}
