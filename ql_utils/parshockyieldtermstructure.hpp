#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/ParYield.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/bootstrap.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <cmath>

namespace QLUtils {
    template <typename I = QuantLib::Linear, QuantLib::Frequency PAR_YIELD_COUPON_FREQ = QuantLib::Semiannual>
    class ParShockYieldTermStructure : public Bootstrapper {
    public:
        // input
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> yieldTermStructure;    // input yield term structure to be par shocked
        // output
        std::shared_ptr<std::vector<QuantLib::Period>> monthlyTenors;   // monthly tenor
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyParYields;  // original monthly par yields
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyParShocks;  // monthly par shock amount
        pInstruments shockedParRateQuotes;
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> zeroCurveShocked;    // shocked output zero curve

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

        void clearOutputs() {
            monthlyTenors = nullptr;
            monthlyParYields = nullptr;
            monthlyParShocks = nullptr;
            shockedParRateQuotes = nullptr;
            zeroCurveShocked = nullptr;
        }
    private:
        void checkInstruments() const {
            QL_REQUIRE(shockedParRateQuotes != nullptr, "instruments is not set");
            QL_REQUIRE(!shockedParRateQuotes->empty(), "instruments cannot be empty");
        }
    public:
        template <typename MONTHLY_PAR_SHOCKER>
        void shock(
            const MONTHLY_PAR_SHOCKER& monthlyParShocker,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const I& interp = I()   // custom interpretor of type I
        ) {
            clearOutputs();
            QL_REQUIRE(yieldTermStructure != nullptr, "yield curve termstructure cannot be null");
            auto curveReferenceDate = yieldTermStructure->referenceDate();
            auto maxDate = yieldTermStructure->maxDate();
            auto d = curveReferenceDate;
            d += 1 * QuantLib::Months;
            QuantLib::Natural month = 0;
            monthlyTenors.reset(new std::vector<QuantLib::Period>());
            monthlyParYields.reset(new std::vector<QuantLib::Rate>());
            monthlyParShocks.reset(new std::vector<QuantLib::Rate>());
            shockedParRateQuotes.reset(new Instruments());
            while (d <= maxDate) {
                QuantLib::Period tenor(++month, QuantLib::Months);
                monthlyTenors->push_back(tenor);
                auto parYield = ParYieldHelper<PAR_YIELD_COUPON_FREQ>::parYield(yieldTermStructure, tenor); // calculate the original par yield for the tenor
                monthlyParYields->push_back(parYield);
                auto parShock = monthlyParShocker(month);   // get the amount of shock from the rate shocker
                monthlyParShocks->push_back(parShock);
                auto shockedParYield = parYield + parShock; // add the shock to the par yield
                std::shared_ptr<BootstrapInstrument> inst(new ParRate<PAR_YIELD_COUPON_FREQ>(tenor, curveReferenceDate));
                inst->rate() = shockedParYield;
                inst->ticker() = (std::ostringstream() << "PAR-" << tenor.length() << "M").str();
                shockedParRateQuotes->push_back(inst);
                d += 1 * QuantLib::Months;
            };
            ZeroCurvesBootstrap<I> bootstrap;
            bootstrap.instruments = shockedParRateQuotes;
            bootstrap.bootstrap(curveReferenceDate, dayCounter, interp);
            zeroCurveShocked = bootstrap.discountZeroCurve;
        }
        template<typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison>
        QuantLib::Rate verify(
            std::ostream& os,
            std::streamsize precision = 16,
            const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
        ) const {
            QL_REQUIRE(zeroCurveShocked != nullptr, "shocked zero curve cannot be null");
            checkInstruments();
            QuantLib::Handle<QuantLib::YieldTermStructure> discountingTermStructure(zeroCurveShocked);
            return verifyImpl(
                shockedParRateQuotes,
                [&discountingTermStructure](const pInstrument& inst) -> QuantLib::Rate {
                    auto parInstrument = std::dynamic_pointer_cast<ParInstrument>(inst);
                    return parInstrument->impliedParRate(discountingTermStructure);
                },
                os,
                precision,
                compare
            );
        }
    };
}