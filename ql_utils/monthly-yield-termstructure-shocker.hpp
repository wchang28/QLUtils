#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/yield-termstructure-shocker.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/ParYield.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/bootstrap.hpp>
#include <ql_utils/dateformat.hpp>
#include <ql_utils/ratehelpers/nominal_forward_ratehelper.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <cmath>
#include <functional>

namespace QuantLib {
    namespace Utils {
        template <
            typename Traits = ZeroYield,   // ZeroYield, Discount, ForwardRate, or SimpleZeroYield
            typename Interpolator = Linear  // Linear, BackwardFlat, ConvexMonotone
        >
        struct MonthlyYieldCurveShockerTraits {
        public:
            typedef typename Traits::template curve<Interpolator>::type BaseCurveType;	// InterpolatedZeroCurve<Interpolator>, InterpolatedDiscountCurve<Interpolator>, InterpolatedForwardCurve<Interpolator>, or InterpolatedSimpleZeroCurve<Interpolator>
        };

        // base class for all monthly yield curve shockers that requires a final bootstrap to get the shocked curve
        template <
            typename Traits = ZeroYield,   // ZeroYield, Discount, ForwardRate, or SimpleZeroYield
            typename I = Linear  // Linear, BackwardFlat, ConvexMonotone
        >
        class MonthlyYieldTermStructureShocker:
            public YieldCurveShocker<typename MonthlyYieldCurveShockerTraits<Traits, I>::BaseCurveType>,
            public Bootstrapper {
        public:
            typedef I Interp;
            typedef typename MonthlyYieldCurveShockerTraits<Traits, I>::BaseCurveType OutputCurveType;
            typedef YieldCurvesBootstrap<Traits, I> BootstrapperType;   // the final bootstrapper type for the shocked curve
            typedef Natural MonthNumber;
            typedef Ramp<Frequency::Monthly> monthly_ramp;
        protected:
            typedef std::function<Rate(MonthNumber)> MonthlyRateShocker;
        private:
            typedef YieldCurveShocker<OutputCurveType> BaseClass;
        public:
            // output
            std::vector<Period> monthlyMaturities;   // monthly maturities (can be tenors or forwards periods)
            std::vector<Rate> monthlyBaseRates;  // original monthly rates
            std::vector<Rate> monthlyShocks;  // monthly shock amount
            pInstruments shockedQuotes;  // shocked instruments
        protected:
            void resetOutputs() override {
                BaseClass::resetOutputs();
                monthlyMaturities.clear();
                monthlyBaseRates.clear();
                monthlyShocks.clear();
                shockedQuotes.reset(new Instruments());
            }
            void verifyOutputs() const override {
                BaseClass::verifyOutputs();
                QL_ASSERT(shockedQuotes != nullptr, "shocked quotes cannot be null");
                auto n = shockedQuotes->size();
                QL_ASSERT(n > 0, "shocked quotes is empty");
                QL_ASSERT(monthlyMaturities.size() == n, "bad monthly maturity vector");
                QL_ASSERT(monthlyBaseRates.size() == n, "bad monthly base rate vector");
                QL_ASSERT(monthlyShocks.size() == n, "bad monthly shock vector");
            }
        private:
            // bootstrap shocked yield curve with shocked quotes
            void bootstrapShockedCurve(
                const DayCounter& dayCounter,
                const I& interp
            ) {
                auto curveRefDate = this->curveRefDate();
                BootstrapperType bootstrap;
                bootstrap.instruments = shockedQuotes;
                bootstrap.bootstrap(curveRefDate, dayCounter, interp);
                this->shockedCurve = bootstrap.discountCurve;
            }
        protected:
            // the actual shock implementation
            virtual void shockImpl(
                const MonthlyRateShocker& monthlyRateShocker
            ) = 0;
            // implied rate calculation
            virtual Rate impliedRate(
                const pInstrument& pInst,
                const YieldTermStructureHandle& shockedTS
            ) const = 0;
        public:
            template <
                typename MONTHLY_SHOCKER
            >
            void shock(
                const MONTHLY_SHOCKER& monthlyShocker,
                const DayCounter& dayCounter = Actual365Fixed(),
                const I& interp = I()   // custom interpretor of type I
            ) {
                auto monthlyRateShocker = [&monthlyShocker](MonthNumber month) {
                    return (Rate)monthlyShocker(month);
                };
                this->verifyInputs();
                this->resetOutputs();
                shockImpl(monthlyRateShocker);
                bootstrapShockedCurve(dayCounter, interp);
            }
            template<
                typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison
            >
            Rate verify(
                std::ostream& os,
                std::streamsize precision = 16,
                const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
            ) const {
                this->verifyOutputs();
                YieldTermStructureHandle shockedTS(this->shockedCurve);
                const auto& me = *this;
                return verifyImpl(
                    *shockedQuotes,
                    [&shockedTS, &me](const pInstrument& pInst) -> Rate {
                        return me.impliedRate(pInst, shockedTS);
                    },
                    os,
                    precision,
                    compare
                );
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
                return verify(os, precision);
            } 
        };

        // monthly spot par yield shocker
        template <
            typename Traits = ZeroYield,   // ZeroYield, Discount, ForwardRate, or SimpleZeroYield
            typename I = Linear,  // Linear, BackwardFlat, ConvexMonotone
            Frequency PAR_YIELD_COUPON_FREQ = Frequency::Semiannual,
            Thirty360::Convention THIRTY_360_DC_CONVENTION = Thirty360::BondBasis
        >
        class ParShockYieldTermStructure: public MonthlyYieldTermStructureShocker<Traits, I> {
        private:
            typedef MonthlyYieldTermStructureShocker<Traits, I> BaseClass;
        protected:
            typedef typename BaseClass::MonthlyRateShocker MonthlyRateShocker;
            typedef typename BaseClass::MonthNumber MonthNumber;
            typedef typename BaseClass::pInstrument pInstrument;
            typedef typename BaseClass::YieldTermStructureHandle YieldTermStructureHandle;
            typedef QLUtils::ParRate<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION> InstrumentUsed;
            typedef QLUtils::ParYieldHelper<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION> ParYieldHelperType;
        protected:
            void shockImpl(
                const MonthlyRateShocker& monthlyRateShocker
            ) override {
                auto curveReferenceDate = this->yieldTermStructure->referenceDate();
                auto maxDate = this->yieldTermStructure->maxDate();
                MonthNumber tenorMonth = 1; // starting with 1MO par rate
                while (true) {
                    Period tenor(tenorMonth, Months);
                    auto parYield = ParYieldHelperType::parYield(this->yieldTermStructure, tenor); // calculate the original spot par yield for the tenor
                    auto shock = monthlyRateShocker(tenorMonth);   // get the amount of shock from the rate shocker
                    auto shockedParYield = parYield + shock; // add the shock to the par yield
                    pInstrument pInst(new InstrumentUsed(tenor, curveReferenceDate));
                    pInst->rate() = shockedParYield;
                    std::ostringstream oss;
                    oss << "PAR-" << tenor.length() << "M";
                    pInst->ticker() = oss.str();
                    if (pInst->maturityDate() > maxDate) {
                        break;
                    }
                    this->monthlyMaturities.push_back(tenor);
                    this->monthlyBaseRates.push_back(parYield);
                    this->monthlyShocks.push_back(shock);
                    this->shockedQuotes->push_back(pInst);
                    tenorMonth++;
                };
            }
            Rate impliedRate(
                const pInstrument& pInst,
                const YieldTermStructureHandle& discountingTermStructure
            ) const override {
                auto pParInstrument = std::dynamic_pointer_cast<QLUtils::ParRateInstrument>(pInst);
                return pParInstrument->impliedParRate(discountingTermStructure);
            }
        };

        // monthly FRA/simple forward rate based shocker
        template <
            typename Traits = ZeroYield,   // ZeroYield, Discount, ForwardRate, or SimpleZeroYield
            typename I = Linear  // Linear, BackwardFlat, ConvexMonotone
        >
        class SimpleForwardTermStructureShocker: public MonthlyYieldTermStructureShocker<Traits, I> {
        private:
            typedef MonthlyYieldTermStructureShocker<Traits, I> BaseClass;
        protected:
            typedef typename BaseClass::MonthlyRateShocker MonthlyRateShocker;
            typedef typename BaseClass::MonthNumber MonthNumber;
            typedef typename BaseClass::pInstrument pInstrument;
            typedef typename BaseClass::YieldTermStructureHandle YieldTermStructureHandle;
            typedef QLUtils::FRA InstrumentUsed;
        public:
            // input
            QLUtils::IborIndexFactory iborIndexFactory;
        protected:
            void verifyInputs() const override {
                BaseClass::verifyInputs();
                QL_REQUIRE(iborIndexFactory != nullptr, "ibor index factory cannot be null");
            }
            void shockImpl(
                const MonthlyRateShocker& monthlyRateShocker
            ) override {
                auto curveReferenceDate = this->yieldTermStructure->referenceDate();
                Date today = Settings::instance().evaluationDate();
                QL_REQUIRE(curveReferenceDate == today, "curve's reference date (" << curveReferenceDate << ") is not equal to today's date (" << today << ")");
                auto maxDate = this->yieldTermStructure->maxDate();
                MonthNumber fwdMonth = 0;
                while (true) {
                    Period forward(fwdMonth, Months);
                    std::shared_ptr<InstrumentUsed> pInst(new InstrumentUsed(iborIndexFactory, forward));
                    if (pInst->maturityDate() > maxDate) {
                        break;
                    }
                    auto fwdRate = pInst->impliedRate(this->yieldTermStructure); // calculate the original forward rate for the forward period
                    auto shock = monthlyRateShocker(fwdMonth);   // get the amount of shock from the rate shocker
                    auto shockedFwdRate = fwdRate + shock; // add the shock to the forward rate
                    pInst->rate() = shockedFwdRate;
                    std::ostringstream oss;
                    oss << "FWD-" << forward.length() << "M";
                    pInst->ticker() = oss.str();
                    this->monthlyMaturities.push_back(forward);
                    this->monthlyBaseRates.push_back(fwdRate);
                    this->monthlyShocks.push_back(shock);
                    this->shockedQuotes->push_back(pInst);
                    fwdMonth++;
                };
            }
            Rate impliedRate(
                const pInstrument& pInst,
                const YieldTermStructureHandle& estimatingTermStructure
            ) const override {
                auto pInstrument = std::dynamic_pointer_cast<InstrumentUsed>(pInst);
                return pInstrument->impliedRate(estimatingTermStructure.currentLink());
            }
        };

        // monthly nominal forward rate based shocker
        template <
            typename Traits = ZeroYield,   // ZeroYield, Discount, ForwardRate, or SimpleZeroYield
            typename I = Linear,  // Linear, BackwardFlat, ConvexMonotone
            Integer TENOR_MONTHS = 1,
            Thirty360::Convention THIRTY_360_DC_CONVENTION = Thirty360::BondBasis,
            Compounding COMPOUNDING = Compounding::Continuous,
            Frequency FREQUENCY = Frequency::NoFrequency
        >
        class NominalForwardShockYieldTermStructure : public MonthlyYieldTermStructureShocker<Traits, I> {
        private:
            typedef MonthlyYieldTermStructureShocker<Traits, I> BaseClass;
        protected:
            typedef typename BaseClass::MonthlyRateShocker MonthlyRateShocker;
            typedef typename BaseClass::MonthNumber MonthNumber;
            typedef typename BaseClass::pInstrument pInstrument;
            typedef typename BaseClass::YieldTermStructureHandle YieldTermStructureHandle;
            typedef QLUtils::NominalForwardRate<TENOR_MONTHS, THIRTY_360_DC_CONVENTION, COMPOUNDING, FREQUENCY> InstrumentUsed;
        protected:
            void shockImpl(
                const MonthlyRateShocker& monthlyRateShocker
            ) override {
                auto curveReferenceDate = this->yieldTermStructure->referenceDate();
                auto maxDate = this->yieldTermStructure->maxDate();
                auto tenor = Period(TENOR_MONTHS, Months);
                MonthNumber forwardMonth = 0;
                Period forward(forwardMonth, Months);
                auto maturityDate = curveReferenceDate + forward + tenor;
                while (maturityDate <= maxDate) {
                    Period forward(forwardMonth, Months);
                    auto rate = NominalForwardRateHelper::impliedRate(
                        *(this->yieldTermStructure),
                        forward,
                        tenor,
                        Thirty360(THIRTY_360_DC_CONVENTION),
                        COMPOUNDING,
                        FREQUENCY
                    );
                    auto shock = monthlyRateShocker(forwardMonth);   // get the amount of shock from the rate shocker
                    auto shockedRate = rate + shock; // add the shock to the rate
                    pInstrument pInst(new InstrumentUsed(forward, curveReferenceDate));
                    pInst->rate() = shockedRate;
                    std::ostringstream oss;
                    oss << "FWD-" << forward.length() << "Mx" << tenor.length() << "M";
                    pInst->ticker() = oss.str();
                    this->monthlyMaturities.push_back(forward);
                    this->monthlyBaseRates.push_back(rate);
                    this->monthlyShocks.push_back(shock);
                    this->shockedQuotes->push_back(pInst);
                    forwardMonth++;
                    maturityDate = curveReferenceDate + Period(forwardMonth, Months) + tenor;
                };
            }
            Rate impliedRate(
                const pInstrument& pInst,
                const YieldTermStructureHandle& discountingTermStructure
            ) const override {
                auto pInstrument = std::dynamic_pointer_cast<InstrumentUsed>(pInst);
                return pInstrument->impliedRate(discountingTermStructure);
            }
        };
        
        template <
            Frequency PAR_YIELD_COUPON_FREQ = Frequency::Semiannual,
            Thirty360::Convention THIRTY_360_DC_CONVENTION = Thirty360::BondBasis
        >
        inline YieldTermStructureShockerPtr make_yield_curve_par_rate_shocker(
            YieldTermStructureInterpolation interpolation
        ) {
            switch(interpolation) {
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearCont:
                return YieldTermStructureShockerPtr(
                    new ParShockYieldTermStructure<
                        ZeroYield,
                        Linear,
                        PAR_YIELD_COUPON_FREQ,
                        THIRTY_360_DC_CONVENTION
                    >()
                );
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearSimple:
                return YieldTermStructureShockerPtr(
                    new ParShockYieldTermStructure<
                        SimpleZeroYield,
                        Linear,
                        PAR_YIELD_COUPON_FREQ,
                        THIRTY_360_DC_CONVENTION
                    >()
                );
            case YieldTermStructureInterpolation::ytsiStepForwardCont:
                return YieldTermStructureShockerPtr(
                    new ParShockYieldTermStructure<
                        ForwardRate,
                        BackwardFlat,
                        PAR_YIELD_COUPON_FREQ,
                        THIRTY_360_DC_CONVENTION
                    >()
                );
            case YieldTermStructureInterpolation::ytsiSmoothForwardCont:
                return YieldTermStructureShockerPtr(
                    new ParShockYieldTermStructure<
                        ForwardRate,
                        ConvexMonotone,
                        PAR_YIELD_COUPON_FREQ,
                        THIRTY_360_DC_CONVENTION
                    >()
                );
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearForwardCont:
                return YieldTermStructureShockerPtr(
                    new ParShockYieldTermStructure<
                        ForwardRate,
                        Linear,
                        PAR_YIELD_COUPON_FREQ,
                        THIRTY_360_DC_CONVENTION
                    >()
                );
            default:
                QL_FAIL("unsupported yield term structure interpolation for curve shock: " << interpolation);
            }
        }
    }
}
