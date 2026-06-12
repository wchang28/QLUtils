#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/bootstrap.hpp>
#include <ql_utils/interpolation-traits.hpp>
#include <ql_utils/math/strip-interval-end-value-calculator.hpp>
#include <ql_utils/math/interpolations/both-ends-flat-extrapolate-interpolation.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>
#include <ql_utils/types.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>

namespace QuantLib {
    namespace Utils {
        class ICurvesForwardSpreadCalculator {
        public:
            typedef Bootstrapper::pInstruments pInstruments;
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
        public:
            // input
            pInstruments baseInstruments;
            pInstruments targetInstruments;
            // output
            std::vector<Date> spreadDates;
            std::vector<Time> spreadTimes;
            std::vector<Real> spreadAreas;
            std::vector<Spread> spreads;    // forward spreads between the target forward curve and the base forward curve at the spread times
            std::shared_ptr<Interpolation> interp_Spreads;
        protected:
            static std::vector<Date> joinDates(
                const std::vector<Date>& dates_1,
                const std::vector<Date>& dates_2
            ) {
                std::set<Date> allDatesSet;
                for (const auto& d : dates_1) {
                    allDatesSet.insert(d);
                }
                for (const auto& d : dates_2) {
                    allDatesSet.insert(d);
                }
                std::vector<Date> allDates(allDatesSet.begin(), allDatesSet.end());
                std::sort(allDates.begin(), allDates.end(), std::less<Date>());
                return allDates;
            }
        protected:
            // protected interface
            virtual void verifyInputs() const {
                QL_REQUIRE(baseInstruments != nullptr && !baseInstruments->empty(), "base bootstrap instruments cannot be null or empty");
                QL_REQUIRE(targetInstruments != nullptr && !targetInstruments->empty(), "target bootstrap instruments cannot be null or empty");
            }
            virtual void clearOutputs() {
                spreadDates.clear();
                spreadTimes.clear();
                spreadAreas.clear();
                spreads.clear();
                interp_Spreads = nullptr;
            }
            virtual void verifyOutputs() const {
                QL_ASSERT(!spreadDates.empty(), "spread dates are not calculated");
                QL_ASSERT(!spreadTimes.empty(), "spread times are not calculated");
                QL_ASSERT(!spreadAreas.empty(), "spread areas are not calculated");
                QL_ASSERT(!spreads.empty(), "spreads are not calculated");
                QL_ASSERT(interp_Spreads != nullptr, "spread interpolation is not calculated");
            }
        public:
            // public interface
            virtual YieldTermStructurePtr baseForwardTermStructure() const = 0;
            virtual YieldTermStructurePtr targetForwardTermStructure() const = 0;
            virtual YieldTermStructurePtr spreadsOnlyForwardTermStructure() const = 0;
            virtual YieldTermStructurePtr forwardSpreadedTermStructure() const = 0;
            virtual void calculate(
                const Date& curveRefDate,
                const DayCounter& curveDayCounter = Actual365Fixed(),
                bool dualBoootstrapsMode = false // if true, target curve can only be bootstarpped when the base curve is served as an exogenous discount term structure
            ) = 0;
            virtual Spread verify(
                std::ostream& os,
                std::streamsize precision = 16
            ) const = 0;
        };
        typedef std::shared_ptr<ICurvesForwardSpreadCalculator> CurvesForwardSpreadCalculatorPtr;

        template <
            typename Interpolator    // can be BackwardFlat or Linear
        >
        class CurvesForwardSpreadCalculator: public ICurvesForwardSpreadCalculator {
        public:
            typedef YieldCurvesBootstrap<ForwardRate, Interpolator> BootstrapperType;
            typedef std::shared_ptr<BootstrapperType> BootstrapperPtr;
            typedef InterpolatedForwardCurve<Interpolator> InterpolatedForwardCurve;
            typedef InterpolatedPiecewiseForwardSpreadedTermStructure<Interpolator> InterpolatedForwardSpreadedCurve;
            typedef ext::shared_ptr<InterpolatedForwardCurve> InterpolatedForwardCurvePtr;
            typedef ext::shared_ptr<InterpolatedForwardSpreadedCurve> InterpolatedForwardSpreadedCurvePtr;
        public:
            // output
            BootstrapperPtr baseCurveBootstrapper;
            BootstrapperPtr targetCurveBootstrapper;
            InterpolatedForwardCurvePtr baseForwardCurve;
            InterpolatedForwardCurvePtr targetForwardCurve;
            InterpolatedForwardCurvePtr spreadsOnlyForwardCurve;
            InterpolatedForwardSpreadedCurvePtr fwdSpreadedCurve;
        protected:
            // protected interface
            void clearOutputs() override {
                ICurvesForwardSpreadCalculator::clearOutputs();
                baseCurveBootstrapper = nullptr;
                targetCurveBootstrapper = nullptr;
                baseForwardCurve = nullptr;
                targetForwardCurve = nullptr;
                spreadsOnlyForwardCurve = nullptr;
                fwdSpreadedCurve = nullptr;
            }
            void verifyOutputs() const override {
                ICurvesForwardSpreadCalculator::verifyOutputs();
                QL_ASSERT(baseCurveBootstrapper != nullptr, "base forward curve bootstrapper is null");
                QL_ASSERT(targetCurveBootstrapper != nullptr, "target forward curve bootstrapper is null");
                QL_ASSERT(baseForwardCurve != nullptr, "base forward curve is not calculated");
                Date curveRefDate = baseForwardCurve->referenceDate();
				DayCounter curveDayCounter = baseForwardCurve->dayCounter();
                QL_ASSERT(targetForwardCurve != nullptr, "target forward curve is not calculated");
				QL_ASSERT(targetForwardCurve->referenceDate() == curveRefDate, "target forward curve reference date (" << ISODateConv::to_str(targetForwardCurve->referenceDate()) << ") does not match base forward curve reference date (" << ISODateConv::to_str(curveRefDate) << ")");
				QL_ASSERT(targetForwardCurve->dayCounter().name() == curveDayCounter.name(), "target forward curve day counter (" << targetForwardCurve->dayCounter().name() << ") does not match base forward curve day counter (" << curveDayCounter.name() << ")");
                QL_ASSERT(spreadsOnlyForwardCurve != nullptr, "spread-only forward curve is not calculated");
				QL_ASSERT(spreadsOnlyForwardCurve->referenceDate() == curveRefDate, "spread-only forward curve reference date (" << ISODateConv::to_str(spreadsOnlyForwardCurve->referenceDate()) << ") does not match base forward curve reference date (" << ISODateConv::to_str(curveRefDate) << ")");
                QL_ASSERT(spreadsOnlyForwardCurve->dayCounter().name() == curveDayCounter.name(), "spread-only forward curve day counter (" << spreadsOnlyForwardCurve->dayCounter().name() << ") does not match base forward curve day counter (" << curveDayCounter.name() << ")");
                QL_ASSERT(fwdSpreadedCurve != nullptr, "forward spreaded curve is not calculated");
				QL_ASSERT(fwdSpreadedCurve->referenceDate() == curveRefDate, "forward spreaded curve reference date (" << ISODateConv::to_str(fwdSpreadedCurve->referenceDate()) << ") does not match base forward curve reference date (" << ISODateConv::to_str(curveRefDate) << ")");
				QL_ASSERT(fwdSpreadedCurve->dayCounter().name() == curveDayCounter.name(), "forward spreaded curve day counter (" << fwdSpreadedCurve->dayCounter().name() << ") does not match base forward curve day counter (" << curveDayCounter.name() << ")");
            }
        public:
            // public interface
            YieldTermStructurePtr baseForwardTermStructure() const override {
                return baseForwardCurve;
            }
            YieldTermStructurePtr targetForwardTermStructure() const override {
                return targetForwardCurve;
            }
            YieldTermStructurePtr spreadsOnlyForwardTermStructure() const override {
                return spreadsOnlyForwardCurve;
            }
            YieldTermStructurePtr forwardSpreadedTermStructure() const override {
                return fwdSpreadedCurve;
            }
            void calculate(
                const Date& curveRefDate,
                const DayCounter& curveDayCounter,
                bool dualBoootstrapsMode
            ) override {
                verifyInputs();
                clearOutputs();
                // bootstrap the base forward curve
                //////////////////////////////////////////////////////////////////////////
                baseCurveBootstrapper.reset(new BootstrapperType());
                baseCurveBootstrapper->exogenousDiscountTermStructure = nullptr;
                baseCurveBootstrapper->instruments = baseInstruments;
                baseCurveBootstrapper->bootstrap(curveRefDate, curveDayCounter);
                baseForwardCurve = baseCurveBootstrapper->estimatingCurve;
                baseForwardCurve->enableExtrapolation(true);
                //////////////////////////////////////////////////////////////////////////
                // bootstrap the target forward curve
                //////////////////////////////////////////////////////////////////////////
                targetCurveBootstrapper.reset(new BootstrapperType());
				targetCurveBootstrapper->exogenousDiscountTermStructure = (dualBoootstrapsMode ? baseForwardCurve : nullptr);
                targetCurveBootstrapper->instruments = targetInstruments;
                targetCurveBootstrapper->bootstrap(curveRefDate, curveDayCounter);
                targetForwardCurve = targetCurveBootstrapper->estimatingCurve;
                targetForwardCurve->enableExtrapolation(true);
                //////////////////////////////////////////////////////////////////////////
                // union/join the pillar dates of the two curves
                //////////////////////////////////////////////////////////////////////////////////
                spreadDates = joinDates(baseForwardCurve->dates(), targetForwardCurve->dates());
                for (const auto& d : spreadDates) {
                    spreadTimes.push_back(curveDayCounter.yearFraction(curveRefDate, d));
                }
                QL_ASSERT(spreadTimes[0] == 0.0, "The first spread time must be 0.0");
                TimeGrid spreadTimeGrid(spreadTimes.begin(), spreadTimes.end());
                //////////////////////////////////////////////////////////////////////////////////
                BothEndsFlatExtrapolateInterpolation<Interpolator> interp_Base(
                    baseForwardCurve->times().begin(),
                    baseForwardCurve->times().end(),
                    baseForwardCurve->data().begin()
                );
                BothEndsFlatExtrapolateInterpolation<Interpolator> interp_Target(
                    targetForwardCurve->times().begin(),
                    targetForwardCurve->times().end(),
                    targetForwardCurve->data().begin()
                );
                // calculate the strip interval areas between the the two curves
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                spreadAreas.resize(spreadTimeGrid.size() - 1);
                for (Size i = 1; i < spreadTimeGrid.size(); ++i) {
                    auto t_prev = spreadTimeGrid[i - 1];
                    auto t = spreadTimeGrid[i];
                    auto area_Target = interp_Target.primitive(t, true) - interp_Target.primitive(t_prev, true);
                    auto area_Base = interp_Base.primitive(t, true) - interp_Base.primitive(t_prev, true);
                    auto area_spread = area_Target - area_Base;
                    spreadAreas[i - 1] = area_spread;
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                // calculate the actual spreads between the the two curves
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                Spread spotSpread = interp_Target(0.0, true) - interp_Base(0.0, true);    // spread @ t = 0.0
                spreads.resize(spreadTimeGrid.size());
                spreads[0] = spotSpread;
                StripIntervalEndValueCalculator<Interpolator, Time, Spread> endSpreadCalculator;
                for (Size i = 1; i < spreadTimeGrid.size(); ++i) {    // for each strip interval, calculate the spread at the end of the interval based on the spread at the beginning of the interval and the area of the strip interval
                    const auto& area = spreadAreas[i - 1];
                    auto dt = spreadTimeGrid.dt(i - 1);
                    spreads[i] = endSpreadCalculator(spreads[i - 1], area, dt);
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////
				// create an interpolation of the spreads at the spread times
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                interp_Spreads.reset(
                    new Interpolation(
                        Interpolator{}.interpolate(
                            spreadTimeGrid.begin(),
                            spreadTimeGrid.end(),
                            spreads.begin()
                        )
                    )
                );
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                // create a spread-only interpolated forward curve
				// this curve can be used to serialize/deserialize the calculated spreads
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                spreadsOnlyForwardCurve = ext::make_shared<InterpolatedForwardCurve>(spreadDates, spreads, curveDayCounter);
                spreadsOnlyForwardCurve->enableExtrapolation(true);
                ////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
                // sanity check: verify R_Target(T) * T - R_Base(T) * T where R(t) is the continuously compounded zero rate at time t and T is the last spread time
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                {
                    Time T = spreadTimeGrid.back();    // last spread time
                    Date lastSpreadDate = spreadDates.back();    // last spread date

                    Rate R_Target = targetForwardCurve->zeroRate(lastSpreadDate, curveDayCounter, Continuous, NoFrequency, true).rate();    // R_Target(T)
                    Rate R_Base = baseForwardCurve->zeroRate(lastSpreadDate, curveDayCounter, Continuous, NoFrequency, true).rate();    // R_Base(T)
					Rate R_Spread_1 = R_Target - R_Base;    // R_Spread(T)
                    Real area_Target = R_Target * T;    // R_Target(T) * T
                    Real area_Base = R_Base * T;    // R_Base(T) * T
                    Real primitive_spread_1 = area_Target - area_Base;

                    Real primitive_Target = interp_Target.primitive(T, true);
                    Real primitive_Base = interp_Base.primitive(T, true);
                    Real primitive_spread_2 = primitive_Target - primitive_Base;

                    Real primitive_spread_3 = interp_Spreads->primitive(T, true);

                    Rate R_Spread_2 = spreadsOnlyForwardCurve->zeroRate(lastSpreadDate, curveDayCounter, Continuous, NoFrequency, true).rate();
                    Real primitive_spread_4 = R_Spread_2 * T;

                    QL_ASSERT(close_enough(area_Target, primitive_Target), "primitive_Target (" << primitive_Target << ") is not close to area_Target (" << area_Target << ")");
                    QL_ASSERT(close_enough(area_Base, primitive_Base), "primitive_Base (" << primitive_Base << ") is not close to area_Base (" << area_Base << ")");
                    QL_ASSERT(close_enough(R_Spread_1, R_Spread_2), "R_Spread_1 (" << (R_Spread_1*10000.0) << " bp) is not close to R_Spread_2 (" << (R_Spread_2 * 10000.0) << " bp)");
                    QL_ASSERT(close_enough(primitive_spread_1, primitive_spread_2), "primitive_spread_2 (" << primitive_spread_2 << ") is not close to primitive_spread_1 (" << primitive_spread_1 << ")");
                    QL_ASSERT(close_enough(primitive_spread_1, primitive_spread_3), "primitive_spread_3 (" << primitive_spread_3 << ") is not close to primitive_spread_1 (" << primitive_spread_1 << ")");
                    QL_ASSERT(close_enough(primitive_spread_1, primitive_spread_4), "primitive_spread_4 (" << primitive_spread_4 << ") is not close to primitive_spread_1 (" << primitive_spread_1 << ")");
                }
#endif
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // construct a forward spreaded term structure with the spreeads and the base forward curve
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                Handle<YieldTermStructure> baseCurve(baseForwardCurve);
                std::vector<Handle<Quote>> spreadQuotes;
                for (Size i = 0; i < spreadDates.size(); ++i) {
                    const auto& spread = spreads[i];
                    spreadQuotes.push_back(Handle<Quote>(ext::make_shared<SimpleQuote>(spread)));
                }
                fwdSpreadedCurve = ext::make_shared<InterpolatedForwardSpreadedCurve>(
                    baseCurve,
                    spreadQuotes,
                    spreadDates
                );
                fwdSpreadedCurve->enableExtrapolation(true);
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            }
        protected:
            // compare forward spreaded curve and target forward curve
            // compare their zero rates on the spread dates
            Spread verifyForwardSpreadedCurveImpl(
                std::ostream& os,
                std::streamsize precision = 16
            ) const {
                DayCounter dc = fwdSpreadedCurve->dayCounter();
                Spread err = 0.0;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(precision);
                for (Size i = 0; i < spreadDates.size(); ++i) {
                    const auto& date = spreadDates[i];
                    Rate forwardSpreadedZeroRate = fwdSpreadedCurve->zeroRate(date, dc, Continuous, NoFrequency, true).rate();
                    Rate expectedZeroRate = targetForwardCurve->zeroRate(date, dc, Continuous, NoFrequency, true).rate();
                    Spread diff = forwardSpreadedZeroRate - expectedZeroRate;
                    oss << i;
                    oss << ": " << "[" << ISODateConv::to_str(date) << "]";
                    oss << "," << "fwdSpreadedZeroRate=" << forwardSpreadedZeroRate * 100.0 << "%";
                    oss << "," << "expectedZeroRate=" << expectedZeroRate * 100.0 << "%";
                    oss << "," << "diff=" << diff * 10000.0 << " bp";
                    oss << std::endl;
                    err += std::pow(diff, 2.0);
                }
                os << oss.str();
                return std::sqrt(err);
            }
        public:
            Spread verify(
                std::ostream& os,
                std::streamsize precision = 16
            ) const override {
                verifyInputs();
                verifyOutputs();
                Spread errorTotal = 0.0;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(precision);

                oss << "Verifying base forward curve..." << std::endl;
                Spread err = baseCurveBootstrapper->verifyBootstrap(oss, precision);
                oss << "err=" << err * 10000.0 << " bp" << std::endl;
                errorTotal += err;

                oss << std::endl;
                oss << "Verifying target forward curve..." << std::endl;
                err = targetCurveBootstrapper->verifyBootstrap(oss, precision);
                oss << "err=" << err * 10000.0 << " bp" << std::endl;
                errorTotal += err;
                
                oss << std::endl;
                oss << "Verifying forward spreaded curve..." << std::endl;
                err = verifyForwardSpreadedCurveImpl(oss, precision);
                oss << "err=" << err * 10000.0 << " bp" << std::endl;
                errorTotal += err;

                os << oss.str();
                return errorTotal;
            }
        };
#define HANDLE_FWD_SPREAD_INTERP_MAKE_CALCULATOR(INTERP) case ForwardSpreadInterpolation::INTERP: {\
        using InterpTraits = ForwardSpreadInterpTraits<ForwardSpreadInterpolation::INTERP>;   \
        return CurvesForwardSpreadCalculatorPtr(new CurvesForwardSpreadCalculator<typename InterpTraits::InterpType>()); \
    }
        inline CurvesForwardSpreadCalculatorPtr make_curves_forward_spreads_calculator(
            ForwardSpreadInterpolation interpolation
        ) {
            switch(interpolation) {
            HANDLE_FWD_SPREAD_INTERP_MAKE_CALCULATOR(fsiStep)
            HANDLE_FWD_SPREAD_INTERP_MAKE_CALCULATOR(fsiLinear)
            default:
                QL_FAIL("unsupported forward spread interpolation: " << interpolation);
            }
        }
    }
#undef HANDLE_FWD_SPREAD_INTERP_MAKE_CALCULATOR
}
