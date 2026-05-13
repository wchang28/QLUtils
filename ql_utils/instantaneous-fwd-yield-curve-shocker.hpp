#pragma once

#include <ql/quantlib.hpp>
#include <functional>
#include <cmath>
#include <vector>
#include <sstream>
#include <ql_utils/yield-termstructure-shocker.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>
#include <ql_utils/types.hpp>

namespace QuantLib {
    namespace Utils {
		// "Actual/Actual" like day counter types for the shock primitive calculation for the instantaneous forward rate shock
        // i.e. instantaneous forward ramp shock is applied in the "Actual/Actual" like space
        enum InstFwdShockPrimitiveActActDayCounterType {
            ifspaadct_ActualActual_ISDA = 0,
            ifspaadct_Thirty360_BondBasis = 1,
            ifspaadct_Actual36525 = 2,
			ifspaadct_UseCurveDayCounter = 3,    // use the output curve's day counter for the shock primitive calculation
		};
        // possible_enum_values specializatiuon for InstFwdShockPrimitiveActActDayCounterType 
        template <>
        inline const std::set<InstFwdShockPrimitiveActActDayCounterType>& possible_enum_values<InstFwdShockPrimitiveActActDayCounterType>::get() {
            static std::set<InstFwdShockPrimitiveActActDayCounterType> s{
                InstFwdShockPrimitiveActActDayCounterType::ifspaadct_ActualActual_ISDA,
                InstFwdShockPrimitiveActActDayCounterType::ifspaadct_Thirty360_BondBasis,
                InstFwdShockPrimitiveActActDayCounterType::ifspaadct_Actual36525,
                InstFwdShockPrimitiveActActDayCounterType::ifspaadct_UseCurveDayCounter
            };
            return s;
        }

        class InstantaneousFwdYieldTermStructureShocker:
            public YieldCurveShocker<InterpolatedForwardCurve<BackwardFlat>> {
        private:
            typedef YieldCurveShocker<InterpolatedForwardCurve<BackwardFlat>> BaseClass;
        public:
            // input
            InstFwdShockPrimitiveActActDayCounterType shockPrimitiveDayCounterType;
            // output
            Date maxDate;
            std::vector<Date> dates;    // pillar dates
            std::vector<Rate> fwdRates;  // original forward rates
            std::vector<Rate> shockedFwdRates;  // shocked forward rates
            Real totalArea;
            Real totalRampArea;
            Real totalShockedArea;
        public:
            InstantaneousFwdYieldTermStructureShocker(
				InstFwdShockPrimitiveActActDayCounterType shockPrimitiveDayCounterType = InstFwdShockPrimitiveActActDayCounterType::ifspaadct_ActualActual_ISDA
            ) :
                shockPrimitiveDayCounterType(shockPrimitiveDayCounterType),
                maxDate(Date()),
                totalArea(Null<Real>()),
                totalRampArea(Null<Real>()),
                totalShockedArea(Null<Real>())
            {}
        protected:
            void resetOutputs() override {
                BaseClass::resetOutputs();
                maxDate = Date();
                dates.clear();
                fwdRates.clear();
                shockedFwdRates.clear();
                totalArea = Null<Real>();
                totalRampArea = Null<Real>();
                totalShockedArea = Null<Real>();
            }
            void verifyOutputs() const override {
                BaseClass::verifyOutputs();
                QL_ASSERT(maxDate != Date(), "max. date is null");
                QL_ASSERT(shockedCurve->maxDate() == maxDate, "shocked curve's max. date (" << ISODateConv::to_str(shockedCurve->maxDate()) << ") does not match the expected max. date (" << ISODateConv::to_str(maxDate) << ")");
                auto n = dates.size();
                QL_ASSERT(n > 0, "dates array cannot be empty");
                QL_ASSERT(dates.back() == maxDate, "last date in dates array (" << ISODateConv::to_str(dates.back()) << ") does not match the max. date (" << ISODateConv::to_str(maxDate) << ")");
                QL_ASSERT(fwdRates.size() == n, "size of the forward rates array (" << fwdRates.size() << ") is not what's expected (" << n << ")");
                QL_ASSERT(shockedFwdRates.size() == n, "size of the shocked forward rates array (" << shockedFwdRates.size() << ") is not what's expected (" << n << ")");
				QL_ASSERT(totalArea != Null<Real>(), "total area is not set");
                QL_ASSERT(totalRampArea != Null<Real>(), "total ramp area is not set");
                QL_ASSERT(totalShockedArea != Null<Real>(), "total shocked area is not set");
            }
        protected:
            DayCounter makeShockPrimitiveDayCounter(
				const DayCounter& curveDayCounter
            ) const {
                switch (shockPrimitiveDayCounterType) {
                    case ifspaadct_ActualActual_ISDA:
                    default:
						return ActualActual(ActualActual::ISDA);
                    case ifspaadct_Thirty360_BondBasis:
						return Thirty360(Thirty360::BondBasis);
					case ifspaadct_Actual36525:
						return Actual36525();
					case ifspaadct_UseCurveDayCounter:
						return curveDayCounter;
                }
				QL_FAIL("unsupported shock primitive day counter type: " << shockPrimitiveDayCounterType);
            }
            typedef std::function<Real(Time)> PrimitiveCalculator;  // time-based primitive calculator for the shock ramp
            // the actual shock implementation
            void shockImpl(
                const PrimitiveCalculator& shockPrimitive,
                const DayCounter& curveDayCounter
            ) {
                auto curveRefDate = this->curveRefDate();
                maxDate = yieldTermStructure->maxDate();
                QL_REQUIRE(maxDate > curveRefDate, "curve max. date (" << ISODateConv::to_str(maxDate) << ") must be greater than the curve ref. date (" << ISODateConv::to_str(curveRefDate) << ")");
                DayCounter shockPrimitiveDayCounter = makeShockPrimitiveDayCounter(curveDayCounter);
                auto dtPrev = curveRefDate;
                auto dt = dtPrev + 1 * Days;
                totalArea = 0.0;
                totalRampArea = 0.0;
                totalShockedArea = 0.0;
                while (dt <= maxDate) {
                    dates.push_back(dt);
                    auto r_t_prev = std::log(1.0 / yieldTermStructure->discount(dtPrev));   // r(t_prev) * t_prev = Integration(f(tau), 0, dtPrev)
                    auto r_t = std::log(1.0 / yieldTermStructure->discount(dt));    // r(t) * t = Integration(f(tau), 0, dt)
                    auto area = r_t - r_t_prev; // area under the forward rate curve between dtPrev and dt = Integration(f(tau), dtPrev, dt)
                    auto t_prev = shockPrimitiveDayCounter.yearFraction(curveRefDate, dtPrev); // time used to get the primitive from the shock ramp
                    auto t = shockPrimitiveDayCounter.yearFraction(curveRefDate, dt); // time used to get the primitive from the shock ramp
                    auto shock_ramp_area = shockPrimitive(t) - shockPrimitive(t_prev);  // Integration(shock(tau), t_prev, dt)
                    auto shocked_area = area + shock_ramp_area; // Integration(f(tau)+shock(tau), dtPrev, t) = Integration(f_shocked(tau), dtPrev, dt)
                    totalArea += area;
                    totalRampArea += shock_ramp_area;
                    totalShockedArea += shocked_area;
                    
                    auto delta_t = curveDayCounter.yearFraction(dtPrev, dt);    // time in the unit of curve day counter
                    Rate avg_fwd_rate = area / delta_t;
                    fwdRates.push_back(avg_fwd_rate);
                    Rate avg_fwd_rate_shocked = shocked_area / delta_t;
                    shockedFwdRates.push_back(avg_fwd_rate_shocked);

                    dtPrev = dt;
                    dt += 1 * Days;
                }
				// sanity check the ramp primitive calculation logic
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                auto a_r = shockPrimitive(shockPrimitiveDayCounter.yearFraction(curveRefDate, maxDate));
                auto diff_ramp = std::abs(a_r - totalRampArea);
                QL_ASSERT(close_enough(a_r, totalRampArea), "bad/inaccurate primitive calculation logic (diff_ramp=" << diff_ramp << ") for the shock ramp");
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				dates.insert(dates.begin(), curveRefDate);
                auto f = fwdRates[0];
                fwdRates.insert(fwdRates.begin(), f);
                f = shockedFwdRates[0];
                shockedFwdRates.insert(shockedFwdRates.begin(), f);
                this->shockedCurve.reset(new OutputCurveType(dates, shockedFwdRates, curveDayCounter));
            };
        public:
            template <
                typename SHOCKER
            >
            void shock(
                const SHOCKER& shocker,
                const DayCounter& curveDayCounter = Actual365Fixed()
            ) {
                auto primitiveCalculator = [&shocker](Time t) {
                    return (Real)shocker.primitive(t);
                };
                this->verifyInputs();
                this->resetOutputs();
                shockImpl(primitiveCalculator, curveDayCounter);
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
                auto curveDayCounter = shockedCurve->dayCounter();
				auto T = curveDayCounter.yearFraction(curveRefDate, maxDate);   // time in the unit of curve day counter between the curve reference date and the max. date
                Rate actual = (Rate)(totalShockedArea / T); // expected continuously compounded zero rate on the shocked curve at maxDate
                Rate implied = shockedCurve->zeroRate(maxDate, curveDayCounter, Compounding::Continuous, Frequency::NoFrequency, false).rate(); // continuously compounded zero rate implied by the shocked curve at maxDate
                Rate diff = implied - actual;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(precision);
                oss << "maxDate=" << ISODateConv::to_str(maxDate);
                oss << "," << "actual=" << actual * 100.0;
                oss << "," << "implied=" << implied * 100.0;
                oss << "," << "diff=" << (diff * 10000.0) << " bp";
                oss << std::endl;
                os << oss.str();
                auto error = diff;
                return error;
            }
        };
    }
}
