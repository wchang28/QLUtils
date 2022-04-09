#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // GBP OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sonia
    >
    class GbpOvernightIndexedSwapIsdaFix : public
#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
        QuantLib::OvernightIndexedSwapIndexEx
#else
        QuantLib::OvernightIndexedSwapIndex
#endif
    {
public:
    typedef typename OVERNIGHTINDEX OvernightIndex;
public:
    GbpOvernightIndexedSwapIsdaFix(
        const QuantLib::Period& tenor,
        const QuantLib::Handle<QuantLib::YieldTermStructure>& indexEstimatingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
    ) :
#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
        OvernightIndexedSwapIndexEx
#else
        OvernightIndexedSwapIndex
#endif
        (
            std::string("GbpOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
            tenor,
            0,
            QuantLib::GBPCurrency(),
            QuantLib::ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
            true,
            QuantLib::RateAveraging::Compound
        ) {}
    };
}
