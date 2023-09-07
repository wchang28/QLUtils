#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // enhanced version of the OvernightIndexedSwapIndex that exposes telescopicValueDates, averagingMethod, and paymentLag
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndex {
    protected:
        Natural paymentLag_;
    public:
        OvernightIndexedSwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex,
            bool telescopicValueDates = false,
            RateAveraging::Type averagingMethod = RateAveraging::Compound,
            Natural paymentLag = 0
        ): OvernightIndexedSwapIndex(familyName, tenor, settlementDays, currency, overnightIndex, telescopicValueDates, averagingMethod)
            , paymentLag_(paymentLag)
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
                    .withPaymentLag(paymentLag_);
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
    };
}
