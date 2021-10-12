#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/ParYield.hpp>
#include <memory>
#include <vector>
#include <iostream>

namespace QLUtils {
    template <typename I = QuantLib::Linear, QuantLib::Frequency PAR_YIELD_COUPON_FREQ = QuantLib::Semiannual>
    class ParShockYieldTermStructure {
    protected:
        typedef ParYieldHelper<PAR_YIELD_COUPON_FREQ> ParYieldHelper;
    public:
        // input
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> yieldTermStructure;    // input yield term structure
        // output
        std::shared_ptr<std::vector<QuantLib::Period>> monthlyTenors;   // monthly tenor
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyParYields;  // original monthly par yields
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyParShocks;  // monthly par shock amount
        std::shared_ptr<std::vector<QuantLib::Rate>> monthlyParYieldsShocked;  // shocked monthly par yields
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> zeroCurveShocked;    // shocked output zero curve

        template <typename MONTHLY_PAR_SHOCKER>
        void shock(
            const MONTHLY_PAR_SHOCKER& monthlyParShocker,
            const I& interp = I()   // custom interpretor of type I
        ) {
            auto curveReferenceDate = yieldTermStructure->referenceDate();
            auto maxDate = yieldTermStructure->maxDate();
            auto d = curveReferenceDate;
            d += 1 * QuantLib::Months;
            QuantLib::Natural month = 0;
            monthlyTenors.reset(new std::vector<QuantLib::Period>());
            monthlyParYields.reset(new std::vector<QuantLib::Rate>());
            monthlyParShocks.reset(new std::vector<QuantLib::Rate>());
            monthlyParYieldsShocked.reset(new std::vector<QuantLib::Rate>());
            while (d <= maxDate) {
                QuantLib::Period tenor(++month, QuantLib::Months);
                monthlyTenors->push_back(tenor);
                auto parYield = ParYieldHelper::parYield(yieldTermStructure, tenor); // calculate the original par yield for the tenor
                monthlyParYields->push_back(parYield);
                auto parShock = monthlyParShocker(month);   // get the amount of shock from the rate shocker
                monthlyParShocks->push_back(parShock);
                auto shockedParYield = parYield + parShock; // add the shock to the par yield
                monthlyParYieldsShocked->push_back(shockedParYield);
                d += 1 * QuantLib::Months;
            };
            PiecewiseCurveBuilder<QuantLib::ZeroYield, I> builder;
            auto it_r = monthlyParYieldsShocked->begin();
            for (auto it = monthlyTenors->begin(); it != monthlyTenors->end(); ++it, ++it_r) {
                auto const& tenor = *it;
                auto const& parYieldShocked = *it_r;
                QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> helper = ParYieldHelper(tenor)
                    .withBaseReferenceDate(curveReferenceDate)
                    .withParYield(parYieldShocked);
                builder.AddHelper(helper);
            }
            auto termStructure = builder.GetCurve(curveReferenceDate, QuantLib::Actual365Fixed(), interp);
            zeroCurveShocked = termStructure;
        }
        void verify(std::ostream& os) {
            os << std::fixed;
            os << std::setprecision(16);
            auto it_r = monthlyParYieldsShocked->begin();
            for (auto it = monthlyTenors->begin(); it != monthlyTenors->end(); ++it, ++it_r) {
                auto const& tenor = *it;
                auto const& parYieldShockedActual = *it_r;
                auto parYieldShockedCalculated = ParYieldHelper::parYield(zeroCurveShocked, tenor);
                os << tenor;
                os << "," << "parYieldShockedActual=" << parYieldShockedActual * 100.0;
                os << "," << "parYieldShockedCalculated=" << parYieldShockedCalculated * 100.0;
                os << std::endl;
            }
        }
    };
}