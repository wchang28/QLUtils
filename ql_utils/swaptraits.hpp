#pragma once

#include <ql/quantlib.hpp>
#include <memory>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/fixing-date-adjustment.hpp>

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
    public:
        SwapIndexTraitsBase(
            const QuantLib::Period& tenor
        ) : tenor_(tenor)
        {}
        const QuantLib::Period& tenor() const {
            return tenor_;
		}

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
        virtual bool bothLegsCouponsAligned(bool includeDayCounter = true) const = 0;
        
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
    private:
        std::shared_ptr<BaseSwapIndex> pSwapIndex_;
    public:
        OISSwapIndexTraits(
            const QuantLib::Period& tenor
		) :
            SwapIndexTraitsBase(tenor),
            pSwapIndex_(new BaseSwapIndex(tenor))
        {}
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
    private:
        std::shared_ptr<BaseSwapIndex> pSwapIndex_;
    public:
        VanillaSwapIndexTraits(
			const QuantLib::Period& tenor
        ) :
            SwapIndexTraitsBase(tenor),
            pSwapIndex_(new BaseSwapIndex(tenor))
		{}
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
            auto pIndexEx = std::dynamic_pointer_cast<QuantLib::SwapIndexEx>(pSwapIndex_);
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
        QuantLib::ext::shared_ptr<QuantLib::IborIndex> makeIborIndex(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}	// index estimating term structure
        ) const {
            BaseSwapIndex swapIndex(tenor(), h);
            auto pIndex = swapIndex.iborIndex();
			QL_ASSERT(pIndex != nullptr, "the ibor index of the swap index " << swapIndex.name() << " is empty");
			auto h2 = pIndex->forwardingTermStructure();
            if (!h.empty()) {
                QL_ASSERT(!h2.empty(), "the forwarding term structure of the ibor index does not match the provided handle");
                QL_ASSERT(h2.currentLink() == h.currentLink(), "the forwarding term structure of the ibor index does not match the provided handle");
            }
            else {
				QL_ASSERT(h2.empty(), "the ibor index of the swap index " << swapIndex.name() << " has a non-empty forwarding term structure, but no handle was provided to match it");
            }
            return pIndex;
        }
    };
}
