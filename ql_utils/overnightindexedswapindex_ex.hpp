#pragma once
#include <ql/quantlib.hpp>
#include <string>

#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
namespace QuantLib {
	// this class is needed because OvernightIndexedSwapIndex did not implement telescopicValueDates() and averagingMethod()
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndex {
    public:
        OvernightIndexedSwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex,
            bool telescopicValueDates = false,
            RateAveraging::Type averagingMethod = RateAveraging::Compound
        ) : OvernightIndexedSwapIndex(familyName, tenor, settlementDays, currency, overnightIndex, telescopicValueDates, averagingMethod)
        {}
        // missing implementations from QuantLib
        bool telescopicValueDates() const;
        RateAveraging::Type averagingMethod() const;
    };
    // missing implementations from QuantLib
    inline bool OvernightIndexedSwapIndexEx::telescopicValueDates() const {
        return telescopicValueDates_;
    }
    inline RateAveraging::Type OvernightIndexedSwapIndexEx::averagingMethod() const {
        return averagingMethod_;
    }
}
#endif