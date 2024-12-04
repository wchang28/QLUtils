#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // enhanced version of the OvernightIndexedSwapIndex that supports
    // telescopicValueDates()
    // averagingMethod()
    // paymentLag()
    // paymentConvention()
    // paymentFrequency()
    // paymentCalendar()
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
    >
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndex {
    protected:
        Natural paymentLag_;
        Calendar paymentCalendar_;
    private:
        static std::string makeFamilyName(
            const Currency& currency
        ) {
            std::ostringstream oss;
            oss << currency.code();
            oss << "OvernightIndexedSwapIndex<<";
            oss << OVERNIGHTINDEX().name();
            oss << ">>";
            return oss.str();
        }
    public:
        OvernightIndexedSwapIndexEx(
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>(),
            Natural paymentLag = 0,
            const Calendar& paymentCalendar = Calendar()
        ): 
            OvernightIndexedSwapIndex(
                makeFamilyName(currency),
                tenor,
                settlementDays,
                currency,
                ext::shared_ptr<OvernightIndex>(new OVERNIGHTINDEX(indexEstimatingTermStructure)),  // overnightIndex
                false,  // telescopicValueDates
                RateAveraging::Compound // averagingMethod
            ),
            paymentLag_(paymentLag),
            paymentCalendar_(paymentCalendar)
        {}
        bool telescopicValueDates() const {
            return telescopicValueDates_;
        }
        RateAveraging::Type averagingMethod() const {
            return averagingMethod_;
        }
        Natural paymentLag() const {
            return paymentLag_;
        }
        BusinessDayConvention paymentConvention() const {
            return BusinessDayConvention::Following;
        }
        QuantLib::Frequency paymentFrequency() const {
            return fixedLegTenor().frequency();
        }
        const Calendar& paymentCalendar() const {
            return paymentCalendar_;
        }
        ext::shared_ptr<OvernightIndexedSwap> underlyingSwap(
            const Date& fixingDate
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");

            // caching mechanism
            if (lastFixingDate_ != fixingDate) {
                Rate fixedRate = 0.0;
                lastSwap_ = MakeOIS(tenor_, overnightIndex_, fixedRate)
                    .withEffectiveDate(valueDate(fixingDate))
                    .withFixedLegDayCount(dayCounter_)
                    .withTelescopicValueDates(telescopicValueDates_)
                    .withAveragingMethod(averagingMethod_)
                    .withPaymentLag(paymentLag_)
                    .withPaymentAdjustment(paymentConvention())
                    .withPaymentFrequency(paymentFrequency())
                    .withPaymentCalendar(paymentCalendar_)
                    ;
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
    };

    // enhanced version of SwapIndex thats support end of month for both legs
    class SwapIndexEx : public SwapIndex {
    protected:
        bool fixedLegEndOfMonth_;
        bool floatingLegEndOfMonth_;
    public:
        SwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Calendar& fixingCalendar,
            const Period& fixedLegTenor,
            BusinessDayConvention fixedLegConvention,
            const DayCounter& fixedLegDayCounter,
            ext::shared_ptr<IborIndex> iborIndex,
            bool fixedLegEndOfMonth = false,
            bool floatingLegEndOfMonth = false
        ) : SwapIndex(
            familyName,
            tenor,
            settlementDays,
            currency,
            fixingCalendar,
            fixedLegTenor,
            fixedLegConvention,
            fixedLegDayCounter,
            iborIndex
            ),
            fixedLegEndOfMonth_(fixedLegEndOfMonth),
            floatingLegEndOfMonth_(floatingLegEndOfMonth)
        {}
        SwapIndexEx(const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Calendar& fixingCalendar,
            const Period& fixedLegTenor,
            BusinessDayConvention fixedLegConvention,
            const DayCounter& fixedLegDayCounter,
            ext::shared_ptr<IborIndex> iborIndex,
            Handle<YieldTermStructure> discountingTermStructure,
            bool fixedLegEndOfMonth = false,
            bool floatingLegEndOfMonth = false
        ): SwapIndex(
            familyName,
            tenor,
            settlementDays,
            currency,
            fixingCalendar,
            fixedLegTenor,
            fixedLegConvention,
            fixedLegDayCounter,
            iborIndex,
            discountingTermStructure
            ),
            fixedLegEndOfMonth_(fixedLegEndOfMonth),
            floatingLegEndOfMonth_(floatingLegEndOfMonth)
        {}
        bool fixedLegEndOfMonth() const { return fixedLegEndOfMonth_; }
        bool floatingLegEndOfMonth() const { return floatingLegEndOfMonth_; }
        ext::shared_ptr<VanillaSwap> underlyingSwap(const Date& fixingDate) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");

            // caching mechanism
            if (lastFixingDate_ != fixingDate) {
                Rate fixedRate = 0.0;
                if (exogenousDiscount_)
                    lastSwap_ = MakeVanillaSwap(tenor_, iborIndex_, fixedRate)
                    .withEffectiveDate(valueDate(fixingDate))
                    .withFixedLegCalendar(fixingCalendar())
                    .withFixedLegDayCount(dayCounter_)
                    .withFixedLegTenor(fixedLegTenor_)
                    .withFixedLegConvention(fixedLegConvention_)
                    .withFixedLegTerminationDateConvention(fixedLegConvention_)
                    .withDiscountingTermStructure(discount_)
                    .withFixedLegEndOfMonth(fixedLegEndOfMonth_)
                    .withFloatingLegEndOfMonth(floatingLegEndOfMonth_);
                else
                    lastSwap_ = MakeVanillaSwap(tenor_, iborIndex_, fixedRate)
                    .withEffectiveDate(valueDate(fixingDate))
                    .withFixedLegCalendar(fixingCalendar())
                    .withFixedLegDayCount(dayCounter_)
                    .withFixedLegTenor(fixedLegTenor_)
                    .withFixedLegConvention(fixedLegConvention_)
                    .withFixedLegTerminationDateConvention(fixedLegConvention_)
                    .withFixedLegEndOfMonth(fixedLegEndOfMonth_)
                    .withFloatingLegEndOfMonth(floatingLegEndOfMonth_);
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
    >
    class FwdOISVanillaSwapIndex : public SwapIndexEx {
    public:
        typedef OVERNIGHTINDEX OvernightIndex;
    private:
        static std::string makeFamilyName(
            const Currency& currency
        ) {
            std::ostringstream oss;
            oss << currency.code();
            oss << "FwdOISVanillaSwapIndex<<";
            oss << OVERNIGHTINDEX().name();
            oss << ">>";
            return oss.str();
        }
    public:
        FwdOISVanillaSwapIndex(
            const Period& tenor,
            Natural settlementDays,
            const Currency currency,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            SwapIndexEx(
                makeFamilyName(currency), // familyName
                tenor,  // tenor
                settlementDays, // settlementDays
                currency,	// currency
                OvernightIndex().fixingCalendar(),	// swap fixingCalendar =  overnight index's fixing calendar
                1 * Years, // fixedLegTenor, fixed leg tenor is also one year
                ModifiedFollowing, // fixedLegConvention, = ModifiedFollowing since all OIS swap's fixed leg convention is ModifiedFollowing
                OvernightIndex().dayCounter(), // fixedLegDaycounter = overnight index's day counter
                ext::shared_ptr<IborIndex>(
                    new OvernightCompoundedAverageInArrearsIndex<OvernightIndex>(
                        settlementDays, // fixingDays of the index matches with the settlementDays of the swap
                        indexEstimatingTermStructure
                    )
                ),  // iborIndex
                true,   // fixedLegEndOfMonth, OIS swaps usually have EndOfMonth=true for both legs
                true    // floatingLegEndOfMonth, OIS swaps usually have EndOfMonth=true for both legs
            )
        {}
    };
}
