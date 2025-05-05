#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/ParYield.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/bootstrap.hpp>
#include <ql_utils/ratehelpers/nominal_forward_ratehelper.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <cmath>
#include <functional>

namespace QLUtils {
    template <typename I = QuantLib::Linear>
    class YieldTermStructureShocker: public Bootstrapper {
    public:
        typedef I Interp;
        typedef std::function<QuantLib::Rate(QuantLib::Natural)> MonthlyRateShocker;

        struct DefaultActualVsImpliedComparison {
            QuantLib::Rate operator() (
                std::ostream& os,
                const std::shared_ptr<BootstrapInstrument>& inst,
                const QuantLib::Rate& actual,
                const QuantLib::Rate& implied
            ) const {
                os << inst->tenor();
                os << "," << inst->ticker();
                os << "," << "actual=" << actual * 100.;
                os << "," << "implied=" << implied * 100.;
                os << "," << "diff=" << (implied - actual) * 10000. << " bp";
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
        virtual void verifyOutputs() const {
            QL_ASSERT(zeroCurveShocked != nullptr, "shocked zero termstructure cannot be null");
            QL_ASSERT(shockedQuotes != nullptr, "shocked quotes cannot be null");
            QL_ASSERT(!shockedQuotes->empty(), "shocked quotes is empty");
            auto n = shockedQuotes->size();
            QL_ASSERT(monthlyMaturities != nullptr && monthlyMaturities->size() == n, "bad monthly maturity vector");
            QL_ASSERT(monthlyBaseRates != nullptr && monthlyBaseRates->size() == n, "bad monthly base rate vector");
            QL_ASSERT(monthlyShocks != nullptr && monthlyShocks->size() == n, "bad monthly shock vector");
        }
    private:
        // bootstrap shocked zero curve with shocked quotes
        void bootstrapShockedZeroCurve(
            const QuantLib::DayCounter& dayCounter,
            const I& interp
        ) {
            auto curveReferenceDate = yieldTermStructure->referenceDate();
            ZeroCurvesBootstrap<I> bootstrap;
            bootstrap.instruments = shockedQuotes;
            bootstrap.bootstrap(curveReferenceDate, dayCounter, interp);
            zeroCurveShocked = bootstrap.discountCurve;
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
        template <typename MONTHLY_SHOCKER>
        void shock(
            const MONTHLY_SHOCKER& monthlyShocker,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const I& interp = I()   // custom interpretor of type I
        ) {
            auto monthlyRateShocker = [&monthlyShocker](QuantLib::Natural month) {
                return (QuantLib::Rate)monthlyShocker(month);
            };
            verifyInputs();
            resetOutputs();
            shockImpl(monthlyRateShocker);
            bootstrapShockedZeroCurve(dayCounter, interp);
        }
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
    protected:
        typedef typename YieldTermStructureShocker<I>::MonthlyRateShocker MonthlyRateShocker;
        typedef std::shared_ptr<BootstrapInstrument> pInstrument;
        void shockImpl(
            const MonthlyRateShocker& monthlyRateShocker
        ) {
            auto curveReferenceDate = this->yieldTermStructure->referenceDate();
            auto maxDate = this->yieldTermStructure->maxDate();
            QuantLib::Natural tenorMonth = 1;
            while (true) {
                QuantLib::Period tenor(tenorMonth, QuantLib::Months);
                auto parYield = ParYieldHelper<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION>::parYield(this->yieldTermStructure, tenor); // calculate the original par yield for the tenor
                auto shock = monthlyRateShocker(tenorMonth);   // get the amount of shock from the rate shocker
                auto shockedParYield = parYield + shock; // add the shock to the par yield
                std::shared_ptr<BootstrapInstrument> pInst(new ParRate<PAR_YIELD_COUPON_FREQ, THIRTY_360_DC_CONVENTION>(tenor, curveReferenceDate));
                pInst->rate() = shockedParYield;
                std::ostringstream oss;
                oss << "PAR-" << tenor.length() << "M";
                pInst->ticker() = oss.str();
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
        QuantLib::Rate impliedRate(
            const pInstrument& pInst,
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
    public:
        void verifyInputs() const {
            YieldTermStructureShocker<I>::verifyInputs();
            QL_REQUIRE(iborIndexFactory != nullptr, "ibor index factory cannot be null");
        }
    protected:
        typedef typename YieldTermStructureShocker<I>::MonthlyRateShocker MonthlyRateShocker;
        typedef std::shared_ptr<BootstrapInstrument> pInstrument;
        void shockImpl(
            const MonthlyRateShocker& monthlyRateShocker
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
                auto shock = monthlyRateShocker(fwdMonth);   // get the amount of shock from the rate shocker
                auto shockedFwdRate = fwdRate + shock; // add the shock to the forward rate
                pInst->rate() = shockedFwdRate;
                std::ostringstream oss;
                oss << "FWD-" << forward.length() << "M";
                pInst->ticker() = oss.str();
                this->monthlyMaturities->push_back(forward);
                this->monthlyBaseRates->push_back(fwdRate);
                this->monthlyShocks->push_back(shock);
                this->shockedQuotes->push_back(pInst);
                fwdMonth++;
            };
        }
        QuantLib::Rate impliedRate(
            const pInstrument& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure
        ) const {
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
    class NominalForwardShockYieldTermStructure : public YieldTermStructureShocker<I> {
    protected:
        typedef NominalForwardRate<TENOR_MONTHS, THIRTY_360_DC_CONVENTION, COMPOUNDING, FREQUENCY> InstrumentUsed;
        typedef typename YieldTermStructureShocker<I>::MonthlyRateShocker MonthlyRateShocker;
        typedef std::shared_ptr<BootstrapInstrument> pInstrument;
        void shockImpl(
            const MonthlyRateShocker& monthlyRateShocker
        ) {
            auto curveReferenceDate = this->yieldTermStructure->referenceDate();
            auto maxDate = this->yieldTermStructure->maxDate();
            auto tenor = QuantLib::Period(TENOR_MONTHS, QuantLib::Months);
            QuantLib::Natural forwardMonth = 0;
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
                std::shared_ptr<BootstrapInstrument> pInst(new InstrumentUsed(forward, curveReferenceDate));
                pInst->rate() = shockedRate;
                std::ostringstream oss;
                oss << "FWD-" << forward.length() << "Mx" << tenor.length() << "M";
                pInst->ticker() = oss.str();
                this->monthlyMaturities->push_back(forward);
                this->monthlyBaseRates->push_back(rate);
                this->monthlyShocks->push_back(shock);
                this->shockedQuotes->push_back(pInst);
                forwardMonth++;
                maturityDate = curveReferenceDate + QuantLib::Period(forwardMonth, QuantLib::Months) + tenor;
            };
        }
        QuantLib::Rate impliedRate(
            const pInstrument& pInst,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto pInstrument = std::dynamic_pointer_cast<InstrumentUsed>(pInst);
            return pInstrument->impliedRate(discountingTermStructure);
        }
    };
}
