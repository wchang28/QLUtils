#pragma once
#include <ql/quantlib.hpp>

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
        const bool& telescopicValueDates() const { return telescopicValueDates_; }
        const RateAveraging::Type& averagingMethod() const { return averagingMethod_; }
    };
}
#endif