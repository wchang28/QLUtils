#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // enhanced version of the OvernightIndexedSwapIndex that supports
    // telescopicValueDates
    // averagingMethod
    // paymentLag
    // paymentConvention
    // paymentFrequency
    // paymentCalendar
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndex {
    protected:
        Natural paymentLag_;
        BusinessDayConvention paymentConvention_;
        Calendar paymentCalendar_;
    public:
        OvernightIndexedSwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex,
            bool telescopicValueDates = false,
            RateAveraging::Type averagingMethod = RateAveraging::Compound,
            Natural paymentLag = 0,
            BusinessDayConvention paymentConvention = BusinessDayConvention::Following,
            const Calendar& paymentCalendar = Calendar()
        ): OvernightIndexedSwapIndex(familyName, tenor, settlementDays, currency, overnightIndex, telescopicValueDates, averagingMethod)
            , paymentLag_(paymentLag), paymentConvention_(paymentConvention), paymentCalendar_(paymentCalendar)
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
            return paymentConvention_;
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
                    .withPaymentAdjustment(paymentConvention_)
                    .withPaymentFrequency(paymentFrequency())
                    .withPaymentCalendar(paymentCalendar_)
                    ;
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
    };
}
