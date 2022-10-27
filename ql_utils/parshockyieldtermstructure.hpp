#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/ParYield.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/bootstrap.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <cmath>

namespace QLUtils {
    template <typename I = QuantLib::Linear>
    class YieldTermStructureShocker: public Bootstrapper {
    public:
        typedef I Interp;

        struct DefaultActualVsImpliedComparison {
            QuantLib::Rate operator() (
                std::ostream& os,
                const std::shared_ptr<BootstrapInstrument>& inst,
                const QuantLib::Rate& actual,
                const QuantLib::Rate& implied
            ) const {
                os << inst->tenor();
                os << "," << inst->ticker();
                os << "," << "actual=" << actual * 100.0;
                os << "," << "implied=" << implied * 100.0;
                os << "," << "diff=" << (implied - actual) * 10000.0 << " bp";
                os << std::endl;
                return implied - actual;
            }
        };
    public:
        // input
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> yieldTermStructure;    // input yield term structure to be shocked
        // output
        std::shared_ptr<std::vector<QuantLib::Period>> monthlyMaturities;   // monthly maturities (can be tenors or forwards periods)
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyBaseRates;  // original monthly rates
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyShocks;  // monthly shock amount
        pInstruments shockedQuotes;  // shocked instruments
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> zeroCurveShocked;    // shocked output zero term structure
    public:
        virtual void verifyInputs() const {
            QL_REQUIRE(yieldTermStructure != nullptr, "input yield termstructure cannot be null");
        }
        virtual void resetOutputs() {
            monthlyMaturities.reset(new std::vector<QuantLib::Period>());
            monthlyBaseRates.reset(new std::vector<QuantLib::Rate>());
            monthlyShocks.reset(new std::vector<QuantLib::Rate>());
            shockedQuotes.reset(new Instruments());
            zeroCurveShocked = nullptr;
        }
    protected:
        // bootstrap shocked zero curve with shocked quotes
        void bootstrapShockedZeroCurve(
            const QuantLib::DayCounter& dayCounter,
            const I& interp
        ) {
            auto curveReferenceDate = yieldTermStructure->referenceDate();
            ZeroCurvesBootstrap<I> bootstrap;
            bootstrap.instruments = shockedQuotes;
            bootstrap.bootstrap(curveReferenceDate, dayCounter, interp);
            zeroCurveShocked = bootstrap.discountZeroCurve;
        }
    public:
        virtual void verifyOutputs() const {
            QL_ASSERT(zeroCurveShocked != nullptr, "shocked zero termstructure cannot be null");
            QL_ASSERT(shockedQuotes != nullptr, "shocked quotes cannot be null");
            QL_ASSERT(!shockedQuotes->empty(), "shocked quotes is empty");
            auto n = shockedQuotes->size();
            QL_ASSERT(monthlyMaturities != nullptr && monthlyMaturities->size() == n, "bad monthly maturity vector");
            QL_ASSERT(monthlyBaseRates != nullptr && monthlyBaseRates->size() == n, "bad monthly base rate vector");
            QL_ASSERT(monthlyShocks != nullptr && monthlyShocks->size() == n, "bad monthly shock vector");
        }
        // implied rate
        virtual QuantLib::Rate impliedRate (
            const pInstrument& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& shockedTS
        ) const = 0;

        template<typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison>
        QuantLib::Rate verify(
            std::ostream& os,
            std::streamsize precision = 16,
            const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
        ) const {
            verifyOutputs();
            QuantLib::Handle<QuantLib::YieldTermStructure> shockedTS(zeroCurveShocked);
            auto const& me = *this;
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
    };

    template <
        typename I = QuantLib::Linear,
        QuantLib::Frequency PAR_YIELD_COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::BondBasis
    >
    class ParShockYieldTermStructure: public YieldTermStructureShocker<I> {
    private:
        template <typename MONTHLY_SHOCKER>
        void shockImpl(
            const MONTHLY_SHOCKER& monthlyShocker
        ) {
            auto curveReferenceDate = this->yieldTermStructure->referenceDate();
            auto maxDate = this->yieldTermStructure->maxDate();
            QuantLib::Natural tenorMonth = 1;
            while (true) {
                QuantLib::Period tenor(tenorMonth, QuantLib::Months);
                auto parYield = ParYieldHelper<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION>::parYield(this->yieldTermStructure, tenor); // calculate the original par yield for the tenor
                auto shock = monthlyShocker(tenorMonth);   // get the amount of shock from the rate shocker
                auto shockedParYield = parYield + shock; // add the shock to the par yield
                std::shared_ptr<BootstrapInstrument> pInst(new ParRate<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION>(tenor, curveReferenceDate));
                pInst->rate() = shockedParYield;
                pInst->ticker() = (std::ostringstream() << "PAR-" << tenor.length() << "M").str();
                if (pInst->maturityDate() > maxDate) {
                    break;
                }
                this->monthlyMaturities->push_back(tenor);
                this->monthlyBaseRates->push_back(parYield);
                this->monthlyShocks->push_back(shock);
                this->shockedQuotes->push_back(pInst);
                tenorMonth++;
            };
        }
    public:
        template <typename MONTHLY_SHOCKER>
        void shock(
            const MONTHLY_SHOCKER& monthlyShocker,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const I& interp = I()   // custom interpretor of type I
        ) {
            this->verifyInputs();
            this->resetOutputs();
            shockImpl(monthlyShocker);
            this->bootstrapShockedZeroCurve(dayCounter, interp);
        }
        QuantLib::Rate impliedRate(
            const std::shared_ptr<BootstrapInstrument>& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto pParInstrument = std::dynamic_pointer_cast<ParInstrument>(pInst);
            return pParInstrument->impliedParRate(discountingTermStructure);
        }
    };

    template <
        typename I = QuantLib::Linear
    >
    class SimpleForwardTermStructureShocker: public YieldTermStructureShocker<I> {
    public:
        // input
        IborIndexFactory iborIndexFactory;
    private:
        template <typename MONTHLY_SHOCKER>
        void shockImpl(
            const MONTHLY_SHOCKER& monthlyShocker
        ) {
            auto curveReferenceDate = this->yieldTermStructure->referenceDate();
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            QL_REQUIRE(curveReferenceDate == today, "curve's reference date (" << curveReferenceDate << ") is not equal to today's date (" << today << ")");
            auto maxDate = this->yieldTermStructure->maxDate();
            QuantLib::Natural fwdMonth = 0;
            while (true) {
                QuantLib::Period forward(fwdMonth, QuantLib::Months);
                std::shared_ptr<FRA> pInst(new FRA(iborIndexFactory, forward));
                if (pInst->maturityDate() > maxDate) {
                    break;
                }
                auto fwdRate = pInst->impliedRate(this->yieldTermStructure); // calculate the original forward rate for the forward period
                auto shock = monthlyShocker(fwdMonth);   // get the amount of shock from the rate shocker
                auto shockedFwdRate = fwdRate + shock; // add the shock to the forward rate
                pInst->rate() = shockedFwdRate;
                pInst->ticker() = (std::ostringstream() << "FWD-" << forward.length() << "M").str();
                this->monthlyMaturities->push_back(forward);
                this->monthlyBaseRates->push_back(fwdRate);
                this->monthlyShocks->push_back(shock);
                this->shockedQuotes->push_back(pInst);
                fwdMonth++;
            };
        }
    public:
        void verifyInputs() const {
            YieldTermStructureShocker<I>::verifyInputs();
            QL_REQUIRE(iborIndexFactory != nullptr, "ibor index factory cannot be null");
        }

        template <typename MONTHLY_SHOCKER>
        void shock(
            const MONTHLY_SHOCKER& monthlyShocker,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const I& interp = I()   // custom interpretor of type I
        ) {
            this->verifyInputs();
            this->clearOutputs();
            shockImpl(monthlyShocker);
            this->bootstrapShockedZeroCurve(dayCounter, interp);
        }
        QuantLib::Rate impliedRate(
            const std::shared_ptr<BootstrapInstrument>& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure
        ) const {
            auto pFRA = std::dynamic_pointer_cast<FRA>(pInst);
            return pFRA->impliedRate(estimatingTermStructure.currentLink());
        }
    };
}