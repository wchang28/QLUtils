#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // EUR OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Estr or Eonia
    >
    class EurOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndex {
public:
    typedef typename OVERNIGHTINDEX OvernightIndex;
public:
    EurOvernightIndexedSwapIsdaFix(
        const Period& tenor,
        const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
    ) :
        OvernightIndexedSwapIndex
        (
            std::string("EurOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
            tenor,
            2,
            EURCurrency(),
            ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
            false,
            RateAveraging::Compound
        ) {}
    };
}
