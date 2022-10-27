#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // GBP OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sonia
    >
    class GbpOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndex {
public:
    typedef typename OVERNIGHTINDEX OvernightIndex;
public:
    GbpOvernightIndexedSwapIsdaFix(
        const Period& tenor,
        const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
    ) :
        OvernightIndexedSwapIndex
        (
            std::string("GbpOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
            tenor,
            0,
            GBPCurrency(),
            ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
            false,
            RateAveraging::Compound
        ) {}
    };
}
