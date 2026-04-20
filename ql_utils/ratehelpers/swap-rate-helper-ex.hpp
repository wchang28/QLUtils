#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
    class SwapRateHelperEx : public SwapRateHelper {
    protected:
        DateGeneration::Rule rule_;
    protected:
        void initializeDates() override {
            // 1. do not pass the spread here, as it might be a Quote
            //    i.e. it can dynamically change
            // 2. input discount curve Handle might be empty now but it could
            //    be assigned a curve later; use a RelinkableHandle here
            auto tmp = MakeVanillaSwap(tenor_, iborIndex_, 0.0, fwdStart_)
                .withSettlementDays(settlementDays_)  // resets effectiveDate
                .withEffectiveDate(startDate_)
                .withTerminationDate(endDate_)
                .withDiscountingTermStructure(discountRelinkableHandle_)
                .withFixedLegDayCount(fixedDayCount_)
                .withFixedLegTenor(fixedFrequency_ == Once ? tenor_ : Period(fixedFrequency_))
                .withFixedLegConvention(fixedConvention_)
                .withFixedLegTerminationDateConvention(fixedConvention_)
                .withFixedLegCalendar(calendar_)
                .withFixedLegEndOfMonth(endOfMonth_)
                .withFloatingLegCalendar(calendar_)
                .withFloatingLegEndOfMonth(endOfMonth_)
                .withRule(rule_)
                .withIndexedCoupons(useIndexedCoupons_);
            if (floatConvention_) {
                tmp.withFloatingLegConvention(*floatConvention_)
                   .withFloatingLegTerminationDateConvention(*floatConvention_);
            }
            swap_ = tmp;

            simplifyNotificationGraph(*swap_, true);

            earliestDate_ = swap_->startDate();
            maturityDate_ = swap_->maturityDate();

            ext::shared_ptr<IborCoupon> lastCoupon =
                ext::dynamic_pointer_cast<IborCoupon>(swap_->floatingLeg().back());
            latestRelevantDate_ = std::max(maturityDate_, lastCoupon->fixingEndDate());

            switch (pillarChoice_) {
              case Pillar::MaturityDate:
                pillarDate_ = maturityDate_;
                break;
              case Pillar::LastRelevantDate:
                pillarDate_ = latestRelevantDate_;
                break;
              case Pillar::CustomDate:
                // pillarDate_ already assigned at construction time
                QL_REQUIRE(pillarDate_ >= earliestDate_,
                    "pillar date (" << pillarDate_ << ") must be later "
                    "than or equal to the instrument's earliest date (" <<
                    earliestDate_ << ")");
                QL_REQUIRE(pillarDate_ <= latestRelevantDate_,
                    "pillar date (" << pillarDate_ << ") must be before "
                    "or equal to the instrument's latest relevant date (" <<
                    latestRelevantDate_ << ")");
                break;
              default:
                QL_FAIL("unknown Pillar::Choice(" << Integer(pillarChoice_) << ")");
            }

            latestDate_ = pillarDate_; // backward compatibility

        }
        void initialize(
            const ext::shared_ptr<IborIndex>& iborIndex,
            Date customPillarDate
        ) {
            // take fixing into account
            iborIndex_ = iborIndex->clone(termStructureHandle_);
            // We want to be notified of changes of fixings, but we don't
            // want notifications from termStructureHandle_ (they would
            // interfere with bootstrapping.)
            iborIndex_->unregisterWith(termStructureHandle_);

            registerWith(iborIndex_);
            registerWith(spread_);
            registerWith(discountHandle_);

            pillarDate_ = customPillarDate;
            SwapRateHelperEx::initializeDates();            
        }
    public:
        SwapRateHelperEx(
           const std::variant<Rate, Handle<Quote>>& rate,
           const Period& tenor,
           Calendar calendar,
           // fixed leg
           Frequency fixedFrequency,
           BusinessDayConvention fixedConvention,
           DayCounter fixedDayCount,
           // floating leg
           const ext::shared_ptr<IborIndex>& iborIndex,
           Handle<Quote> spread = {},
           const Period& fwdStart = 0 * Days,
           // exogenous discounting curve
           Handle<YieldTermStructure> discountingCurve = {},
           Natural settlementDays = Null<Natural>(),
           Pillar::Choice pillar = Pillar::LastRelevantDate,
           Date customPillarDate = Date(),
           bool endOfMonth = false,
           DateGeneration::Rule rule = DateGeneration::Rule::Forward,
           const ext::optional<bool>& useIndexedCoupons = ext::nullopt,
           const ext::optional<BusinessDayConvention>& floatConvention = ext::nullopt
        ):
        SwapRateHelper(
            rate,
            tenor,
            calendar,
            fixedFrequency,
            fixedConvention,
            fixedDayCount,
            iborIndex,
            spread,
            fwdStart,
            discountingCurve,
            settlementDays,
            pillar,
            customPillarDate,
            endOfMonth,
            useIndexedCoupons,
            floatConvention
        ),
        rule_(rule) {
            initialize(iborIndex, customPillarDate);
        }
        void accept(AcyclicVisitor& v) override {
            auto* v1 = dynamic_cast<Visitor<SwapRateHelperEx>*>(&v);
            if (v1 != nullptr)
                v1->visit(*this);
            else
                SwapRateHelper::accept(v);
        }
    };
}
