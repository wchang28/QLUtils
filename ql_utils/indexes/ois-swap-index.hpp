#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // enhanced version of the OvernightIndexedSwapIndex that supports
    // cashflow frequency for both legs
    // telescopicValueDates()
    // averagingMethod()
    // paymentLag()
    // paymentConvention()
    // paymentFrequency()
    // paymentCalendar()
    template <
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class OvernightIndexedSwapIndexEnhanced : public OvernightIndexedSwapIndex {
    protected:
        Natural paymentLag_;
        Calendar paymentCalendar_;
    private:
        static std::string makeFamilyName(
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex
        ) {
            std::ostringstream oss;
            oss << currency.code();
            oss << "OvernightIndexedSwapIndex<<";
            oss << overnightIndex->name();
            oss << ">>";
            return oss.str();
        }
    public:
        OvernightIndexedSwapIndexEnhanced(
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex,
            Natural paymentLag = 0,
            const Calendar& paymentCalendar = Calendar()
        ) :
            OvernightIndexedSwapIndex(
                makeFamilyName(currency, overnightIndex),   // familyName
                tenor,                                      // swap tenor
                settlementDays,
                currency,
                overnightIndex,
                false,                                      // telescopicValueDates
                RateAveraging::Compound                     // averagingMethod
            ),
            paymentLag_(paymentLag),
            paymentCalendar_(paymentCalendar)
        {
            this->fixedLegTenor_ = Period(FREQ);    // set the cashflow tenor for both legs
        }
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
        ext::shared_ptr<OvernightIndexedSwap> makeSwap(
            const Date& fixingDate,
            Swap::Type type = Swap::Type::Payer,
            Rate fixedRate = Null<Rate>()
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");
            ext::shared_ptr<OvernightIndexedSwap> swap = MakeOIS(tenor_, overnightIndex_, fixedRate)
                .withType(type)
                .withEffectiveDate(valueDate(fixingDate))
                .withFixedLegDayCount(dayCounter_)
                .withTelescopicValueDates(telescopicValueDates_)
                .withAveragingMethod(averagingMethod_)
                .withPaymentLag(paymentLag_)
                .withPaymentAdjustment(paymentConvention())
                .withPaymentFrequency(paymentFrequency())
                .withPaymentCalendar(paymentCalendar_)
                .withRule(DateGeneration::Forward)
                ;
            return swap;
        }
        ext::shared_ptr<OvernightIndexedSwap> underlyingSwap(
            const Date& fixingDate
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");
            // caching mechanism
            if (lastFixingDate_ != fixingDate) {
                Rate fixedRate = 0.0;
                lastSwap_ = makeSwap(fixingDate, Swap::Type::Payer, fixedRate);
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndexEnhanced<FREQ> {
    public:
        typedef OVERNIGHTINDEX OvernightIndex;
    public:
        OvernightIndexedSwapIndexEx(
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {},
            Natural paymentLag = 0,
            const Calendar& paymentCalendar = Calendar()
        ):
            OvernightIndexedSwapIndexEnhanced<FREQ>(
                tenor,
                settlementDays,
                currency,
                ext::shared_ptr<OvernightIndex>(new OVERNIGHTINDEX(indexEstimatingTermStructure)),  // overnightIndex
                paymentLag,
                paymentCalendar
            )
        {}
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
                    .withFloatingLegEndOfMonth(floatingLegEndOfMonth_)
                    .withRule(DateGeneration::Forward)
                    ;
                else
                    lastSwap_ = MakeVanillaSwap(tenor_, iborIndex_, fixedRate)
                    .withEffectiveDate(valueDate(fixingDate))
                    .withFixedLegCalendar(fixingCalendar())
                    .withFixedLegDayCount(dayCounter_)
                    .withFixedLegTenor(fixedLegTenor_)
                    .withFixedLegConvention(fixedLegConvention_)
                    .withFixedLegTerminationDateConvention(fixedLegConvention_)
                    .withFixedLegEndOfMonth(fixedLegEndOfMonth_)
                    .withFloatingLegEndOfMonth(floatingLegEndOfMonth_)
                    .withRule(DateGeneration::Forward)
                    ;
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
        ext::shared_ptr<VanillaSwap> makeSwap(
            const Date& fixingDate,
            Swap::Type type = Swap::Type::Payer,
            Rate fixedRate = Null<Rate>()
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");
            ext::shared_ptr<VanillaSwap> swap;
            if (exogenousDiscount_)
                swap = MakeVanillaSwap(tenor_, iborIndex_, fixedRate)
                .withType(type)
                .withEffectiveDate(valueDate(fixingDate))
                .withFixedLegCalendar(fixingCalendar())
                .withFixedLegDayCount(dayCounter_)
                .withFixedLegTenor(fixedLegTenor_)
                .withFixedLegConvention(fixedLegConvention_)
                .withFixedLegTerminationDateConvention(fixedLegConvention_)
                .withDiscountingTermStructure(discount_)
                .withFixedLegEndOfMonth(fixedLegEndOfMonth_)
                .withFloatingLegEndOfMonth(floatingLegEndOfMonth_)
                .withRule(DateGeneration::Forward)
                ;
            else
                swap = MakeVanillaSwap(tenor_, iborIndex_, fixedRate)
                .withType(type)
                .withEffectiveDate(valueDate(fixingDate))
                .withFixedLegCalendar(fixingCalendar())
                .withFixedLegDayCount(dayCounter_)
                .withFixedLegTenor(fixedLegTenor_)
                .withFixedLegConvention(fixedLegConvention_)
                .withFixedLegTerminationDateConvention(fixedLegConvention_)
                .withFixedLegEndOfMonth(fixedLegEndOfMonth_)
                .withFloatingLegEndOfMonth(floatingLegEndOfMonth_)
                .withRule(DateGeneration::Forward)
                ;
            return swap;
        }
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class FwdOISVanillaSwapIndex : public SwapIndexEx {
    public:
        typedef OVERNIGHTINDEX OvernightIndex;
        typedef OvernightCompoundedAverageInArrearsIndex<OvernightIndex, FREQ> IndexType;
    public:
        // both legs' cashflow frequency
        static Frequency legsFrequency() {
            return FREQ;
        }
        // both legs' cashflow tenor
        static Period legsTenor() {
            return Period(legsFrequency());
        }
    private:
        static std::string makeFamilyName(
            const Currency& currency,
            Natural fixingDays  // index fixing days
        ) {
            IndexType index(fixingDays);
            std::ostringstream oss;
            oss << currency.code();
            oss << "FwdOISVanillaSwapIndex";
            oss << "(" << legsFrequency() << ")";
            oss << "<<" << index.name() << ">>";
            return oss.str();
        }
    public:
        FwdOISVanillaSwapIndex(
            const Period& tenor,        // tenor of the swap
            Natural settlementDays,     // number of days required to settle the swap
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            SwapIndexEx(
                makeFamilyName(OvernightIndex().currency(), settlementDays), // familyName
                tenor,  // tenor
                settlementDays, // settlementDays
                OvernightIndex().currency(),	// currency = overnight index's currency
                OvernightIndex().fixingCalendar(),	// swap fixingCalendar =  overnight index's fixing calendar
                legsTenor(), // fixedLegTenor, fixed leg tenor is usually one year
                ModifiedFollowing, // fixedLegConvention, = ModifiedFollowing since all OIS swap's fixed leg convention is ModifiedFollowing
                OvernightIndex().dayCounter(), // fixedLegDaycounter = overnight index's day counter
                ext::shared_ptr<IborIndex>(
                    new IndexType(
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
