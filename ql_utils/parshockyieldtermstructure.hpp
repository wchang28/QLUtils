#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/ParYield.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/dateformat.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/bootstrap.hpp>
#include <memory>
#include <vector>
#include <iostream>

namespace QLUtils {
    template <typename I = QuantLib::Linear, QuantLib::Frequency PAR_YIELD_COUPON_FREQ = QuantLib::Semiannual>
    class ParShockYieldTermStructure : public Verifiable {
    public:
        // input
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> yieldTermStructure;    // input yield term structure
        // output
        std::shared_ptr<std::vector<QuantLib::Period>> monthlyTenors;   // monthly tenor
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyParYields;  // original monthly par yields
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyParShocks;  // monthly par shock amount
        std::shared_ptr<std::vector<std::shared_ptr<BootstrapInstrument>>> shockedParRateQuotes;
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> zeroCurveShocked;    // shocked output zero curve

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
            shockedParRateQuotes.reset(new std::vector<std::shared_ptr<BootstrapInstrument>>());
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
                shockedParRateQuotes->push_back(inst);
                d += 1 * QuantLib::Months;
            };
            ZeroCurvesBootstrap<I> bootstrap;
            bootstrap.instruments = shockedParRateQuotes;
            bootstrap.bootstrap(curveReferenceDate, dayCounter, interp);
            zeroCurveShocked = bootstrap.discountZeroCurve;
        }
        void verify(std::ostream& os, std::streamsize precision = 16) const {
            QL_REQUIRE(zeroCurveShocked != nullptr, "shocked zero curve cannot be null");
            checkInstruments();
            os << std::fixed;
            os << std::setprecision(precision);
            QuantLib::Handle<QuantLib::YieldTermStructure> discountCurve(zeroCurveShocked);
            QuantLib::Handle<QuantLib::YieldTermStructure> estimatingCurve(zeroCurveShocked);
            for (auto const& inst : *shockedParRateQuotes) {
                auto const& actual = inst->value();
                auto implied = inst->impliedQuote(estimatingCurve, discountCurve);
                auto startDate = inst->startDate();
                auto maturityDate = inst->maturityDate();
                os << inst->tenor();
                os << "," << "start=" << DateFormat<char>::to_yyyymmdd(startDate, true);
                os << "," << "maturity=" << DateFormat<char>::to_yyyymmdd(maturityDate, true);
                os << "," << "shockActual=" << actual * 100.0;
                os << "," << "shockImplied=" << implied * 100.0;
                os << std::endl;
            }
        }
    };
}