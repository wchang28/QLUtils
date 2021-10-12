#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/PiecewiseCurveBuilder.hpp>
#include <ql_utils/dateformat.hpp>
#include <memory>
#include <vector>
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
        template<typename INSTRUMENT>
        void checkInstruments(const std::shared_ptr<std::vector<std::shared_ptr<INSTRUMENT>>>& instruments) const {
            QL_REQUIRE(instruments != nullptr, "instruments is not set");
            QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
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
        this->checkInstruments(instruments);
        auto overnightIndex = QuantLib::ext::make_shared<OvernightIndex>();
        this->curveBuilder_.reset(new PiecewiseCurveBuilder<QuantLib::ZeroYield, I>());
        for (auto const& inst: *instruments) { // for each instrument
            if (inst->use()) {
                auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(inst->value());
                this->curveBuilder_->AddOIS(
                    quote,
                    swapTraits_.settlementDays(inst->tenor()),
                    inst->tenor(),
                    overnightIndex,
                    swapTraits_.telescopicValueDates(inst->tenor()),
                    swapTraits_.averagingMethod(inst->tenor()),
                    swapTraits_.paymentAdjustment(inst->tenor())
                );
            }
        }
        zeroCurve = this->curveBuilder_->GetCurve(curveReferenceDate, QuantLib::Actual365Fixed(), interp);
    }

    // verify the bootstrapping
    template <typename OvernightIndex, typename SwapTraits, typename I>
    void OISZeroCurveBootstrap<OvernightIndex, SwapTraits, I>::verify(std::ostream& os) {
        this->checkInstruments(instruments);
        os << std::fixed;
        os << std::setprecision(16);
        zeroCurve->enableExtrapolation(true);
        QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> discountCurve(zeroCurve);
        QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> estimatingCurve(zeroCurve);
        auto overnightIndex = QuantLib::ext::make_shared<OvernightIndex>(estimatingCurve);
        QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountCurve));
        for (auto const& inst : *instruments) { // for each instrument
            if (inst->use()) {
                auto const& actual = inst->value();
                QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> swap = QuantLib::MakeOIS(inst->tenor(), overnightIndex)
                    .withSettlementDays(swapTraits_.settlementDays(inst->tenor()))
                    .withTelescopicValueDates(swapTraits_.telescopicValueDates(inst->tenor()))
                    .withPaymentAdjustment(swapTraits_.paymentAdjustment(inst->tenor()))
                    .withAveragingMethod(swapTraits_.averagingMethod(inst->tenor()));
                auto calculated = swap->fairRate();
                auto startDate = swap->startDate();
                auto maturityDate = swap->maturityDate();
                os << inst->tenor();
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
    class SwapZeroCurveBootstrap : public Bootstrapper<I> {
    private:
        SwapTraits swapTraits_; // swap traits
    public:
        template <typename INSTRUMENT>
        using Instruments = std::vector<std::shared_ptr<INSTRUMENT>>;

        template <typename INSTRUMENT>
        using pInstruments = std::shared_ptr<Instruments<INSTRUMENT>>;

        // input
        pInstruments<SwapCurveInstrument<>> instruments;
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discountingTermStructure;   // this can be nullptr

        // output
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> discountZeroCurve;   // this can be nullptr
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> estimatingZeroCurve;

        SwapZeroCurveBootstrap() : Bootstrapper<I>() {}
        void bootstrap(const QuantLib::Date& curveReferenceDate, const I& interp = I());
        void verify(std::ostream& os);
    };

    template <typename TargetIndex, QuantLib::Natural TargetIndexTenorMonths, typename SwapTraits, typename I>
    void SwapZeroCurveBootstrap<TargetIndex, TargetIndexTenorMonths, SwapTraits, I>::bootstrap(const QuantLib::Date& curveReferenceDate, const I& interp) {
        this->checkInstruments(instruments);
        QuantLib::Handle<QuantLib::YieldTermStructure> discountingCurve(discountingTermStructure);
        auto targetIndex = QuantLib::ext::make_shared<TargetIndex>(TargetIndexTenorMonths * QuantLib::Months);
        this->curveBuilder_.reset(new PiecewiseCurveBuilder<QuantLib::ZeroYield, I>());
        for (auto const& inst : *instruments) { // for each instrument
            if (inst->use()) {
                auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(inst->value());
                if (inst->instType() == SwapCurveInstrument<>::Deposit) {
                    this->curveBuilder_->AddDeposit(quote, targetIndex);
                }
                else if (inst->instType() == SwapCurveInstrument<>::Future) {
                    auto future = std::static_pointer_cast<IMMFuture<>>(inst);
                    auto const& convexityAdj = future->convexityAdj();
                    auto convexityAdjQuote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(convexityAdj);
                    this->curveBuilder_->AddFuture(quote, future->immDate(), targetIndex, convexityAdjQuote);
                }
                else if (inst->instType() == SwapCurveInstrument<>::FRA) {
                    this->curveBuilder_->AddFRA(quote, inst->tenor(), targetIndex);
                }
                else if (inst->instType() == SwapCurveInstrument<>::Swap) {
                    this->curveBuilder_->AddSwap(
                        quote,
                        swapTraits_.settlementDays(inst->tenor()),
                        inst->tenor(),
                        swapTraits_.fixingCalendar(inst->tenor()),
                        swapTraits_.fixedLegFrequency(inst->tenor()),
                        swapTraits_.fixedLegConvention(inst->tenor()),
                        swapTraits_.fixedLegDayCount(inst->tenor()),
                        targetIndex,
                        discountingCurve,
                        swapTraits_.endOfMonth(inst->tenor())
                    );
                }
            }
        }
        estimatingZeroCurve = this->curveBuilder_->GetCurve(curveReferenceDate, QuantLib::Actual365Fixed(), interp);
        discountZeroCurve = (discountingCurve.empty() ? estimatingZeroCurve : nullptr);
    }

    // verify the bootstrapping
    template <typename TargetIndex, QuantLib::Natural TargetIndexTenorMonths, typename SwapTraits, typename I>
    void SwapZeroCurveBootstrap<TargetIndex, TargetIndexTenorMonths, SwapTraits, I>::verify(std::ostream& os) {
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discountTS = (discountingTermStructure ? discountingTermStructure : discountZeroCurve);
        QL_REQUIRE(discountTS != nullptr, "discount term structure cannot be null");
        QL_REQUIRE(estimatingZeroCurve != nullptr, "estimating zero curve cannot be null");
        this->checkInstruments(instruments);
        os << std::fixed;
        os << std::setprecision(16);
        QuantLib::Handle<QuantLib::YieldTermStructure> discountCurve(discountTS);
        QuantLib::Handle<QuantLib::YieldTermStructure> estimatingCurve(estimatingZeroCurve);
        QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountCurve));
        auto targetIndex = QuantLib::ext::make_shared<TargetIndex>(TargetIndexTenorMonths * QuantLib::Months, estimatingCurve);
        for (auto const& inst : *instruments) { // for each instrument
            if (inst->use()) {
                auto const& actual = inst->value();
                auto valueIsPrice = (inst->valueType() == SwapCurveInstrument<>::Price);
                QuantLib::Real calculated = 0.0;
                QuantLib::Date startDate;
                QuantLib::Date maturityDate;
                if (inst->instType() == SwapCurveInstrument<>::Deposit && inst->tenor() == TargetIndexTenorMonths * QuantLib::Months) {
                    auto fixingDate = targetIndex->fixingCalendar().adjust(QuantLib::Settings::instance().evaluationDate());
                    calculated = targetIndex->forecastFixing(fixingDate);
                    auto valueDate = targetIndex->valueDate(fixingDate);
                    startDate = valueDate;
                    maturityDate = targetIndex->maturityDate(valueDate);
                }
                else if (inst->instType() == SwapCurveInstrument<>::Future) {
                    auto future = std::static_pointer_cast<IMMFuture<>>(inst);
                    startDate = future->immDate();
                    maturityDate = future->immEndDate();
                    auto compounding = estimatingCurve->discount(startDate)/ estimatingCurve->discount(maturityDate);
                    auto t = targetIndex->dayCounter().yearFraction(startDate, maturityDate);
                    QuantLib::Rate forwardRate = (compounding - 1.0) / t;
                    auto const& convexityAdj = future->convexityAdj();
                    auto futureRate = forwardRate + convexityAdj;
                    calculated = 100.0 * (1.0 - futureRate);
                }
                else if (inst->instType() == SwapCurveInstrument<>::FRA) {
                    QuantLib::FraRateHelper rateHelper(0.0, inst->tenor(), targetIndex);
                    rateHelper.setTermStructure(&(*estimatingZeroCurve));
                    calculated = rateHelper.impliedQuote();
                    startDate = rateHelper.earliestDate();
                    maturityDate = rateHelper.maturityDate();
                }
                else if (inst->instType() == SwapCurveInstrument<>::Swap) {
                    // create a vanilla swap and price it with the discounting curve and the estimating curve
                    QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> swap = QuantLib::MakeVanillaSwap(inst->tenor(), targetIndex)
                        .withSettlementDays(swapTraits_.settlementDays(inst->tenor()))
                        .withFixedLegCalendar(swapTraits_.fixingCalendar(inst->tenor()))
                        .withFixedLegTenor(swapTraits_.fixedLegTenor(inst->tenor()))
                        .withFixedLegConvention(swapTraits_.fixedLegConvention(inst->tenor()))
                        .withFixedLegDayCount(swapTraits_.fixedLegDayCount(inst->tenor()))
                        .withFixedLegEndOfMonth(swapTraits_.endOfMonth(inst->tenor()))
                        .withFloatingLegCalendar(swapTraits_.fixingCalendar(inst->tenor()))
                        .withFloatingLegEndOfMonth(swapTraits_.endOfMonth(inst->tenor()))
                        .withDiscountingTermStructure(discountCurve);
                    swap->setPricingEngine(swapPricingEngine);
                    calculated = swap->fairRate();
                    startDate = swap->startDate();
                    maturityDate = swap->maturityDate();
                }
                os << inst->tenor();
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