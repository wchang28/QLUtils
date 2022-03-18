#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // EUR OIS swap index
    template <typename OvernightIndex> // OvernightIndex can be Estr
    class EurOvernightIndexedSwapIsdaFix : public
#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
        QuantLib::OvernightIndexedSwapIndexEx
#else
        QuantLib::OvernightIndexedSwapIndex
#endif
    {
public:
    EurOvernightIndexedSwapIsdaFix(
        const QuantLib::Period& tenor,
        const QuantLib::Handle<QuantLib::YieldTermStructure>& indexEstimatingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
    ) :
#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
        OvernightIndexedSwapIndexEx
#else
        OvernightIndexedSwapIndex
#endif
        (
            std::string("EurOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
            tenor,
            2,
            QuantLib::EURCurrency(),
            QuantLib::ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
            true,
            QuantLib::RateAveraging::Compound
        ) {}
    };
}
