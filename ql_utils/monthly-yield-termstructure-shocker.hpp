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

namespace QLUtils {
    template <
        typename Traits = QuantLib::ZeroYield,   // QuantLib::ZeroYield, QuantLib::Discount, QuantLib::ForwardRate, or QuantLib::SimpleZeroYield
        typename Interpolator = QuantLib::Linear
    >
    struct MonthlyYieldCurveShockerTraits {
    public:
        typedef typename Traits::template curve<Interpolator>::type BaseCurveType;	// InterpolatedZeroCurve<Interpolator>, InterpolatedDiscountCurve<Interpolator>, InterpolatedForwardCurve<Interpolator>, or InterpolatedSimpleZeroCurve<Interpolator>
    };
    template <
        typename I = QuantLib::Linear
    >
    class MonthlyYieldTermStructureShocker:
        public QuantLib::Utils::YieldCurveShocker<typename MonthlyYieldCurveShockerTraits<QuantLib::ZeroYield, I>::BaseCurveType>,
        public Bootstrapper {
    public:
        typedef I Interp;
        typedef typename MonthlyYieldCurveShockerTraits<QuantLib::ZeroYield, I>::BaseCurveType OutputCurveType;
        typedef QuantLib::Natural MonthNumber;
        typedef QuantLib::Utils::Ramp<QuantLib::Frequency::Monthly> monthly_ramp;
    protected:
        typedef std::function<QuantLib::Rate(MonthNumber)> MonthlyRateShocker;
    private:
        typedef QuantLib::Utils::YieldCurveShocker<OutputCurveType> BaseClass;
    public:
        struct DefaultActualVsImpliedComparison {
            QuantLib::Rate operator() (
                std::ostream& os,
                const std::shared_ptr<BootstrapInstrument>& inst,
                const QuantLib::Real& actual,
                const QuantLib::Real& implied
            ) const {
                using DtFormat = DateFormat<char>;
                auto startDate = inst->startDate();
                auto endDate = inst->maturityDate();
                auto diff = implied - actual;
                os << inst->tenor();
                os << "," << inst->ticker();
                os << "," << "[" << DtFormat::to_yyyymmdd(startDate, true) << "," << DtFormat::to_yyyymmdd(endDate, true) << ")";
                os << "," << "actual=" << actual * inst->valueMultiplier();
                os << "," << "implied=" << implied * inst->valueMultiplier();
                os << "," << "diff=" << diff * inst->basisPointDiffMultiplier() << " bp";
                os << std::endl;
                return diff * inst->absoluteDiffMultiplier();
            }
        };
    public:
        // output
        std::vector<QuantLib::Period> monthlyMaturities;   // monthly maturities (can be tenors or forwards periods)
        std::vector<QuantLib::Rate> monthlyBaseRates;  // original monthly rates
        std::vector<QuantLib::Rate> monthlyShocks;  // monthly shock amount
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
        // bootstrap shocked zero curve with shocked quotes
        void bootstrapShockedZeroCurve(
            const QuantLib::DayCounter& dayCounter,
            const I& interp
        ) {
            auto curveRefDate = this->curveRefDate();
            ZeroCurvesBootstrap<I> bootstrap;
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
        virtual QuantLib::Rate impliedRate(
            const pInstrument& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& shockedTS
        ) const = 0;
    public:
        template <
            typename MONTHLY_SHOCKER
        >
        void shock(
            const MONTHLY_SHOCKER& monthlyShocker,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const I& interp = I()   // custom interpretor of type I
        ) {
            auto monthlyRateShocker = [&monthlyShocker](MonthNumber month) {
                return (QuantLib::Rate)monthlyShocker(month);
            };
            this->verifyInputs();
            this->resetOutputs();
            shockImpl(monthlyRateShocker);
            bootstrapShockedZeroCurve(dayCounter, interp);
        }
        template<
            typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison
        >
        QuantLib::Rate verify(
            std::ostream& os,
            std::streamsize precision = 16,
            const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
        ) const {
            this->verifyOutputs();
            QuantLib::Handle<QuantLib::YieldTermStructure> shockedTS(this->shockedCurve);
            const auto& me = *this;
            return verifyImpl(
                shockedQuotes,
                [&shockedTS, &me](const pInstrument& pInst) -> QuantLib::Rate {
                    return me.impliedRate(pInst, shockedTS);
                },
                os,
                precision,
                compare
            );
        }
        void monthlyRampShock(
            const monthly_ramp& monthlyRamp,
            const QuantLib::DayCounter& curveDayCounter
        ) override {
            shock(monthlyRamp, curveDayCounter);
        }
        QuantLib::Rate verifyShock(
            std::ostream& os,
            std::streamsize precision
        ) const override {
            return verify(os, precision);
        } 
    };

    // spot par yield shocker
    template <
        typename I = QuantLib::Linear,
        QuantLib::Frequency PAR_YIELD_COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::BondBasis
    >
    class ParShockYieldTermStructure: public MonthlyYieldTermStructureShocker<I> {
    private:
        typedef MonthlyYieldTermStructureShocker<I> BaseClass;
    protected:
        typedef typename BaseClass::MonthlyRateShocker MonthlyRateShocker;
        typedef typename BaseClass::MonthNumber MonthNumber;
        typedef typename BaseClass::pInstrument pInstrument;
        typedef ParRate<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION> InstrumentUsed;
		typedef ParYieldHelper<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION> ParYieldHelperType;
    protected:
        void shockImpl(
            const MonthlyRateShocker& monthlyRateShocker
        ) override {
            auto curveReferenceDate = this->yieldTermStructure->referenceDate();
            auto maxDate = this->yieldTermStructure->maxDate();
            MonthNumber tenorMonth = 1; // starting with 1MO par rate
            while (true) {
                QuantLib::Period tenor(tenorMonth, QuantLib::Months);
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
        QuantLib::Rate impliedRate(
            const pInstrument& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const override {
            auto pParInstrument = std::dynamic_pointer_cast<ParRateInstrument>(pInst);
            return pParInstrument->impliedParRate(discountingTermStructure);
        }
    };

    template <
        typename I = QuantLib::Linear
    >
    class SimpleForwardTermStructureShocker: public MonthlyYieldTermStructureShocker<I> {
    private:
        typedef MonthlyYieldTermStructureShocker<I> BaseClass;
    protected:
        typedef typename BaseClass::MonthlyRateShocker MonthlyRateShocker;
        typedef typename BaseClass::MonthNumber MonthNumber;
        typedef typename BaseClass::pInstrument pInstrument;
        typedef FRA InstrumentUsed;
    public:
        // input
        IborIndexFactory iborIndexFactory;
    protected:
        void verifyInputs() const override {
            BaseClass::verifyInputs();
            QL_REQUIRE(iborIndexFactory != nullptr, "ibor index factory cannot be null");
        }
        void shockImpl(
            const MonthlyRateShocker& monthlyRateShocker
        ) override {
            auto curveReferenceDate = this->yieldTermStructure->referenceDate();
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            QL_REQUIRE(curveReferenceDate == today, "curve's reference date (" << curveReferenceDate << ") is not equal to today's date (" << today << ")");
            auto maxDate = this->yieldTermStructure->maxDate();
            MonthNumber fwdMonth = 0;
            while (true) {
                QuantLib::Period forward(fwdMonth, QuantLib::Months);
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
        QuantLib::Rate impliedRate(
            const pInstrument& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure
        ) const override {
            auto pFRA = std::dynamic_pointer_cast<FRA>(pInst);
            return pFRA->impliedRate(estimatingTermStructure.currentLink());
        }
    };

    template <
        typename I = QuantLib::Linear,
        QuantLib::Integer TENOR_MONTHS = 1,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::BondBasis,
        QuantLib::Compounding COMPOUNDING = QuantLib::Compounding::Continuous,
        QuantLib::Frequency FREQUENCY = QuantLib::Frequency::NoFrequency
    >
    class NominalForwardShockYieldTermStructure : public MonthlyYieldTermStructureShocker<I> {
    private:
        typedef MonthlyYieldTermStructureShocker<I> BaseClass;
    protected:
        typedef typename BaseClass::MonthlyRateShocker MonthlyRateShocker;
        typedef typename BaseClass::MonthNumber MonthNumber;
        typedef typename BaseClass::pInstrument pInstrument;
        typedef NominalForwardRate<TENOR_MONTHS, THIRTY_360_DC_CONVENTION, COMPOUNDING, FREQUENCY> InstrumentUsed;
    protected:
        void shockImpl(
            const MonthlyRateShocker& monthlyRateShocker
        ) override {
            auto curveReferenceDate = this->yieldTermStructure->referenceDate();
            auto maxDate = this->yieldTermStructure->maxDate();
            auto tenor = QuantLib::Period(TENOR_MONTHS, QuantLib::Months);
            MonthNumber forwardMonth = 0;
            QuantLib::Period forward(forwardMonth, QuantLib::Months);
            auto maturityDate = curveReferenceDate + forward + tenor;
            while (maturityDate <= maxDate) {
                QuantLib::Period forward(forwardMonth, QuantLib::Months);
                auto rate = QuantLib::NominalForwardRateHelper::impliedRate(
                    *(this->yieldTermStructure),
                    forward,
                    tenor,
                    QuantLib::Thirty360(THIRTY_360_DC_CONVENTION),
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
                maturityDate = curveReferenceDate + QuantLib::Period(forwardMonth, QuantLib::Months) + tenor;
            };
        }
        QuantLib::Rate impliedRate(
            const pInstrument& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const override {
            auto pInstrument = std::dynamic_pointer_cast<InstrumentUsed>(pInst);
            return pInstrument->impliedRate(discountingTermStructure);
        }
    };
}
