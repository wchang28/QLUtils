#pragma once

#include <ql/quantlib.hpp>
#include <memory>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/fixing-date-adjustment.hpp>
#include <ql_utils/utilities/time.hpp>

#define VERIFY_SWAP_RATE_HELPER(helper) \
{   \
    auto swap_rh = helper->swap();  \
    auto swap_expected = makeSwap(startDate);   \
    QL_ASSERT(swap_rh->startDate() == startDate, "swap start/effective date (" << swap_rh->startDate() << ") is not what's expected (" << startDate << ") for tenor " << this->tenor());    \
    QL_ASSERT(swap_rh->maturityDate() == swap_expected->maturityDate(), "swap maturity date (" << swap_rh->maturityDate() << ") is not what's expected (" << swap_expected->maturityDate() << ") for tenor " << this->tenor()); \
    auto dates_rh_fixed = swap_rh->fixedSchedule().dates(); \
    auto dates_expected_fixed = swap_expected->fixedSchedule().dates(); \
    if (dates_rh_fixed != dates_expected_fixed) {   \
        QL_FAIL("rate helper's swap fixed leg schedule is not what's expected for tenor " << this->tenor());    \
    }   \
    auto dates_rh_float = swap_rh->floatingSchedule().dates();  \
    auto dates_expected_float = swap_expected->floatingSchedule().dates();  \
    if (dates_rh_float != dates_expected_float) {   \
        QL_FAIL("rate helper's swap floating leg schedule is not what's expected for tenor " << this->tenor()); \
    }   \
}

namespace QLUtils {
    // base class for swap index traits, which defines the common interface for swap index traits
    class SwapIndexTraitsBase {
    protected:
        QuantLib::Period tenor_;
    public:
        struct FixingResult {
            QuantLib::Calendar fixingCalendar; // swap fixing calendar
            QuantLib::Date fixingDate;  // swap fixing date
            QuantLib::Date startDate;   // swap start date
        };
        typedef QuantLib::Handle<QuantLib::YieldTermStructure> YieldTermStructureHandle;
        typedef QuantLib::ext::shared_ptr<QuantLib::RateHelper> pRateHelper;
        typedef QuantLib::ext::shared_ptr<QuantLib::IborIndex> pIborIndex;
    public:
        SwapIndexTraitsBase(
            const QuantLib::Period& tenor   // swap index's tenor
        ) : tenor_(tenor)
        {}
        const QuantLib::Period& tenor() const {
            return tenor_;
        }
        // returns a meaningful type name for the swap
        virtual std::string typeAlias() const = 0;
        // number of days to settle for the swap
        virtual QuantLib::Natural settlementDays() const = 0;
        // fixing calendar for both legs
        virtual QuantLib::Calendar fixingCalendar() const = 0;
        // end of month flag for both legs
        virtual bool endOfMonth() const = 0;
        // whether cashflow/coupon for both legs aligned (accrual dates + payment dates + couponday counter)
        // coupon aligned means
        // 1. same fixing calendar for both legs
        // 2. same coupon tenor/frequency for both legs
        // 3. same business day adj convention for both legs
        // 4. same payment calendar for both legs (if applicable)
        // 5. same payment leg for both legs (if applicable)
        // 6. same payment business day adj convention for both legs (if applicable)
        // 7. same end of month flag for both legs
        // 8. same cashflow generation rule for both legs (if applicable)
        // 9. same day count convention for both legs
        virtual bool bothLegsCouponsAligned(
            bool includeDayCounter = true
        ) const = 0;
        // iborIndex factory
        virtual pIborIndex makeIborIndex(
            const YieldTermStructureHandle& h = {}  // index estimating term structure
        ) const = 0;
        // rate helper factory
        virtual pRateHelper makeRateHelper(
            QuantLib::Rate quotedFixedRate,
            const QuantLib::Date& startDate,
            const YieldTermStructureHandle& discountingTermStructure = {}   // exogenous discounting curve
        ) const = 0;
        // calculate the fixing date and effective date for a given reference date (default to evaluation date if not provided)
        FixingResult calculateFixing(
            QuantLib::Date refDate = QuantLib::Date()
        ) const {
            if (refDate == QuantLib::Date()) {
                refDate = QuantLib::Settings::instance().evaluationDate();
            }
            auto fixingCalendar = this->fixingCalendar();
            auto settlementDays = this->settlementDays();
            QuantLib::Utils::FixingDateAdjustment fixingAdj(settlementDays, fixingCalendar);
            auto ret = fixingAdj.calculate(refDate);
            auto fixingDate = std::get<0>(ret);
            auto startDate = std::get<1>(ret);
            return FixingResult{
                fixingCalendar,
                fixingDate,
                startDate
            };
        }
        // from the valuation/reference date set by QuantLib::Settings::instance().evaluationDate(),
        // find the number of settlement days that would yield the expected swap start/effective date
        QuantLib::Natural getMatchingSettlementDays(
            const QuantLib::Date& startDate // target start/effective date for the swap
        ) const {
            auto fixingCalendar = this->fixingCalendar();
            QL_REQUIRE(fixingCalendar.isBusinessDay(startDate), "swap start/effective date (" << startDate << ") is not a business day of the swap fixing calendar");
            auto refDate = QuantLib::Settings::instance().evaluationDate();
            QL_REQUIRE(refDate != QuantLib::Date(), "evaluation/reference date is not set");
            refDate = fixingCalendar.adjust(refDate);   // refDate is now a business day on the fixing calendar
            QL_REQUIRE(refDate <= startDate, "evaluation/reference date (" << refDate << ") is after the target swap start/effective date (" << startDate << ")");
            auto n = this->settlementDays();
            auto d = fixingCalendar.advance(refDate, n, QuantLib::Days);
            if (d < startDate) {
                while (d < startDate) {
                    n++;
                    d = fixingCalendar.advance(refDate, n, QuantLib::Days);
                }
            }
            else if (d > startDate) {
                while (d > startDate) {
                    n--;
                    d = fixingCalendar.advance(refDate, n, QuantLib::Days);
                }
            }
            QL_ASSERT(d == startDate, "fixing calendar advancing logic did not yield the expected swap start/effective date (" << startDate << ")");
            QL_ASSERT(n >= 0, "n (" << n << ") is not positive");
            QL_ASSERT(fixingCalendar.advance(refDate, n, QuantLib::Days) == startDate, "fixing calendar advancing logic did not yield the expected swap start/effective date (" << startDate << ")");
            return n;
        }
    };

    // traits class for the OIS swap index
    // both legs of the OIS swap index have the exact same characteristics 
    // example:
    // OISSwapIndexTraits<UsdOvernightIndexedSwapIsdaFix<FedFunds>>
    // OISSwapIndexTraits<UsdOvernightIndexedSwapIsdaFix<Sofr>>
    // OISSwapIndexTraits<GbpOvernightIndexedSwapIsdaFix<Sonia>>
    // OISSwapIndexTraits<EurOvernightIndexedSwapIsdaFix<Estr>>
    // OISSwapIndexTraits<EurOvernightIndexedSwapIsdaFix<Eonia>>
    template<
        typename BASE_SWAP_INDEX
    >
    class OISSwapIndexTraits: public SwapIndexTraitsBase {
    public:
        typedef BASE_SWAP_INDEX BaseSwapIndex;
        typedef typename BaseSwapIndex::OvernightIndex OvernightIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> pOvernightIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> SwapPtr;
    private:
        QuantLib::ext::shared_ptr<BaseSwapIndex> pSwapIndex_;
    public:
        OISSwapIndexTraits(
            const QuantLib::Period& tenor
        ) :
            SwapIndexTraitsBase(tenor),
            pSwapIndex_(new BaseSwapIndex(tenor))
        {}
        std::string typeAlias() const override {
            return "OIS swap";
        }
        // number of days to settle for the swap
        QuantLib::Natural settlementDays() const override {
            return pSwapIndex_->fixingDays();
        }
        // fixing calendar for both legs
        QuantLib::Calendar fixingCalendar() const override {
            return pSwapIndex_->fixingCalendar();
        }
        // end of month flag for both legs
        bool endOfMonth() const override {
            return pSwapIndex_->endOfMonth();
        }
        // whether cashflow/coupon for both legs aligned
        bool bothLegsCouponsAligned(bool) const override {
            return true;
        }
        // cashflow generation rule for both legs
        QuantLib::DateGeneration::Rule rule() const {
            return pSwapIndex_->rule();
        }
        // whether the swap has telescopic value dates
        bool telescopicValueDates() const {
            return pSwapIndex_->telescopicValueDates();
        }
        // averaging method for the overnight leg
        QuantLib::RateAveraging::Type averagingMethod() const {
            return pSwapIndex_->averagingMethod();
        }
        // cashflow frequency for both legs
        QuantLib::Frequency paymentFrequency() const {
            return pSwapIndex_->paymentFrequency();
        }
        // cashflow tenor for both legs
        QuantLib::Period paymentTenor() const {
            return QuantLib::Period(paymentFrequency());
        }
        // business day adj convention for payment date for both legs
        QuantLib::BusinessDayConvention paymentConvention() const {
            return pSwapIndex_->paymentConvention();
        }
        // payment lag for both legs
        QuantLib::Natural paymentLag() const {
            return pSwapIndex_->paymentLag();
        }
        // payment calendar for both legs
        QuantLib::Calendar paymentCalendar() const {
            return pSwapIndex_->paymentCalendar();
        }
        // business day adj convention for both legs
        QuantLib::BusinessDayConvention convention() const {
            return pSwapIndex_->convention();
        }
        // day count convention for both legs
        QuantLib::DayCounter dayCounter() const {
            return pSwapIndex_->dayCounter();
        }
        // create overnight leg's overnight index given an optional index estimating term structure
        pOvernightIndex makeOvernightIndex(
            const YieldTermStructureHandle& h = {}  // index estimating term structure
        ) const {
            pOvernightIndex pIndex(new OvernightIndex(h));
#ifdef _DEBUG
            QL_ASSERT(pIndex != nullptr, "the overnight index is empty");
            auto h2 = pIndex->forwardingTermStructure();
            if (!h.empty()) {
                QL_ASSERT(!h2.empty(), "the forwarding term structure of the overnight index does not match the provided handle");
                QL_ASSERT(h2.currentLink() == h.currentLink(), "the forwarding term structure of the overnight index does not match the provided handle");
            }
            else {
                QL_ASSERT(h2.empty(), "the overnight index has a non-empty forwarding term structure, but no handle was provided to match it");
            }
#endif
            return pIndex;
        }
        pIborIndex makeIborIndex(
            const YieldTermStructureHandle& h = {}  // index estimating term structure
        ) const override {
            return makeOvernightIndex(h);
        }
        SwapPtr makeSwap(
            const QuantLib::Date& startDate, // start date of the swap
            QuantLib::Swap::Type type = QuantLib::Swap::Type::Payer,    // type of the swap
            QuantLib::Rate fixedRate = 0.,  // fixed rate
            QuantLib::Spread overnightSpread = 0.,  // spread on the overnight leg
            const YieldTermStructureHandle& h = {}  // index estimating term structure
        ) const {
            auto overnightIndex = makeOvernightIndex(h);
            SwapPtr swap = QuantLib::MakeOIS(this->tenor(), overnightIndex, fixedRate)
                .withType(type)
                .withOvernightLegSpread(overnightSpread)
                .withEffectiveDate(startDate)
                .withTelescopicValueDates(telescopicValueDates())
                .withAveragingMethod(averagingMethod())
                .withPaymentLag(paymentLag())
                .withPaymentAdjustment(paymentConvention())
                .withPaymentFrequency(paymentFrequency())
                .withPaymentCalendar(paymentCalendar())
                .withFixedLegCalendar(fixingCalendar())
                .withOvernightLegCalendar(fixingCalendar())
                .withRule(rule())
                ;
#ifdef _DEBUG
            QL_ASSERT(swap->startDate() == startDate, "swap start/effective date (" << swap->startDate() << ") is not what's expected (" << startDate << ") for tenor " << this->tenor());
#endif
            return swap;
        }
        // rate helper factory
        virtual pRateHelper makeRateHelper(
            QuantLib::Rate quotedFixedRate,
            const QuantLib::Date& startDate,
            const YieldTermStructureHandle& discountingTermStructure = {}   // exogenous discounting curve
        ) const override {
            auto settlementDays = getMatchingSettlementDays(startDate);
            auto overnightIndex = makeOvernightIndex();
            QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> helper(
                new QuantLib::OISRateHelper(
                    settlementDays, // settlementDays
                    this->tenor(),  // tenor
                    quotedFixedRate,    // fixedRate
                    overnightIndex, // overnightIndex
                    discountingTermStructure,   // discountingCurve - exogenous discounting curve
                    telescopicValueDates(),  // telescopicValueDates
                    paymentLag(),    // paymentLag
                    paymentConvention(), // paymentConvention
                    paymentFrequency(),   // paymentFrequency
                    paymentCalendar(),   // paymentCalendar
                    0 * QuantLib::Days,     // forwardStart
                    QuantLib::Spread(0.),    // overnightSpread
                    QuantLib::Pillar::MaturityDate, // pillar
                    QuantLib::Date(),   // customPillarDate
                    averagingMethod(),    // averagingMethod
                    QuantLib::ext::nullopt,   // endOfMonth
                    QuantLib::ext::nullopt,   // fixedPaymentFrequency
                    fixingCalendar(), // fixedCalendar
                    QuantLib::Null<QuantLib::Natural>(),    // lookbackDays
                    0,  // lockoutDays
                    false,  // applyObservationShift
                    {}, // pricer
                    rule(),   // rule
                    fixingCalendar()  // overnightCalendar
                )
            );
#ifdef _DEBUG
            VERIFY_SWAP_RATE_HELPER(helper);
#endif
            return helper;
        }
    };

    // traits class for the vanilla swap index
    // examples:
    // VanillaSwapIndexTraits<EuriborSwapIsdaFixA>
    // VanillaSwapIndexTraits<UsdTermSofrSwapIsdaFix<1>>
    // VanillaSwapIndexTraits<UsdTermSofrSwapIsdaFix<3>>
    // VanillaSwapIndexTraits<UsdTermSofrSwapIsdaFix<6>>
    // VanillaSwapIndexTraits<UsdTermSofrSwapIsdaFix<12>>
    // VanillaSwapIndexTraits<UsdFwdOISVanillaSwapIndex<Sofr>>
    template<
        typename BASE_SWAP_INDEX
    >
    class VanillaSwapIndexTraits: public SwapIndexTraitsBase {
    public:
        typedef BASE_SWAP_INDEX BaseSwapIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> SwapPtr;
    private:
        QuantLib::ext::shared_ptr<BaseSwapIndex> pSwapIndex_;
		QuantLib::Natural expectedFixedCoupons_;
        QuantLib::Natural expectedFloatingCoupons_;
    public:
        VanillaSwapIndexTraits(
            const QuantLib::Period& tenor
        ) :
            SwapIndexTraitsBase(tenor),
            pSwapIndex_(new BaseSwapIndex(tenor))
        {
            const auto& [multiple_fixed, n_fixed] = QuantLib::Utils::isMultiple(this->tenor(), this->fixedLegTenor());
            const auto& [multiple_floating, n_floating] = QuantLib::Utils::isMultiple(this->tenor(), this->floatingLegTenor());
			QL_REQUIRE(multiple_fixed, "the vanilla swap index tenor (" << this->tenor() << ") is not an integer multiple of the fixed leg cashflow tenor (" << this->fixedLegTenor() << ")");
			QL_REQUIRE(multiple_floating, "the vanilla swap index tenor (" << this->tenor() << ") is not an integer multiple of the floating leg cashflow tenor (" << this->floatingLegTenor() << ")");
            expectedFixedCoupons_ = n_fixed;
            expectedFloatingCoupons_ = n_floating;
        }
        std::string typeAlias() const override {
            return "vanilla swap";
        }
        // number of days to settle for the swap
        QuantLib::Natural settlementDays() const override {
            return pSwapIndex_->fixingDays();
        }
        // fixing calendar for both legs
        QuantLib::Calendar fixingCalendar() const override {
            return pSwapIndex_->fixingCalendar();
        }
        // end of month flag for both legs
        bool endOfMonth() const override {
            auto pIndexEx = QuantLib::ext::dynamic_pointer_cast<QuantLib::SwapIndexEx>(pSwapIndex_);
            return (pIndexEx != nullptr ? pIndexEx->endOfMonth() : false);
        }
        // whether cashflow/coupon for both legs aligned
        bool bothLegsCouponsAligned(
            bool includeDayCounter
        ) const override {
            auto sameCouponFrequency = (fixedLegFrequency() == floatingLegFrequency());
            auto sameBusinessDayAdj = (fixedLegConvention() == floatingLegConvention());
            auto ret = (sameCouponFrequency && sameBusinessDayAdj);
            if (includeDayCounter) {
                auto sameDayCount = (fixedLegDayCount() == floatingLegDayCount());
                ret = ret && sameDayCount;
            }
            return ret;
        }
        // fixed leg's cashflow tenor
        QuantLib::Period fixedLegTenor() const {
            return pSwapIndex_->fixedLegTenor();
        }
        // fixed leg's cashflow frequency
        QuantLib::Frequency fixedLegFrequency() const {
            auto freq = fixedLegTenor().frequency();
            return freq;
        }
        // fixed leg's business day adj convention
        QuantLib::BusinessDayConvention fixedLegConvention() const {
            return pSwapIndex_->fixedLegConvention();
        }
        // fixed leg's day count convention
        QuantLib::DayCounter fixedLegDayCount() const {
            return pSwapIndex_->dayCounter();
        }
        // floating leg's cashflow tenor
        QuantLib::Period floatingLegTenor() const {
            return pSwapIndex_->iborIndex()->tenor();
        }
        // floating leg's cashflow frequency
        QuantLib::Frequency floatingLegFrequency() const {
            return floatingLegTenor().frequency();
        }
        // floating leg's business day adj convention
        QuantLib::BusinessDayConvention floatingLegConvention() const {
            return pSwapIndex_->iborIndex()->businessDayConvention();
        }
        // floating leg's day count convention
        QuantLib::DayCounter floatingLegDayCount() const {
            return pSwapIndex_->iborIndex()->dayCounter();
        }
        // create floating leg's ibor index given an optional index estimating term structure
        pIborIndex makeIborIndex(
            const YieldTermStructureHandle& h = {}  // index estimating term structure
        ) const override {
            BaseSwapIndex swapIndex(this->tenor(), h);
            auto pIndex = swapIndex.iborIndex();
#ifdef _DEBUG
            QL_ASSERT(pIndex != nullptr, "the ibor index is empty");
            auto h2 = pIndex->forwardingTermStructure();
            if (!h.empty()) {
                QL_ASSERT(!h2.empty(), "the forwarding term structure of the ibor index does not match the provided handle");
                QL_ASSERT(h2.currentLink() == h.currentLink(), "the forwarding term structure of the ibor index does not match the provided handle");
            }
            else {
                QL_ASSERT(h2.empty(), "the ibor index has a non-empty forwarding term structure, but no handle was provided to match it");
            }
#endif
            return pIndex;
        }
        SwapPtr makeSwap(
            const QuantLib::Date& startDate, // start date of the swap
            QuantLib::Swap::Type type = QuantLib::Swap::Type::Payer,    // type of the swap
            QuantLib::Rate fixedRate = 0.,  // fixed rate
            QuantLib::Spread floatSpread = 0.,  // spread on the floating leg
            const YieldTermStructureHandle& h = {}  // index estimating term structure
        ) const {
            auto iborIndex = makeIborIndex(h);
            SwapPtr swap = QuantLib::MakeVanillaSwap(this->tenor(), iborIndex, fixedRate)
                .withType(type)
                .withFloatingLegSpread(floatSpread)
                .withEffectiveDate(startDate)
                .withFixedLegCalendar(fixingCalendar())
                .withFixedLegDayCount(fixedLegDayCount())
                .withFixedLegTenor(fixedLegTenor())
                .withFixedLegConvention(fixedLegConvention())
                .withFixedLegTerminationDateConvention(fixedLegConvention())
                .withFixedLegEndOfMonth(endOfMonth())
                .withFloatingLegCalendar(fixingCalendar())
                .withFloatingLegEndOfMonth(endOfMonth())
                ;
#ifdef _DEBUG
            QL_ASSERT(swap->startDate() == startDate, "swap start/effective date (" << swap->startDate() << ") is not what's expected (" << startDate << ") for tenor " << this->tenor());
            auto fixedCoupons = swap->fixedSchedule().dates().size() - 1;
            auto floatingCoupons = swap->floatingSchedule().dates().size() - 1;
			QL_ASSERT(fixedCoupons == expectedFixedCoupons_, "the number of fixed coupons (" << fixedCoupons << ") is not what's expected (" << expectedFixedCoupons_ << ") for tenor " << this->tenor());
			QL_ASSERT(floatingCoupons == expectedFloatingCoupons_, "the number of floating coupons (" << floatingCoupons << ") is not what's expected (" << expectedFloatingCoupons_ << ") for tenor " << this->tenor());
#endif
            return swap;
        }
        // rate helper factory
        virtual pRateHelper makeRateHelper(
            QuantLib::Rate quotedFixedRate,
            const QuantLib::Date& startDate,
            const YieldTermStructureHandle& discountingTermStructure = {}   // exogenous discounting curve
        ) const override {
            auto settlementDays = getMatchingSettlementDays(startDate);
            auto iborIndex = makeIborIndex();
            QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> helper(
                new QuantLib::SwapRateHelper(
                    quotedFixedRate,    // rate
                    this->tenor(), // tenor
                    this->fixingCalendar(), //calendar
                    fixedLegFrequency(), // fixedFrequency
                    fixedLegConvention(), // fixedConvention
                    fixedLegDayCount(), // fixedDayCount
                    iborIndex, // iborIndex
                    {},    // spread
                    0 * QuantLib::Days,   // fwdStart
                    discountingTermStructure,   // discountingCurve - exogenous discounting curve
                    settlementDays, // settlementDays
                    QuantLib::Pillar::MaturityDate, // pillar
                    QuantLib::Date(),   // customPillarDate
                    endOfMonth()  // endOfMonth
                )
            );
#ifdef _DEBUG
            VERIFY_SWAP_RATE_HELPER(helper);
#endif
            return helper;
        }
    };
}

#undef VERIFY_SWAP_RATE_HELPER