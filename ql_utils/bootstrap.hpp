#pragma once

#include <memory>
#include <vector>
#include <ql/quantlib.hpp>
#include <ql_utils/all.hpp>
#include <iostream>
#include <iomanip>

namespace QLUtils {
    // bootstrapper base class
    template <typename I>
    class Bootstrapper {
    protected:
        // zero curve builder
        std::shared_ptr<PiecewiseCurveBuilder<QuantLib::ZeroYield, I>> curveBuilder_;
    public:
        Bootstrapper() {}
        virtual void bootstrap(const QuantLib::Date& curveReferenceDate, const I& interp) = 0;
        virtual void verify(std::ostream& os) = 0;
        const std::vector<QuantLib::ext::shared_ptr<QuantLib::RateHelper>>& rateHelpers() const {
            QL_REQUIRE(curveBuilder_ != nullptr, "must perform the bootstrapping first");
            return curveBuilder_->helpers();
        }
    };

    // overnight indexed swap bootstrapping
    template <typename OvernightIndex, typename SwapTraits, typename I = QuantLib::Linear>
    class OISZeroCurveBootstrap : public Bootstrapper<I> {
    private:
        SwapTraits swapTraits_; // swap traits
    public:
        template <typename INSTRUMENT>
        using Instruments = std::vector<std::shared_ptr<INSTRUMENT>>;

        template <typename INSTRUMENT>
        using pInstruments = std::shared_ptr<Instruments<INSTRUMENT>>;

        // input
        pInstruments<OISSwapIndex<>> instruments;

        // output
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> zeroCurve;

        OISZeroCurveBootstrap() : Bootstrapper<I>() {}
        void bootstrap(const QuantLib::Date& curveReferenceDate, const I& interp = I());
        void verify(std::ostream& os);
    };

    template <typename OvernightIndex, typename SwapTraits, typename I>
    void OISZeroCurveBootstrap<OvernightIndex, SwapTraits, I>::bootstrap(const QuantLib::Date& curveReferenceDate, const I& interp) {
        QL_REQUIRE(instruments != nullptr, "instruments is not set");
        QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
        auto overnightIndex = QuantLib::ext::make_shared<OvernightIndex>();
        this->curveBuilder_.reset(new PiecewiseCurveBuilder<QuantLib::ZeroYield, I>());
        for (auto it = instruments->begin(); it != instruments->end(); ++it) {
            auto const& inst = *it;
            if (inst->use()) {
                auto quote = QuantLib::ext::make_shared < QuantLib::SimpleQuote >(inst->value());
                this->curveBuilder_->AddOIS(
                    quote,
                    swapTraits_.settlementDays(),
                    inst->tenor(),
                    overnightIndex,
                    swapTraits_.telescopicValueDates(),
                    swapTraits_.averagingMethod(),
                    swapTraits_.paymentAdjustment()
                );
            }
        }
        zeroCurve = this->curveBuilder_->GetCurve(curveReferenceDate, QuantLib::Actual365Fixed(), interp);
    }

    // verify the bootstrapping
    template <typename OvernightIndex, typename SwapTraits, typename I>
    void OISZeroCurveBootstrap<OvernightIndex, SwapTraits, I>::verify(std::ostream& os) {
        os << std::fixed;
        os << std::setprecision(16);
        zeroCurve->enableExtrapolation(true);
        QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> discountCurve(zeroCurve);
        QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> estimatingCurve(zeroCurve);
        auto overnightIndex = QuantLib::ext::make_shared<OvernightIndex>(estimatingCurve);
        QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountCurve));
        for (auto it = instruments->begin(); it != instruments->end(); ++it) {
            auto const& inst = *it;
            if (inst->use()) {
                auto const& actual = inst->value();
                QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> swap = QuantLib::MakeOIS(inst->tenor(), overnightIndex)
                    .withSettlementDays(swapTraits_.settlementDays())
                    .withTelescopicValueDates(swapTraits_.telescopicValueDates())
                    .withPaymentAdjustment(swapTraits_.paymentAdjustment())
                    .withAveragingMethod(swapTraits_.averagingMethod());
                auto calculated = swap->fairRate();
                auto startDate = swap->startDate();
                auto maturityDate = swap->maturityDate();
                os << "tenor=" << inst->tenor();
                os << "," << "ticker=" << inst->ticker();
                os << "," << "start=" << DateFormat<char>::to_yyyymmdd(startDate, true);
                os << "," << "maturity=" << DateFormat<char>::to_yyyymmdd(maturityDate, true);
                os << "," << "actual=" << actual * 100.0;
                os << "," << "calculated=" << calculated * 100.0;
                os << std::endl;
            }
        }
    }

    // index estimating zero curve bootstrapping
    template <typename TargetIndex, QuantLib::Natural TargetIndexTenorMonths, typename SwapTraits, typename I = QuantLib::Linear>
    class SwapEstimatingZeroCurveBootstrap : public Bootstrapper<I> {
    private:
        SwapTraits swapTraits_; // swap traits
    public:
        template <typename INSTRUMENT>
        using Instruments = std::vector<std::shared_ptr<INSTRUMENT>>;

        template <typename INSTRUMENT>
        using pInstruments = std::shared_ptr<Instruments<INSTRUMENT>>;

          // input
          pInstruments<SwapCurveInstrument<>> instruments;
          QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discountingTermStructure;

          // output
          QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> estimatingZeroCurve;

          SwapEstimatingZeroCurveBootstrap() : Bootstrapper<I>() {}
          void bootstrap(const QuantLib::Date& curveReferenceDate, const I& interp = I());
          void verify(std::ostream& os);
    };

    template <typename TargetIndex, QuantLib::Natural TargetIndexTenorMonths, typename SwapTraits, typename I>
    void SwapEstimatingZeroCurveBootstrap<TargetIndex, TargetIndexTenorMonths, SwapTraits, I>::bootstrap(const QuantLib::Date& curveReferenceDate, const I& interp) {
        QL_REQUIRE(instruments != nullptr, "instruments is not set");
        QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
        QuantLib::Handle<QuantLib::YieldTermStructure> discountingCurve(discountingTermStructure);
        auto targetIndex = QuantLib::ext::make_shared<TargetIndex>(TargetIndexTenorMonths * QuantLib::Months);
        this->curveBuilder_.reset(new PiecewiseCurveBuilder<QuantLib::ZeroYield, I>());
        for (auto it = instruments->begin(); it != instruments->end(); ++it) {
            auto const& instrument = *it;
            if (instrument->use()) {
                auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(instrument->value());
                if (instrument->instType() == SwapCurveInstrument<>::Deposit) {
                    this->curveBuilder_->AddDeposit(quote, targetIndex);
                }
                else if (instrument->instType() == SwapCurveInstrument<>::Future) {
                    this->curveBuilder_->AddFuture(quote, instrument->datedDate(), targetIndex);
                }
                else if (instrument->instType() == SwapCurveInstrument<>::FRA) {
                    this->curveBuilder_->AddFRA(quote, instrument->tenor(), targetIndex);
                }
                else if (instrument->instType() == SwapCurveInstrument<>::Swap) {
                    this->curveBuilder_->AddSwap(
                        quote,
                        swapTraits_.settlementDays(),
                        instrument->tenor(),
                        swapTraits_.fixingCalendar(),
                        swapTraits_.fixedLegFrequency(),
                        swapTraits_.fixedLegConvention(),
                        swapTraits_.fixedLegDayCount(),
                        targetIndex,
                        discountingCurve,
                        swapTraits_.endOfMonth()
                    );
                }
            }
        }
        estimatingZeroCurve = this->curveBuilder_->GetCurve(curveReferenceDate, QuantLib::Actual365Fixed(), interp);
    }

    // verify the bootstrapping
    template <typename TargetIndex, QuantLib::Natural TargetIndexTenorMonths, typename SwapTraits, typename I>
    void SwapEstimatingZeroCurveBootstrap<TargetIndex, TargetIndexTenorMonths, SwapTraits, I>::verify(std::ostream& os) {
        os << std::fixed;
        os << std::setprecision(16);
        discountingTermStructure->enableExtrapolation(true);
        estimatingZeroCurve->enableExtrapolation(true);
        QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> discountCurve(discountingTermStructure);
        QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> estimatingCurve(estimatingZeroCurve);
        QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountCurve));
        auto targetIndex = QuantLib::ext::make_shared<TargetIndex>(TargetIndexTenorMonths * QuantLib::Months, estimatingCurve);
        for (auto it = instruments->begin(); it != instruments->end(); ++it) { // for each instrument
            auto const& inst = *it;
            if (inst->use()) {
                auto const& actual = inst->value();
                auto valueIsPrice = (inst->valueType() == SwapCurveInstrument<>::Price);
                QuantLib::Real calculated = 0.0;
                QuantLib::Date startDate;
                QuantLib::Date maturityDate;
                if (inst->instType() == SwapCurveInstrument<>::Deposit && inst->tenor() == TargetIndexTenorMonths * QuantLib::Months) {
                    QuantLib::DepositRateHelper rateHelper(0.0, targetIndex);
                    rateHelper.setTermStructure(&(*estimatingZeroCurve));
                    calculated = rateHelper.impliedQuote();
                    startDate = rateHelper.earliestDate();
                    maturityDate = rateHelper.maturityDate();
                }
                else if (inst->instType() == SwapCurveInstrument<>::Future) {
                    // inst->datedDate() is the IMM date
                    QuantLib::FuturesRateHelper rateHelper(100.0, inst->datedDate(), targetIndex);
                    rateHelper.setTermStructure(&(*estimatingZeroCurve));
                    calculated = rateHelper.impliedQuote();
                    startDate = rateHelper.earliestDate(); // IMM date
                    maturityDate = rateHelper.maturityDate();
                }
                else if (inst->instType() == SwapCurveInstrument<>::FRA) {
                    QuantLib::FraRateHelper rateHelper(0.0, inst->tenor(), targetIndex);
                    rateHelper.setTermStructure(&(*estimatingZeroCurve));
                    calculated = rateHelper.impliedQuote();
                    startDate = rateHelper.earliestDate();
                    maturityDate = rateHelper.maturityDate();
                }
                else if (inst->instType() == SwapCurveInstrument<>::Swap) {
                    /*
                    QuantLib::SwapRateHelper rateHelper(
                        0.0,
                        inst->tenor,
                        swapTraits_.fixingCalendar(),
                        swapTraits_.fixedLegFrequency(),
                        swapTraits_.fixedLegConvention(),
                        swapTraits_.fixedLegDayCount(),
                        targetIndex,
                        QuantLib::Handle<QuantLib::Quote>(),
                        0 * QuantLib::Days,
                        discountCurve,
                        swapTraits_.settlementDays(),
                        QuantLib::Pillar::LastRelevantDate,
                        QuantLib::Date(),
                        swapTraits_.endOfMonth()
                    );
                    rateHelper.setTermStructure(&(*estimatingZeroCurve));
                    calculated = rateHelper.impliedQuote();
                    startDate = rateHelper.earliestDate();
                    maturityDate = rateHelper.maturityDate();
                    */
                    // create a vanilla swap and price it with the discounting curve and the estimating curve
                    QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> swap = QuantLib::MakeVanillaSwap(inst->tenor(), targetIndex)
                        .withSettlementDays(swapTraits_.settlementDays())
                        .withFixedLegCalendar(swapTraits_.fixingCalendar())
                        .withFixedLegTenor(swapTraits_.fixedLegTenor())
                        .withFixedLegConvention(swapTraits_.fixedLegConvention())
                        .withFixedLegDayCount(swapTraits_.fixedLegDayCount())
                        .withFixedLegEndOfMonth(swapTraits_.endOfMonth())
                        .withFloatingLegCalendar(swapTraits_.fixingCalendar())
                        .withFloatingLegEndOfMonth(swapTraits_.endOfMonth())
                        .withDiscountingTermStructure(discountCurve);
                    swap->setPricingEngine(swapPricingEngine);
                    calculated = swap->fairRate();
                    startDate = swap->startDate();
                    maturityDate = swap->maturityDate();
                }
                os << "tenor=" << inst->tenor();
                os << "," << "ticker=" << inst->ticker();
                os << "," << "start=" << DateFormat<char>::to_yyyymmdd(startDate, true);
                os << "," << "maturity=" << DateFormat<char>::to_yyyymmdd(maturityDate, true);
                os << "," << "actual=" << (valueIsPrice ? actual : actual * 100.0);
                os << "," << "calculated=" << (valueIsPrice ? calculated : calculated * 100.0);
                os << std::endl;
            }
        }
    }
}