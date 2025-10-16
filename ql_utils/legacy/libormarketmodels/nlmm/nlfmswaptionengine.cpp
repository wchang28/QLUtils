#pragma once

#include <ql/quantlib.hpp>
#include <utility>
#include <cmath>

namespace QuantLib {
    //! %Normal/Bachelier Libor forward model swaption engine based on Bachelier Black formula
    /*! \ingroup swaptionengines */
    class NLfmSwaptionEngine : public GenericModelEngine<LiborForwardModel, Swaption::arguments, Swaption::results> {
    public:
        NLfmSwaptionEngine(
            const ext::shared_ptr<LiborForwardModel>& model,
            Handle<YieldTermStructure> discountCurve
        ):
            GenericModelEngine<LiborForwardModel, Swaption::arguments, Swaption::results>(model),
            discountCurve_(std::move(discountCurve))
        {
            registerWith(discountCurve_);
        }
        void calculate() const override {
            QL_REQUIRE(arguments_.settlementMethod != Settlement::ParYieldCurve,
                "cash settled (ParYieldCurve) swaptions not priced with Lfm engine");

            static const Spread basisPoint = 1.0e-4;

            auto swap = arguments_.swap;
            swap->setPricingEngine(ext::shared_ptr<PricingEngine>(
                new DiscountingSwapEngine(discountCurve_, false)));

            Spread correction = swap->spread() *
                std::fabs(swap->floatingLegBPS() / swap->fixedLegBPS());
            Rate fixedRate = swap->fixedRate() - correction;
            Rate fairRate = swap->fairRate() - correction;

            ext::shared_ptr<SwaptionVolatilityMatrix> volatility =
                model_->getSwaptionVolatilityMatrix();

            Date referenceDate = volatility->referenceDate();
            DayCounter dayCounter = volatility->dayCounter();

            Time exercise = dayCounter.yearFraction(referenceDate,
                arguments_.exercise->date(0));
            Time swapLength =
                dayCounter.yearFraction(referenceDate,
                    arguments_.fixedPayDates.back())
                - dayCounter.yearFraction(referenceDate,
                    arguments_.fixedResetDates[0]);

            Option::Type w = arguments_.type == Swap::Payer ? Option::Call : Option::Put;
            Volatility vol = volatility->volatility(exercise, swapLength,
                fairRate, true);

            // !!! the original QuantLib code does not apply absolute value when calculating annuity which is a bug !!!
            Real annuity = std::fabs(swap->fixedLegBPS()) / basisPoint;
            //results_.value = annuity * blackFormula(w, fixedRate, fairRate, vol * std::sqrt(exercise));
            results_.value = annuity * bachelierBlackFormula(w, fixedRate, fairRate, vol * std::sqrt(exercise));
        }

    private:
        Handle<YieldTermStructure> discountCurve_;
    };
}
